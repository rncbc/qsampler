// qsamplerMainForm.cpp
//
/****************************************************************************
   Copyright (C) 2004-2020, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007-2019 Christian Schoenebeck

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qsamplerAbout.h"
#include "qsamplerMainForm.h"

#include "qsamplerOptions.h"
#include "qsamplerChannel.h"
#include "qsamplerMessages.h"

#include "qsamplerChannelStrip.h"
#include "qsamplerInstrumentList.h"

#include "qsamplerInstrumentListForm.h"
#include "qsamplerDeviceForm.h"
#include "qsamplerOptionsForm.h"
#include "qsamplerDeviceStatusForm.h"

#include <QMdiArea>
#include <QMdiSubWindow>

#include <QApplication>
#include <QProcess>
#include <QMessageBox>

#include <QRegExp>
#include <QTextStream>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QUrl>

#include <QDragEnterEvent>

#include <QStatusBar>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QTimer>
#include <QDateTime>

#include <QElapsedTimer>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QMimeData>
#endif

#if QT_VERSION < QT_VERSION_CHECK(4, 5, 0)
namespace Qt {
const WindowFlags WindowCloseButtonHint = WindowFlags(0x08000000);
}
#endif

#ifdef CONFIG_LIBGIG
#include <gig.h>
#endif

// Needed for lroundf()
#include <math.h>

#ifndef CONFIG_ROUND
static inline long lroundf ( float x )
{
	if (x >= 0.0f)
		return long(x + 0.5f);
	else
		return long(x - 0.5f);
}
#endif


// All winsock apps needs this.
#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
static WSADATA _wsaData;
#undef HAVE_SIGNAL_H
#endif


//-------------------------------------------------------------------------
// LADISH Level 1 support stuff.

#if defined(HAVE_SIGNAL_H) && defined(HAVE_SYS_SOCKET_H)

#include <QSocketNotifier>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

// File descriptor for SIGUSR1 notifier.
static int g_fdSigusr1[2] = { -1, -1 };

// Unix SIGUSR1 signal handler.
static void qsampler_sigusr1_handler ( int /* signo */ )
{
	char c = 1;

	(::write(g_fdSigusr1[0], &c, sizeof(c)) > 0);
}

// File descriptor for SIGTERM notifier.
static int g_fdSigterm[2] = { -1, -1 };

// Unix SIGTERM signal handler.
static void qsampler_sigterm_handler ( int /* signo */ )
{
	char c = 1;

	(::write(g_fdSigterm[0], &c, sizeof(c)) > 0);
}

#endif	// HAVE_SIGNAL_H


//-------------------------------------------------------------------------
// QSampler -- namespace


namespace QSampler {

// Timer constant stuff.
#define QSAMPLER_TIMER_MSECS    200

// Status bar item indexes
#define QSAMPLER_STATUS_CLIENT  0       // Client connection state.
#define QSAMPLER_STATUS_SERVER  1       // Currenr server address (host:port)
#define QSAMPLER_STATUS_CHANNEL 2       // Active channel caption.
#define QSAMPLER_STATUS_SESSION 3       // Current session modification state.


// Specialties for thread-callback comunication.
#define QSAMPLER_LSCP_EVENT   QEvent::Type(QEvent::User + 1)


//-------------------------------------------------------------------------
// QSampler::LscpEvent -- specialty for LSCP callback comunication.

class LscpEvent : public QEvent
{
public:

	// Constructor.
	LscpEvent(lscp_event_t event, const char *pchData, int cchData)
		: QEvent(QSAMPLER_LSCP_EVENT)
	{
		m_event = event;
		m_data  = QString::fromUtf8(pchData, cchData);
	}

	// Accessors.
	lscp_event_t  event() { return m_event; }
	const QString& data() { return m_data;  }

private:

	// The proper event type.
	lscp_event_t m_event;
	// The event data as a string.
	QString      m_data;
};


//-------------------------------------------------------------------------
// QSampler::Workspace -- Main window workspace (MDI Area) decl.

class Workspace : public QMdiArea
{
public:

	Workspace(MainForm *pMainForm) : QMdiArea(pMainForm) {}

protected:

	void resizeEvent(QResizeEvent *)
	{
		MainForm *pMainForm = static_cast<MainForm *> (parentWidget());
		if (pMainForm)
			pMainForm->channelsArrangeAuto();
	}
};


//-------------------------------------------------------------------------
// QSampler::MainForm -- Main window form implementation.

// Kind of singleton reference.
MainForm *MainForm::g_pMainForm = nullptr;

MainForm::MainForm ( QWidget *pParent )
	: QMainWindow(pParent)
{
	m_ui.setupUi(this);

	// Pseudo-singleton reference setup.
	g_pMainForm = this;

	// Initialize some pointer references.
	m_pOptions = nullptr;

	// All child forms are to be created later, not earlier than setup.
	m_pMessages = nullptr;
	m_pInstrumentListForm = nullptr;
	m_pDeviceForm = nullptr;

	// We'll start clean.
	m_iUntitled   = 0;
	m_iDirtySetup = 0;
	m_iDirtyCount = 0;

	m_pServer = nullptr;
	m_pClient = nullptr;

	m_iStartDelay = 0;
	m_iTimerDelay = 0;

	m_iTimerSlot = 0;

#if defined(HAVE_SIGNAL_H) && defined(HAVE_SYS_SOCKET_H)

	// Set to ignore any fatal "Broken pipe" signals.
	::signal(SIGPIPE, SIG_IGN);

	// LADISH Level 1 suport.

	// Initialize file descriptors for SIGUSR1 socket notifier.
	::socketpair(AF_UNIX, SOCK_STREAM, 0, g_fdSigusr1);
	m_pSigusr1Notifier
		= new QSocketNotifier(g_fdSigusr1[1], QSocketNotifier::Read, this);

	QObject::connect(m_pSigusr1Notifier,
		SIGNAL(activated(int)),
		SLOT(handle_sigusr1()));

	// Install SIGUSR1 signal handler.
	struct sigaction sigusr1;
	sigusr1.sa_handler = qsampler_sigusr1_handler;
	sigemptyset(&sigusr1.sa_mask);
	sigusr1.sa_flags = 0;
	sigusr1.sa_flags |= SA_RESTART;
	::sigaction(SIGUSR1, &sigusr1, nullptr);

	// Initialize file descriptors for SIGTERM socket notifier.
	::socketpair(AF_UNIX, SOCK_STREAM, 0, g_fdSigterm);
	m_pSigtermNotifier
		= new QSocketNotifier(g_fdSigterm[1], QSocketNotifier::Read, this);

	QObject::connect(m_pSigtermNotifier,
		SIGNAL(activated(int)),
		SLOT(handle_sigterm()));

	// Install SIGTERM signal handler.
	struct sigaction sigterm;
	sigterm.sa_handler = qsampler_sigterm_handler;
	sigemptyset(&sigterm.sa_mask);
	sigterm.sa_flags = 0;
	sigterm.sa_flags |= SA_RESTART;
	::sigaction(SIGTERM, &sigterm, nullptr);
	::sigaction(SIGQUIT, &sigterm, nullptr);

	// Ignore SIGHUP/SIGINT signals.
	::signal(SIGHUP, SIG_IGN);
	::signal(SIGINT, SIG_IGN);

#else	// HAVE_SIGNAL_H

	m_pSigusr1Notifier = nullptr;
	m_pSigtermNotifier = nullptr;
	
#endif	// !HAVE_SIGNAL_H

#ifdef CONFIG_VOLUME
	// Make some extras into the toolbar...
	const QString& sVolumeText = tr("Master volume");
	m_iVolumeChanging = 0;
	// Volume slider...
	m_ui.channelsToolbar->addSeparator();
	m_pVolumeSlider = new QSlider(Qt::Horizontal, m_ui.channelsToolbar);
	m_pVolumeSlider->setTickPosition(QSlider::TicksBothSides);
	m_pVolumeSlider->setTickInterval(10);
	m_pVolumeSlider->setPageStep(10);
	m_pVolumeSlider->setSingleStep(10);
	m_pVolumeSlider->setMinimum(0);
	m_pVolumeSlider->setMaximum(100);
	m_pVolumeSlider->setMaximumHeight(26);
	m_pVolumeSlider->setMinimumWidth(160);
	m_pVolumeSlider->setToolTip(sVolumeText);
	QObject::connect(m_pVolumeSlider,
		SIGNAL(valueChanged(int)),
		SLOT(volumeChanged(int)));
	m_ui.channelsToolbar->addWidget(m_pVolumeSlider);
	// Volume spin-box
	m_ui.channelsToolbar->addSeparator();
	m_pVolumeSpinBox = new QSpinBox(m_ui.channelsToolbar);
	m_pVolumeSpinBox->setSuffix(" %");
	m_pVolumeSpinBox->setMinimum(0);
	m_pVolumeSpinBox->setMaximum(100);
	m_pVolumeSpinBox->setToolTip(sVolumeText);
	QObject::connect(m_pVolumeSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(volumeChanged(int)));
	m_ui.channelsToolbar->addWidget(m_pVolumeSpinBox);
#endif

	// Make it an MDI workspace.
	m_pWorkspace = new Workspace(this);
	m_pWorkspace->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_pWorkspace->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	// Set the activation connection.
	QObject::connect(m_pWorkspace,
		SIGNAL(subWindowActivated(QMdiSubWindow *)),
		SLOT(activateStrip(QMdiSubWindow *)));
	// Make it shine :-)
	setCentralWidget(m_pWorkspace);

	// Create some statusbar labels...
	QLabel *pLabel;
	// Client status.
	pLabel = new QLabel(tr("Connected"), this);
	pLabel->setAlignment(Qt::AlignLeft);
	pLabel->setMinimumSize(pLabel->sizeHint());
	m_statusItem[QSAMPLER_STATUS_CLIENT] = pLabel;
	statusBar()->addWidget(pLabel);
	// Server address.
	pLabel = new QLabel(this);
	pLabel->setAlignment(Qt::AlignLeft);
	m_statusItem[QSAMPLER_STATUS_SERVER] = pLabel;
	statusBar()->addWidget(pLabel, 1);
	// Channel title.
	pLabel = new QLabel(this);
	pLabel->setAlignment(Qt::AlignLeft);
	m_statusItem[QSAMPLER_STATUS_CHANNEL] = pLabel;
	statusBar()->addWidget(pLabel, 2);
	// Session modification status.
	pLabel = new QLabel(tr("MOD"), this);
	pLabel->setAlignment(Qt::AlignHCenter);
	pLabel->setMinimumSize(pLabel->sizeHint());
	m_statusItem[QSAMPLER_STATUS_SESSION] = pLabel;
	statusBar()->addWidget(pLabel);

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
	WSAStartup(MAKEWORD(1, 1), &_wsaData);
#endif

	// Some actions surely need those
	// shortcuts firmly attached...
	addAction(m_ui.viewMenubarAction);
	addAction(m_ui.viewToolbarAction);

	QObject::connect(m_ui.fileNewAction,
		SIGNAL(triggered()),
		SLOT(fileNew()));
	QObject::connect(m_ui.fileOpenAction,
		SIGNAL(triggered()),
		SLOT(fileOpen()));
	QObject::connect(m_ui.fileSaveAction,
		SIGNAL(triggered()),
		SLOT(fileSave()));
	QObject::connect(m_ui.fileSaveAsAction,
		SIGNAL(triggered()),
		SLOT(fileSaveAs()));
	QObject::connect(m_ui.fileResetAction,
		SIGNAL(triggered()),
		SLOT(fileReset()));
	QObject::connect(m_ui.fileRestartAction,
		SIGNAL(triggered()),
		SLOT(fileRestart()));
	QObject::connect(m_ui.fileExitAction,
		SIGNAL(triggered()),
		SLOT(fileExit()));
	QObject::connect(m_ui.editAddChannelAction,
		SIGNAL(triggered()),
		SLOT(editAddChannel()));
	QObject::connect(m_ui.editRemoveChannelAction,
		SIGNAL(triggered()),
		SLOT(editRemoveChannel()));
	QObject::connect(m_ui.editSetupChannelAction,
		SIGNAL(triggered()),
		SLOT(editSetupChannel()));
	QObject::connect(m_ui.editEditChannelAction,
		SIGNAL(triggered()),
		SLOT(editEditChannel()));
	QObject::connect(m_ui.editResetChannelAction,
		SIGNAL(triggered()),
		SLOT(editResetChannel()));
	QObject::connect(m_ui.editResetAllChannelsAction,
		SIGNAL(triggered()),
		SLOT(editResetAllChannels()));
	QObject::connect(m_ui.viewMenubarAction,
		SIGNAL(toggled(bool)),
		SLOT(viewMenubar(bool)));
	QObject::connect(m_ui.viewToolbarAction,
		SIGNAL(toggled(bool)),
		SLOT(viewToolbar(bool)));
	QObject::connect(m_ui.viewStatusbarAction,
		SIGNAL(toggled(bool)),
		SLOT(viewStatusbar(bool)));
	QObject::connect(m_ui.viewMessagesAction,
		SIGNAL(toggled(bool)),
		SLOT(viewMessages(bool)));
	QObject::connect(m_ui.viewInstrumentsAction,
		SIGNAL(triggered()),
		SLOT(viewInstruments()));
	QObject::connect(m_ui.viewDevicesAction,
		SIGNAL(triggered()),
		SLOT(viewDevices()));
	QObject::connect(m_ui.viewOptionsAction,
		SIGNAL(triggered()),
		SLOT(viewOptions()));
	QObject::connect(m_ui.channelsArrangeAction,
		SIGNAL(triggered()),
		SLOT(channelsArrange()));
	QObject::connect(m_ui.channelsAutoArrangeAction,
		SIGNAL(toggled(bool)),
		SLOT(channelsAutoArrange(bool)));
	QObject::connect(m_ui.helpAboutAction,
		SIGNAL(triggered()),
		SLOT(helpAbout()));
	QObject::connect(m_ui.helpAboutQtAction,
		SIGNAL(triggered()),
		SLOT(helpAboutQt()));

	QObject::connect(m_ui.fileMenu,
		SIGNAL(aboutToShow()),
		SLOT(updateRecentFilesMenu()));
	QObject::connect(m_ui.channelsMenu,
		SIGNAL(aboutToShow()),
		SLOT(channelsMenuAboutToShow()));
#ifdef CONFIG_VOLUME
	QObject::connect(m_ui.channelsToolbar,
		SIGNAL(orientationChanged(Qt::Orientation)),
		SLOT(channelsToolbarOrientation(Qt::Orientation)));
#endif
}

// Destructor.
MainForm::~MainForm()
{
	// Do final processing anyway.
	processServerExit();

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
	WSACleanup();
#endif

#if defined(HAVE_SIGNAL_H) && defined(HAVE_SYS_SOCKET_H)
	if (m_pSigusr1Notifier)
		delete m_pSigusr1Notifier;
	if (m_pSigtermNotifier)
		delete m_pSigtermNotifier;
#endif

	// Finally drop any widgets around...
	if (m_pDeviceForm)
		delete m_pDeviceForm;
	if (m_pInstrumentListForm)
		delete m_pInstrumentListForm;
	if (m_pMessages)
		delete m_pMessages;
	if (m_pWorkspace)
		delete m_pWorkspace;

	// Delete status item labels one by one.
	if (m_statusItem[QSAMPLER_STATUS_CLIENT])
		delete m_statusItem[QSAMPLER_STATUS_CLIENT];
	if (m_statusItem[QSAMPLER_STATUS_SERVER])
		delete m_statusItem[QSAMPLER_STATUS_SERVER];
	if (m_statusItem[QSAMPLER_STATUS_CHANNEL])
		delete m_statusItem[QSAMPLER_STATUS_CHANNEL];
	if (m_statusItem[QSAMPLER_STATUS_SESSION])
		delete m_statusItem[QSAMPLER_STATUS_SESSION];

#ifdef CONFIG_VOLUME
	delete m_pVolumeSpinBox;
	delete m_pVolumeSlider;
#endif

	// Pseudo-singleton reference shut-down.
	g_pMainForm = nullptr;
}


// Make and set a proper setup options step.
void MainForm::setup ( Options *pOptions )
{
	// We got options?
	m_pOptions = pOptions;

	// What style do we create these forms?
	Qt::WindowFlags wflags = Qt::Window
		| Qt::CustomizeWindowHint
		| Qt::WindowTitleHint
		| Qt::WindowSystemMenuHint
		| Qt::WindowMinMaxButtonsHint
		| Qt::WindowCloseButtonHint;
	if (m_pOptions->bKeepOnTop)
		wflags |= Qt::Tool;

	// Some child forms are to be created right now.
	m_pMessages = new Messages(this);
	m_pDeviceForm = new DeviceForm(this, wflags);
#ifdef CONFIG_MIDI_INSTRUMENT
	m_pInstrumentListForm = new InstrumentListForm(this, wflags);
#else
	m_ui.viewInstrumentsAction->setEnabled(false);
#endif

	// Setup messages logging appropriately...
	m_pMessages->setLogging(
		m_pOptions->bMessagesLog,
		m_pOptions->sMessagesLogPath);

	// Set message defaults...
	updateMessagesFont();
	updateMessagesLimit();
	updateMessagesCapture();

	// Set the visibility signal.
	QObject::connect(m_pMessages,
		SIGNAL(visibilityChanged(bool)),
		SLOT(stabilizeForm()));

	// Initial decorations toggle state.
	m_ui.viewMenubarAction->setChecked(m_pOptions->bMenubar);
	m_ui.viewToolbarAction->setChecked(m_pOptions->bToolbar);
	m_ui.viewStatusbarAction->setChecked(m_pOptions->bStatusbar);
	m_ui.channelsAutoArrangeAction->setChecked(m_pOptions->bAutoArrange);

	// Initial decorations visibility state.
	viewMenubar(m_pOptions->bMenubar);
	viewToolbar(m_pOptions->bToolbar);
	viewStatusbar(m_pOptions->bStatusbar);

	addDockWidget(Qt::BottomDockWidgetArea, m_pMessages);

	// Restore whole dock windows state.
	QByteArray aDockables = m_pOptions->settings().value(
		"/Layout/DockWindows").toByteArray();
	if (!aDockables.isEmpty()) {
		restoreState(aDockables);
	}

	// Try to restore old window positioning and initial visibility.
	m_pOptions->loadWidgetGeometry(this, true);
	m_pOptions->loadWidgetGeometry(m_pInstrumentListForm);
	m_pOptions->loadWidgetGeometry(m_pDeviceForm);

	// Final startup stabilization...
	updateMaxVolume();
	updateRecentFilesMenu();
	stabilizeForm();

	// Make it ready :-)
	statusBar()->showMessage(tr("Ready"), 3000);

	// We'll try to start immediately...
	startSchedule(0);

	// Register the first timer slot.
	QTimer::singleShot(QSAMPLER_TIMER_MSECS, this, SLOT(timerSlot()));
}


// Window close event handlers.
bool MainForm::queryClose (void)
{
	bool bQueryClose = closeSession(false);

	// Try to save current general state...
	if (m_pOptions) {
		// Some windows default fonts is here on demand too.
		if (bQueryClose && m_pMessages)
			m_pOptions->sMessagesFont = m_pMessages->messagesFont().toString();
		// Try to save current positioning.
		if (bQueryClose) {
			// Save decorations state.
			m_pOptions->bMenubar = m_ui.MenuBar->isVisible();
			m_pOptions->bToolbar = (m_ui.fileToolbar->isVisible()
				|| m_ui.editToolbar->isVisible()
				|| m_ui.channelsToolbar->isVisible());
			m_pOptions->bStatusbar = statusBar()->isVisible();
			// Save the dock windows state.
			m_pOptions->settings().setValue("/Layout/DockWindows", saveState());
			// And the children, and the main windows state,.
			m_pOptions->saveWidgetGeometry(m_pDeviceForm);
			m_pOptions->saveWidgetGeometry(m_pInstrumentListForm);
			m_pOptions->saveWidgetGeometry(this, true);
			// Close popup widgets.
			if (m_pInstrumentListForm)
				m_pInstrumentListForm->close();
			if (m_pDeviceForm)
				m_pDeviceForm->close();
			// Stop client and/or server, gracefully.
			stopServer(true /*interactive*/);
		}
	}

	return bQueryClose;
}


void MainForm::closeEvent ( QCloseEvent *pCloseEvent )
{
	if (queryClose()) {
		DeviceStatusForm::deleteAllInstances();
		pCloseEvent->accept();
	} else
		pCloseEvent->ignore();
}


// Window drag-n-drop event handlers.
void MainForm::dragEnterEvent ( QDragEnterEvent* pDragEnterEvent )
{
	// Accept external drags only...
	if (pDragEnterEvent->source() == nullptr
		&& pDragEnterEvent->mimeData()->hasUrls()) {
		pDragEnterEvent->accept();
	} else {
		pDragEnterEvent->ignore();
	}
}


void MainForm::dropEvent ( QDropEvent *pDropEvent )
{
	// Accept externally originated drops only...
	if (pDropEvent->source())
		return;

	const QMimeData *pMimeData = pDropEvent->mimeData();
	if (pMimeData->hasUrls()) {
		QListIterator<QUrl> iter(pMimeData->urls());
		while (iter.hasNext()) {
			const QString& sPath = iter.next().toLocalFile();
		//	if (Channel::isDlsInstrumentFile(sPath)) {
			if (QFileInfo(sPath).exists()) {
				// Try to create a new channel from instrument file...
				Channel *pChannel = new Channel();
				if (pChannel == nullptr)
					return;
				// Start setting the instrument filename...
				pChannel->setInstrument(sPath, 0);
				// Before we show it up, may be we'll
				// better ask for some initial values?
				if (!pChannel->channelSetup(this)) {
					delete pChannel;
					return;
				}
				// Finally, give it to a new channel strip...
				if (!createChannelStrip(pChannel)) {
					delete pChannel;
					return;
				}
				// Make that an overall update.
				m_iDirtyCount++;
				stabilizeForm();
			}   // Otherwise, load an usual session file (LSCP script)...
			else if (closeSession(true)) {
				loadSessionFile(sPath);
				break;
			}
		}
		// Make it look responsive...:)
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}


// Custome event handler.
void MainForm::customEvent ( QEvent* pEvent )
{
	// For the time being, just pump it to messages.
	if (pEvent->type() == QSAMPLER_LSCP_EVENT) {
		LscpEvent *pLscpEvent = static_cast<LscpEvent *> (pEvent);
		switch (pLscpEvent->event()) {
			case LSCP_EVENT_CHANNEL_COUNT:
				updateAllChannelStrips(true);
				break;
			case LSCP_EVENT_CHANNEL_INFO: {
				const int iChannelID = pLscpEvent->data().toInt();
				ChannelStrip *pChannelStrip = channelStrip(iChannelID);
				if (pChannelStrip)
					channelStripChanged(pChannelStrip);
				break;
			}
			case LSCP_EVENT_MIDI_INPUT_DEVICE_COUNT:
				if (m_pDeviceForm) m_pDeviceForm->refreshDevices();
				DeviceStatusForm::onDevicesChanged();
				updateViewMidiDeviceStatusMenu();
				break;
			case LSCP_EVENT_MIDI_INPUT_DEVICE_INFO: {
				if (m_pDeviceForm) m_pDeviceForm->refreshDevices();
				const int iDeviceID = pLscpEvent->data().section(' ', 0, 0).toInt();
				DeviceStatusForm::onDeviceChanged(iDeviceID);
				break;
			}
			case LSCP_EVENT_AUDIO_OUTPUT_DEVICE_COUNT:
				if (m_pDeviceForm) m_pDeviceForm->refreshDevices();
				break;
			case LSCP_EVENT_AUDIO_OUTPUT_DEVICE_INFO:
				if (m_pDeviceForm) m_pDeviceForm->refreshDevices();
				break;
		#if CONFIG_EVENT_CHANNEL_MIDI
			case LSCP_EVENT_CHANNEL_MIDI: {
				const int iChannelID = pLscpEvent->data().section(' ', 0, 0).toInt();
				ChannelStrip *pChannelStrip = channelStrip(iChannelID);
				if (pChannelStrip)
					pChannelStrip->midiActivityLedOn();
				break;
			}
		#endif
		#if CONFIG_EVENT_DEVICE_MIDI
			case LSCP_EVENT_DEVICE_MIDI: {
				const int iDeviceID = pLscpEvent->data().section(' ', 0, 0).toInt();
				const int iPortID   = pLscpEvent->data().section(' ', 1, 1).toInt();
				DeviceStatusForm *pDeviceStatusForm
					= DeviceStatusForm::getInstance(iDeviceID);
				if (pDeviceStatusForm)
					pDeviceStatusForm->midiArrived(iPortID);
				break;
			}
		#endif
			default:
				appendMessagesColor(tr("LSCP Event: %1 data: %2")
					.arg(::lscp_event_to_text(pLscpEvent->event()))
					.arg(pLscpEvent->data()), "#996699");
		}
	}
}


// LADISH Level 1 -- SIGUSR1 signal handler.
void MainForm::handle_sigusr1 (void)
{
#if defined(HAVE_SIGNAL_H) && defined(HAVE_SYS_SOCKET_H)

	char c;

	if (::read(g_fdSigusr1[1], &c, sizeof(c)) > 0)
		saveSession(false);

#endif
}


void MainForm::handle_sigterm (void)
{
#if defined(HAVE_SIGNAL_H) && defined(HAVE_SYS_SOCKET_H)

	char c;

	if (::read(g_fdSigterm[1], &c, sizeof(c)) > 0)
		close();

#endif
}


void MainForm::updateViewMidiDeviceStatusMenu (void)
{
	m_ui.viewMidiDeviceStatusMenu->clear();
	const std::map<int, DeviceStatusForm *> statusForms
		= DeviceStatusForm::getInstances();
	std::map<int, DeviceStatusForm *>::const_iterator iter
		= statusForms.begin();
	for ( ; iter != statusForms.end(); ++iter) {
		DeviceStatusForm *pStatusForm = iter->second;
		m_ui.viewMidiDeviceStatusMenu->addAction(
			pStatusForm->visibleAction());
	}
}


// Context menu event handler.
void MainForm::contextMenuEvent( QContextMenuEvent *pEvent )
{
	stabilizeForm();

	m_ui.editMenu->exec(pEvent->globalPos());
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Brainless public property accessors.

// The global options settings property.
Options *MainForm::options (void) const
{
	return m_pOptions;
}


// The LSCP client descriptor property.
lscp_client_t *MainForm::client (void) const
{
	return m_pClient;
}


// The pseudo-singleton instance accessor.
MainForm *MainForm::getInstance (void)
{
	return g_pMainForm;
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Session file stuff.

// Format the displayable session filename.
QString MainForm::sessionName ( const QString& sFilename )
{
	const bool bCompletePath = (m_pOptions && m_pOptions->bCompletePath);
	QString sSessionName = sFilename;
	if (sSessionName.isEmpty())
		sSessionName = tr("Untitled") + QString::number(m_iUntitled);
	else if (!bCompletePath)
		sSessionName = QFileInfo(sSessionName).fileName();
	return sSessionName;
}


// Create a new session file from scratch.
bool MainForm::newSession (void)
{
	// Check if we can do it.
	if (!closeSession(true))
		return false;

	// Give us what the server has, right now...
	updateSession();

	// Ok increment untitled count.
	m_iUntitled++;

	// Stabilize form.
	m_sFilename = QString();
	m_iDirtyCount = 0;
	appendMessages(tr("New session: \"%1\".").arg(sessionName(m_sFilename)));
	stabilizeForm();

	return true;
}


// Open an existing sampler session.
bool MainForm::openSession (void)
{
	if (m_pOptions == nullptr)
		return false;

	// Ask for the filename to open...
	QString sFilename = QFileDialog::getOpenFileName(this,
		tr("Open Session"),                       // Caption.
		m_pOptions->sSessionDir,                  // Start here.
		tr("LSCP Session files") + " (*.lscp)"    // Filter (LSCP files)
	);

	// Have we cancelled?
	if (sFilename.isEmpty())
		return false;

	// Check if we're going to discard safely the current one...
	if (!closeSession(true))
		return false;

	// Load it right away.
	return loadSessionFile(sFilename);
}


// Save current sampler session with another name.
bool MainForm::saveSession ( bool bPrompt )
{
	if (m_pOptions == nullptr)
		return false;

	QString sFilename = m_sFilename;

	// Ask for the file to save, if there's none...
	if (bPrompt || sFilename.isEmpty()) {
		// If none is given, assume default directory.
		if (sFilename.isEmpty())
			sFilename = m_pOptions->sSessionDir;
		// Prompt the guy...
		sFilename = QFileDialog::getSaveFileName(this,
			tr("Save Session"),                       // Caption.
			sFilename,                                // Start here.
			tr("LSCP Session files") + " (*.lscp)"    // Filter (LSCP files)
		);
		// Have we cancelled it?
		if (sFilename.isEmpty())
			return false;
		// Enforce .lscp extension...
		if (QFileInfo(sFilename).suffix().isEmpty())
			sFilename += ".lscp";
	#if 0
		// Check if already exists...
		if (sFilename != m_sFilename && QFileInfo(sFilename).exists()) {
			if (QMessageBox::warning(this,
				tr("Warning"),
				tr("The file already exists:\n\n"
				"\"%1\"\n\n"
				"Do you want to replace it?")
				.arg(sFilename),
				QMessageBox::Yes | QMessageBox::No)
				== QMessageBox::No)
				return false;
		}
	#endif
	}

	// Save it right away.
	return saveSessionFile(sFilename);
}


// Close current session.
bool MainForm::closeSession ( bool bForce )
{
	bool bClose = true;

	// Are we dirty enough to prompt it?
	if (m_iDirtyCount > 0) {
		switch (QMessageBox::warning(this,
			tr("Warning"),
			tr("The current session has been changed:\n\n"
			"\"%1\"\n\n"
			"Do you want to save the changes?")
			.arg(sessionName(m_sFilename)),
			QMessageBox::Save |
			QMessageBox::Discard |
			QMessageBox::Cancel)) {
		case QMessageBox::Save:
			bClose = saveSession(false);
			// Fall thru....
		case QMessageBox::Discard:
			break;
		default:    // Cancel.
			bClose = false;
			break;
		}
	}

	// If we may close it, dot it.
	if (bClose) {
		// Remove all channel strips from sight...
		m_pWorkspace->setUpdatesEnabled(false);
		const QList<QMdiSubWindow *>& wlist
			= m_pWorkspace->subWindowList();
		foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
			ChannelStrip *pChannelStrip
				= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
			if (pChannelStrip) {
				Channel *pChannel = pChannelStrip->channel();
				if (bForce && pChannel)
					pChannel->removeChannel();
				delete pChannelStrip;
			}
			delete pMdiSubWindow;
		}
		m_pWorkspace->setUpdatesEnabled(true);
		// We're now clean, for sure.
		m_iDirtyCount = 0;
	}

	return bClose;
}


// Load a session from specific file path.
bool MainForm::loadSessionFile ( const QString& sFilename )
{
	if (m_pClient == nullptr)
		return false;

	// Open and read from real file.
	QFile file(sFilename);
	if (!file.open(QIODevice::ReadOnly)) {
		appendMessagesError(
			tr("Could not open \"%1\" session file.\n\nSorry.")
			.arg(sFilename));
		return false;
	}

	// Tell the world we'll take some time...
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	// Read the file.
	int iLine = 0;
	int iErrors = 0;
	QTextStream ts(&file);
	while (!ts.atEnd()) {
		// Read the line.
		QString sCommand = ts.readLine().trimmed();
		iLine++;
		// If not empty, nor a comment, call the server...
		if (!sCommand.isEmpty() && sCommand[0] != '#') {
			// Remember that, no matter what,
			// all LSCP commands are CR/LF terminated.
			sCommand += "\r\n";
			if (::lscp_client_query(m_pClient, sCommand.toUtf8().constData())
				!= LSCP_OK) {
				appendMessagesColor(QString("%1(%2): %3")
					.arg(QFileInfo(sFilename).fileName()).arg(iLine)
					.arg(sCommand.simplified()), "#996633");
				appendMessagesClient("lscp_client_query");
				iErrors++;
			}
		}
		// Try to make it snappy :)
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	// Ok. we've read it.
	file.close();

	// Now we'll try to create (update) the whole GUI session.
	updateSession();

	// We're fornerly done.
	QApplication::restoreOverrideCursor();

	// Have we any errors?
	if (iErrors > 0) {
		appendMessagesError(
			tr("Session loaded with errors\nfrom \"%1\".\n\nSorry.")
			.arg(sFilename));
	}

	// Save as default session directory.
	if (m_pOptions)
		m_pOptions->sSessionDir = QFileInfo(sFilename).dir().absolutePath();
	// We're not dirty anymore, if loaded without errors,
	m_iDirtyCount = iErrors;
	// Stabilize form...
	m_sFilename = sFilename;
	updateRecentFiles(sFilename);
	appendMessages(tr("Open session: \"%1\".").arg(sessionName(m_sFilename)));

	// Make that an overall update.
	stabilizeForm();
	return true;
}


// Save current session to specific file path.
bool MainForm::saveSessionFile ( const QString& sFilename )
{
	if (m_pClient == nullptr)
		return false;

	// Check whether server is apparently OK...
	if (::lscp_get_channels(m_pClient) < 0) {
		appendMessagesClient("lscp_get_channels");
		return false;
	}

	// Open and write into real file.
	QFile file(sFilename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		appendMessagesError(
			tr("Could not open \"%1\" session file.\n\nSorry.")
			.arg(sFilename));
		return false;
	}

	// Tell the world we'll take some time...
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	// Write the file.
	int iErrors = 0;
	QTextStream ts(&file);
	ts << "# " << QSAMPLER_TITLE " - " << tr(QSAMPLER_SUBTITLE) << Qt::endl;
	ts << "# " << tr("Version") << ": " CONFIG_BUILD_VERSION << Qt::endl;
//	ts << "# " << tr("Build") << ": " CONFIG_BUILD_DATE << Qt::endl;
	ts << "#"  << Qt::endl;
	ts << "# " << tr("File")
	<< ": " << QFileInfo(sFilename).fileName() << Qt::endl;
	ts << "# " << tr("Date")
	<< ": " << QDate::currentDate().toString("MMM dd yyyy")
	<< " "  << QTime::currentTime().toString("hh:mm:ss") << Qt::endl;
	ts << "#"  << Qt::endl;
	ts << Qt::endl;

	// It is assumed that this new kind of device+session file
	// will be loaded from a complete initialized server...
	int *piDeviceIDs;
	int  i, iDevice;
	ts << "RESET" << Qt::endl;

	// Audio device mapping.
	QMap<int, int> audioDeviceMap; iDevice = 0;
	piDeviceIDs = Device::getDevices(m_pClient, Device::Audio);
	for (i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; ++i) {
		Device device(Device::Audio, piDeviceIDs[i]);
		// Avoid plug-in driver devices...
		if (device.driverName().toUpper() == "PLUGIN")
			continue;
		// Audio device specification...
		ts << Qt::endl;
		ts << "# " << device.deviceTypeName() << " " << device.driverName()
			<< " " << tr("Device") << " " << iDevice << Qt::endl;
		ts << "CREATE AUDIO_OUTPUT_DEVICE " << device.driverName();
		DeviceParamMap::ConstIterator deviceParam;
		for (deviceParam = device.params().begin();
				deviceParam != device.params().end();
					++deviceParam) {
			const DeviceParam& param = deviceParam.value();
			if (param.value.isEmpty()) ts << "# ";
			ts << " " << deviceParam.key() << "='" << param.value << "'";
		}
		ts << Qt::endl;
		// Audio channel parameters...
		int iPort = 0;
		QListIterator<DevicePort *> iter(device.ports());
		while (iter.hasNext()) {
			DevicePort *pPort = iter.next();
			DeviceParamMap::ConstIterator portParam;
			for (portParam = pPort->params().begin();
					portParam != pPort->params().end();
						++portParam) {
				const DeviceParam& param = portParam.value();
				if (param.fix || param.value.isEmpty()) ts << "# ";
				ts << "SET AUDIO_OUTPUT_CHANNEL_PARAMETER " << iDevice
					<< " " << iPort << " " << portParam.key()
					<< "='" << param.value << "'" << Qt::endl;
			}
			iPort++;
		}
		// Audio device index/id mapping.
		audioDeviceMap.insert(device.deviceID(), iDevice++);
		// Try to keep it snappy :)
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	// MIDI device mapping.
	QMap<int, int> midiDeviceMap; iDevice = 0;
	piDeviceIDs = Device::getDevices(m_pClient, Device::Midi);
	for (i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; ++i) {
		Device device(Device::Midi, piDeviceIDs[i]);
		// Avoid plug-in driver devices...
		if (device.driverName().toUpper() == "PLUGIN")
			continue;
		// MIDI device specification...
		ts << Qt::endl;
		ts << "# " << device.deviceTypeName() << " " << device.driverName()
			<< " " << tr("Device") << " " << iDevice << Qt::endl;
		ts << "CREATE MIDI_INPUT_DEVICE " << device.driverName();
		DeviceParamMap::ConstIterator deviceParam;
		for (deviceParam = device.params().begin();
				deviceParam != device.params().end();
					++deviceParam) {
			const DeviceParam& param = deviceParam.value();
			if (param.value.isEmpty()) ts << "# ";
			ts << " " << deviceParam.key() << "='" << param.value << "'";
		}
		ts << Qt::endl;
		// MIDI port parameters...
		int iPort = 0;
		QListIterator<DevicePort *> iter(device.ports());
		while (iter.hasNext()) {
			DevicePort *pPort = iter.next();
			DeviceParamMap::ConstIterator portParam;
			for (portParam = pPort->params().begin();
					portParam != pPort->params().end();
						++portParam) {
				const DeviceParam& param = portParam.value();
				if (param.fix || param.value.isEmpty()) ts << "# ";
				ts << "SET MIDI_INPUT_PORT_PARAMETER " << iDevice
				<< " " << iPort << " " << portParam.key()
				<< "='" << param.value << "'" << Qt::endl;
			}
			iPort++;
		}
		// MIDI device index/id mapping.
		midiDeviceMap.insert(device.deviceID(), iDevice++);
		// Try to keep it snappy :)
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	ts << Qt::endl;

#ifdef CONFIG_MIDI_INSTRUMENT
	// MIDI instrument mapping...
	QMap<int, int> midiInstrumentMap;
	int *piMaps = ::lscp_list_midi_instrument_maps(m_pClient);
	for (int iMap = 0; piMaps && piMaps[iMap] >= 0; iMap++) {
		const int iMidiMap = piMaps[iMap];
		const char *pszMapName
			= ::lscp_get_midi_instrument_map_name(m_pClient, iMidiMap);
		ts << "# " << tr("MIDI instrument map") << " " << iMap;
		if (pszMapName)
			ts << " - " << pszMapName;
		ts << Qt::endl;
		ts << "ADD MIDI_INSTRUMENT_MAP";
		if (pszMapName)
			ts << " '" << pszMapName << "'";
		ts << Qt::endl;
		// MIDI instrument mapping...
		lscp_midi_instrument_t *pInstrs
			= ::lscp_list_midi_instruments(m_pClient, iMidiMap);
		for (int iInstr = 0; pInstrs && pInstrs[iInstr].map >= 0; iInstr++) {
			lscp_midi_instrument_info_t *pInstrInfo
				= ::lscp_get_midi_instrument_info(m_pClient, &pInstrs[iInstr]);
			if (pInstrInfo) {
				ts << "MAP MIDI_INSTRUMENT "
					<< iMap                        << " "
					<< pInstrs[iInstr].bank        << " "
					<< pInstrs[iInstr].prog        << " "
					<< pInstrInfo->engine_name     << " '"
					<< pInstrInfo->instrument_file << "' "
					<< pInstrInfo->instrument_nr   << " "
					<< pInstrInfo->volume          << " ";
				switch (pInstrInfo->load_mode) {
					case LSCP_LOAD_PERSISTENT:
						ts << "PERSISTENT";
						break;
					case LSCP_LOAD_ON_DEMAND_HOLD:
						ts << "ON_DEMAND_HOLD";
						break;
					case LSCP_LOAD_ON_DEMAND:
					case LSCP_LOAD_DEFAULT:
					default:
						ts << "ON_DEMAND";
						break;
				}
				if (pInstrInfo->name)
					ts << " '" << pInstrInfo->name << "'";
				ts << Qt::endl;
			}	// Check for errors...
			else if (::lscp_client_get_errno(m_pClient)) {
				appendMessagesClient("lscp_get_midi_instrument_info");
				iErrors++;
			}
			// Try to keep it snappy :)
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		}
		ts << Qt::endl;
		// Check for errors...
		if (pInstrs == nullptr && ::lscp_client_get_errno(m_pClient)) {
			appendMessagesClient("lscp_list_midi_instruments");
			iErrors++;
		}
		// MIDI strument index/id mapping.
		midiInstrumentMap.insert(iMidiMap, iMap);
	}
	// Check for errors...
	if (piMaps == nullptr && ::lscp_client_get_errno(m_pClient)) {
		appendMessagesClient("lscp_list_midi_instrument_maps");
		iErrors++;
	}
#endif	// CONFIG_MIDI_INSTRUMENT

	// Sampler channel mapping...
	int iChannelID = 0;
	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
		ChannelStrip *pChannelStrip
			= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
		if (pChannelStrip) {
			Channel *pChannel = pChannelStrip->channel();
			if (pChannel) {
				// Avoid "artifial" plug-in devices...
				const int iAudioDevice = pChannel->audioDevice();
				if (!audioDeviceMap.contains(iAudioDevice))
					continue;
				const int iMidiDevice = pChannel->midiDevice();
				if (!midiDeviceMap.contains(iMidiDevice))
					continue;
				// Go for regular, canonical devices...
				ts << "# " << tr("Channel") << " " << iChannelID << Qt::endl;
				ts << "ADD CHANNEL" << Qt::endl;
				if (audioDeviceMap.isEmpty()) {
					ts << "SET CHANNEL AUDIO_OUTPUT_TYPE " << iChannelID
						<< " " << pChannel->audioDriver() << Qt::endl;
				} else {
					ts << "SET CHANNEL AUDIO_OUTPUT_DEVICE " << iChannelID
						<< " " << audioDeviceMap.value(iAudioDevice) << Qt::endl;
				}
				if (midiDeviceMap.isEmpty()) {
					ts << "SET CHANNEL MIDI_INPUT_TYPE " << iChannelID
						<< " " << pChannel->midiDriver() << Qt::endl;
				} else {
					ts << "SET CHANNEL MIDI_INPUT_DEVICE " << iChannelID
						<< " " << midiDeviceMap.value(iMidiDevice) << Qt::endl;
				}
				ts << "SET CHANNEL MIDI_INPUT_PORT " << iChannelID
					<< " " << pChannel->midiPort() << Qt::endl;
				ts << "SET CHANNEL MIDI_INPUT_CHANNEL " << iChannelID << " ";
				if (pChannel->midiChannel() == LSCP_MIDI_CHANNEL_ALL)
					ts << "ALL";
				else
					ts << pChannel->midiChannel();
				ts << Qt::endl;
				ts << "LOAD ENGINE " << pChannel->engineName()
					<< " " << iChannelID << Qt::endl;
				if (pChannel->instrumentStatus() < 100) ts << "# ";
				ts << "LOAD INSTRUMENT NON_MODAL '"
					<< pChannel->instrumentFile() << "' "
					<< pChannel->instrumentNr() << " " << iChannelID << Qt::endl;
				ChannelRoutingMap::ConstIterator audioRoute;
				for (audioRoute = pChannel->audioRouting().begin();
						audioRoute != pChannel->audioRouting().end();
							++audioRoute) {
					ts << "SET CHANNEL AUDIO_OUTPUT_CHANNEL " << iChannelID
						<< " " << audioRoute.key()
						<< " " << audioRoute.value() << Qt::endl;
				}
				ts << "SET CHANNEL VOLUME " << iChannelID
					<< " " << pChannel->volume() << Qt::endl;
				if (pChannel->channelMute())
					ts << "SET CHANNEL MUTE " << iChannelID << " 1" << Qt::endl;
				if (pChannel->channelSolo())
					ts << "SET CHANNEL SOLO " << iChannelID << " 1" << Qt::endl;
			#ifdef CONFIG_MIDI_INSTRUMENT
				const int iMidiMap = pChannel->midiMap();
				if (midiInstrumentMap.contains(iMidiMap)) {
					ts << "SET CHANNEL MIDI_INSTRUMENT_MAP " << iChannelID
						<< " " << midiInstrumentMap.value(iMidiMap) << Qt::endl;
				}
			#endif
			#ifdef CONFIG_FXSEND
				int *piFxSends = ::lscp_list_fxsends(m_pClient, iChannelID);
				for (int iFxSend = 0;
						piFxSends && piFxSends[iFxSend] >= 0;
							iFxSend++) {
					lscp_fxsend_info_t *pFxSendInfo	= ::lscp_get_fxsend_info(
						m_pClient, iChannelID, piFxSends[iFxSend]);
					if (pFxSendInfo) {
						ts << "CREATE FX_SEND " << iChannelID
							<< " " << pFxSendInfo->midi_controller;
						if (pFxSendInfo->name)
							ts << " '" << pFxSendInfo->name << "'";
						ts << Qt::endl;
						int *piRouting = pFxSendInfo->audio_routing;
						for (int iAudioSrc = 0;
								piRouting && piRouting[iAudioSrc] >= 0;
									iAudioSrc++) {
							ts << "SET FX_SEND AUDIO_OUTPUT_CHANNEL "
								<< iChannelID
								<< " " << iFxSend
								<< " " << iAudioSrc
								<< " " << piRouting[iAudioSrc] << Qt::endl;
						}
					#ifdef CONFIG_FXSEND_LEVEL
						ts << "SET FX_SEND LEVEL " << iChannelID
							<< " " << iFxSend
							<< " " << pFxSendInfo->level << Qt::endl;
					#endif
					}	// Check for errors...
					else if (::lscp_client_get_errno(m_pClient)) {
						appendMessagesClient("lscp_get_fxsend_info");
						iErrors++;
					}
				}
			#endif
				ts << Qt::endl;
				// Go for next channel...
				++iChannelID;
			}
		}
		// Try to keep it snappy :)
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

#ifdef CONFIG_VOLUME
	ts << "# " << tr("Global volume level") << Qt::endl;
	ts << "SET VOLUME " << ::lscp_get_volume(m_pClient) << Qt::endl;
	ts << Qt::endl;
#endif

	// Ok. we've wrote it.
	file.close();

	// We're fornerly done.
	QApplication::restoreOverrideCursor();

	// Have we any errors?
	if (iErrors > 0) {
		appendMessagesError(
			tr("Some settings could not be saved\n"
			"to \"%1\" session file.\n\nSorry.")
			.arg(sFilename));
	}

	// Save as default session directory.
	if (m_pOptions)
		m_pOptions->sSessionDir = QFileInfo(sFilename).dir().absolutePath();
	// We're not dirty anymore.
	m_iDirtyCount = 0;
	// Stabilize form...
	m_sFilename = sFilename;
	updateRecentFiles(sFilename);
	appendMessages(tr("Save session: \"%1\".").arg(sessionName(m_sFilename)));
	stabilizeForm();
	return true;
}


// Session change receiver slot.
void MainForm::sessionDirty (void)
{
	// Just mark the dirty form.
	m_iDirtyCount++;
	// and update the form status...
	stabilizeForm();
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- File Action slots.

// Create a new sampler session.
void MainForm::fileNew (void)
{
	// Of course we'll start clean new.
	newSession();
}


// Open an existing sampler session.
void MainForm::fileOpen (void)
{
	// Open it right away.
	openSession();
}


// Open a recent file session.
void MainForm::fileOpenRecent (void)
{
	// Retrive filename index from action data...
	QAction *pAction = qobject_cast<QAction *> (sender());
	if (pAction && m_pOptions) {
		const int iIndex = pAction->data().toInt();
		if (iIndex >= 0 && iIndex < m_pOptions->recentFiles.count()) {
			QString sFilename = m_pOptions->recentFiles[iIndex];
			// Check if we can safely close the current session...
			if (!sFilename.isEmpty() && closeSession(true))
				loadSessionFile(sFilename);
		}
	}
}


// Save current sampler session.
void MainForm::fileSave (void)
{
	// Save it right away.
	saveSession(false);
}


// Save current sampler session with another name.
void MainForm::fileSaveAs (void)
{
	// Save it right away, maybe with another name.
	saveSession(true);
}


// Reset the sampler instance.
void MainForm::fileReset (void)
{
	if (m_pClient == nullptr)
		return;

	// Ask user whether he/she want's an internal sampler reset...
	if (m_pOptions && m_pOptions->bConfirmReset) {
		const QString& sTitle = tr("Warning");
		const QString& sText = tr(
			"Resetting the sampler instance will close\n"
			"all device and channel configurations.\n\n"
			"Please note that this operation may cause\n"
			"temporary MIDI and Audio disruption.\n\n"
			"Do you want to reset the sampler engine now?");
	#if 0
		if (QMessageBox::warning(this, sTitle, sText,
			QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
			return;
	#else
		QMessageBox mbox(this);
		mbox.setIcon(QMessageBox::Warning);
		mbox.setWindowTitle(sTitle);
		mbox.setText(sText);
		mbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		QCheckBox cbox(tr("Don't ask this again"));
		cbox.setChecked(false);
		cbox.blockSignals(true);
		mbox.addButton(&cbox, QMessageBox::ActionRole);
		if (mbox.exec() == QMessageBox::Cancel)
			return;
		if (cbox.isChecked())
			m_pOptions->bConfirmReset = false;
	#endif
	}

	// Trye closing the current session, first...
	if (!closeSession(true))
		return;

	// Just do the reset, after closing down current session...
	// Do the actual sampler reset...
	if (::lscp_reset_sampler(m_pClient) != LSCP_OK) {
		appendMessagesClient("lscp_reset_sampler");
		appendMessagesError(tr("Could not reset sampler instance.\n\nSorry."));
		return;
	}

	// Log this.
	appendMessages(tr("Sampler reset."));

	// Make it a new session...
	newSession();
}


// Restart the client/server instance.
void MainForm::fileRestart (void)
{
	if (m_pOptions == nullptr)
		return;

	bool bRestart = true;

	// Ask user whether he/she want's a complete restart...
	// (if we're currently up and running)
	if (m_pOptions && m_pOptions->bConfirmRestart) {
		const QString& sTitle = tr("Warning");
		const QString& sText = tr(
			"New settings will be effective after\n"
			"restarting the client/server connection.\n\n"
			"Please note that this operation may cause\n"
			"temporary MIDI and Audio disruption.\n\n"
			"Do you want to restart the connection now?");
	#if 0
		if (QMessageBox::warning(this, sTitle, sText,
			QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
			bRestart = false;
	#else
		QMessageBox mbox(this);
		mbox.setIcon(QMessageBox::Warning);
		mbox.setWindowTitle(sTitle);
		mbox.setText(sText);
		mbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		QCheckBox cbox(tr("Don't ask this again"));
		cbox.setChecked(false);
		cbox.blockSignals(true);
		mbox.addButton(&cbox, QMessageBox::ActionRole);
		if (mbox.exec() == QMessageBox::Cancel)
			bRestart = false;
		else
		if (cbox.isChecked())
			m_pOptions->bConfirmRestart = false;
	#endif
	}

	// Are we still for it?
	if (bRestart && closeSession(true)) {
		// Stop server, it will force the client too.
		stopServer();
		// Reschedule a restart...
		startSchedule(m_pOptions->iStartDelay);
	}
}


// Exit application program.
void MainForm::fileExit (void)
{
	// Go for close the whole thing.
	close();
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Edit Action slots.

// Add a new sampler channel.
void MainForm::editAddChannel (void)
{
	++m_iDirtySetup;
	addChannelStrip();
	--m_iDirtySetup;
}

void MainForm::addChannelStrip (void)
{
	if (m_pClient == nullptr)
		return;

	// Just create the channel instance...
	Channel *pChannel = new Channel();
	if (pChannel == nullptr)
		return;

	// Before we show it up, may be we'll
	// better ask for some initial values?
	if (!pChannel->channelSetup(this)) {
		delete pChannel;
		return;
	}

	// And give it to the strip...
	// (will own the channel instance, if successful).
	if (!createChannelStrip(pChannel)) {
		delete pChannel;
		return;
	}

	// Do we auto-arrange?
	channelsArrangeAuto();

	// Make that an overall update.
	m_iDirtyCount++;
	stabilizeForm();
}


// Remove current sampler channel.
void MainForm::editRemoveChannel (void)
{
	++m_iDirtySetup;
	removeChannelStrip();
	--m_iDirtySetup;
}

void MainForm::removeChannelStrip (void)
{
	if (m_pClient == nullptr)
		return;

	ChannelStrip *pChannelStrip = activeChannelStrip();
	if (pChannelStrip == nullptr)
		return;

	Channel *pChannel = pChannelStrip->channel();
	if (pChannel == nullptr)
		return;

	// Prompt user if he/she's sure about this...
	if (m_pOptions && m_pOptions->bConfirmRemove) {
		const QString& sTitle = tr("Warning");
		const QString& sText = tr(
			"About to remove channel:\n\n"
			"%1\n\n"
			"Are you sure?")
			.arg(pChannelStrip->windowTitle());
	#if 0
		if (QMessageBox::warning(this, sTitle, sText,
			QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
			return;
	#else
		QMessageBox mbox(this);
		mbox.setIcon(QMessageBox::Warning);
		mbox.setWindowTitle(sTitle);
		mbox.setText(sText);
		mbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		QCheckBox cbox(tr("Don't ask this again"));
		cbox.setChecked(false);
		cbox.blockSignals(true);
		mbox.addButton(&cbox, QMessageBox::ActionRole);
		if (mbox.exec() == QMessageBox::Cancel)
			return;
		if (cbox.isChecked())
			m_pOptions->bConfirmRemove = false;
	#endif
	}

	// Remove the existing sampler channel.
	if (!pChannel->removeChannel())
		return;

	// Just delete the channel strip.
	destroyChannelStrip(pChannelStrip);

	// We'll be dirty, for sure...
	m_iDirtyCount++;
	stabilizeForm();
}


// Setup current sampler channel.
void MainForm::editSetupChannel (void)
{
	if (m_pClient == nullptr)
		return;

	ChannelStrip *pChannelStrip = activeChannelStrip();
	if (pChannelStrip == nullptr)
		return;

	// Just invoque the channel strip procedure.
	pChannelStrip->channelSetup();
}


// Edit current sampler channel.
void MainForm::editEditChannel (void)
{
	if (m_pClient == nullptr)
		return;

	ChannelStrip *pChannelStrip = activeChannelStrip();
	if (pChannelStrip == nullptr)
		return;

	// Just invoque the channel strip procedure.
	pChannelStrip->channelEdit();
}


// Reset current sampler channel.
void MainForm::editResetChannel (void)
{
	if (m_pClient == nullptr)
		return;

	ChannelStrip *pChannelStrip = activeChannelStrip();
	if (pChannelStrip == nullptr)
		return;

	// Just invoque the channel strip procedure.
	pChannelStrip->channelReset();
}


// Reset all sampler channels.
void MainForm::editResetAllChannels (void)
{
	if (m_pClient == nullptr)
		return;

	// Invoque the channel strip procedure,
	// for all channels out there...
	m_pWorkspace->setUpdatesEnabled(false);
	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
		ChannelStrip *pChannelStrip
			= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
		if (pChannelStrip)
			pChannelStrip->channelReset();
	}
	m_pWorkspace->setUpdatesEnabled(true);
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- View Action slots.

// Show/hide the main program window menubar.
void MainForm::viewMenubar ( bool bOn )
{
	if (bOn)
		m_ui.MenuBar->show();
	else
		m_ui.MenuBar->hide();
}


// Show/hide the main program window toolbar.
void MainForm::viewToolbar ( bool bOn )
{
	if (bOn) {
		m_ui.fileToolbar->show();
		m_ui.editToolbar->show();
		m_ui.channelsToolbar->show();
	} else {
		m_ui.fileToolbar->hide();
		m_ui.editToolbar->hide();
		m_ui.channelsToolbar->hide();
	}
}


// Show/hide the main program window statusbar.
void MainForm::viewStatusbar ( bool bOn )
{
	if (bOn)
		statusBar()->show();
	else
		statusBar()->hide();
}


// Show/hide the messages window logger.
void MainForm::viewMessages ( bool bOn )
{
	if (bOn)
		m_pMessages->show();
	else
		m_pMessages->hide();
}


// Show/hide the MIDI instrument list-view form.
void MainForm::viewInstruments (void)
{
	if (m_pOptions == nullptr)
		return;

	if (m_pInstrumentListForm) {
		m_pOptions->saveWidgetGeometry(m_pInstrumentListForm);
		if (m_pInstrumentListForm->isVisible()) {
			m_pInstrumentListForm->hide();
		} else {
			m_pInstrumentListForm->show();
			m_pInstrumentListForm->raise();
			m_pInstrumentListForm->activateWindow();
		}
	}
}


// Show/hide the device configurator form.
void MainForm::viewDevices (void)
{
	if (m_pOptions == nullptr)
		return;

	if (m_pDeviceForm) {
		m_pOptions->saveWidgetGeometry(m_pDeviceForm);
		if (m_pDeviceForm->isVisible()) {
			m_pDeviceForm->hide();
		} else {
			m_pDeviceForm->show();
			m_pDeviceForm->raise();
			m_pDeviceForm->activateWindow();
		}
	}
}


// Show options dialog.
void MainForm::viewOptions (void)
{
	if (m_pOptions == nullptr)
		return;

	OptionsForm* pOptionsForm = new OptionsForm(this);
	if (pOptionsForm) {
		// Check out some initial nullities(tm)...
		ChannelStrip *pChannelStrip = activeChannelStrip();
		if (m_pOptions->sDisplayFont.isEmpty() && pChannelStrip)
			m_pOptions->sDisplayFont = pChannelStrip->displayFont().toString();
		if (m_pOptions->sMessagesFont.isEmpty() && m_pMessages)
			m_pOptions->sMessagesFont = m_pMessages->messagesFont().toString();
		// To track down deferred or immediate changes.
		const QString sOldServerHost      = m_pOptions->sServerHost;
		const int     iOldServerPort      = m_pOptions->iServerPort;
		const int     iOldServerTimeout   = m_pOptions->iServerTimeout;
		const bool    bOldServerStart     = m_pOptions->bServerStart;
		const QString sOldServerCmdLine   = m_pOptions->sServerCmdLine;
		const bool    bOldMessagesLog     = m_pOptions->bMessagesLog;
		const QString sOldMessagesLogPath = m_pOptions->sMessagesLogPath;
		const QString sOldDisplayFont     = m_pOptions->sDisplayFont;
		const bool    bOldDisplayEffect   = m_pOptions->bDisplayEffect;
		const int     iOldMaxVolume       = m_pOptions->iMaxVolume;
		const QString sOldMessagesFont    = m_pOptions->sMessagesFont;
		const bool    bOldKeepOnTop       = m_pOptions->bKeepOnTop;
		const bool    bOldStdoutCapture   = m_pOptions->bStdoutCapture;
		const int     bOldMessagesLimit   = m_pOptions->bMessagesLimit;
		const int     iOldMessagesLimitLines = m_pOptions->iMessagesLimitLines;
		const bool    bOldCompletePath    = m_pOptions->bCompletePath;
		const bool    bOldInstrumentNames = m_pOptions->bInstrumentNames;
		const int     iOldMaxRecentFiles  = m_pOptions->iMaxRecentFiles;
		const int     iOldBaseFontSize    = m_pOptions->iBaseFontSize;
		// Load the current setup settings.
		pOptionsForm->setup(m_pOptions);
		// Show the setup dialog...
		if (pOptionsForm->exec()) {
			// Warn if something will be only effective on next run.
			if (( bOldStdoutCapture && !m_pOptions->bStdoutCapture) ||
				(!bOldStdoutCapture &&  m_pOptions->bStdoutCapture) ||
				( bOldKeepOnTop     && !m_pOptions->bKeepOnTop)     ||
				(!bOldKeepOnTop     &&  m_pOptions->bKeepOnTop)     ||
				(iOldBaseFontSize   !=  m_pOptions->iBaseFontSize)) {
				QMessageBox::information(this,
					tr("Information"),
					tr("Some settings may be only effective\n"
					"next time you start this program."));
				updateMessagesCapture();
			}
			// Check wheather something immediate has changed.
			if (( bOldMessagesLog && !m_pOptions->bMessagesLog) ||
				(!bOldMessagesLog &&  m_pOptions->bMessagesLog) ||
				(sOldMessagesLogPath != m_pOptions->sMessagesLogPath))
				m_pMessages->setLogging(
					m_pOptions->bMessagesLog, m_pOptions->sMessagesLogPath);
			if (( bOldCompletePath && !m_pOptions->bCompletePath) ||
				(!bOldCompletePath &&  m_pOptions->bCompletePath) ||
				(iOldMaxRecentFiles != m_pOptions->iMaxRecentFiles))
				updateRecentFilesMenu();
			if (( bOldInstrumentNames && !m_pOptions->bInstrumentNames) ||
				(!bOldInstrumentNames &&  m_pOptions->bInstrumentNames))
				updateInstrumentNames();
			if (( bOldDisplayEffect && !m_pOptions->bDisplayEffect) ||
				(!bOldDisplayEffect &&  m_pOptions->bDisplayEffect))
				updateDisplayEffect();
			if (sOldDisplayFont != m_pOptions->sDisplayFont)
				updateDisplayFont();
			if (iOldMaxVolume != m_pOptions->iMaxVolume)
				updateMaxVolume();
			if (sOldMessagesFont != m_pOptions->sMessagesFont)
				updateMessagesFont();
			if (( bOldMessagesLimit && !m_pOptions->bMessagesLimit) ||
				(!bOldMessagesLimit &&  m_pOptions->bMessagesLimit) ||
				(iOldMessagesLimitLines !=  m_pOptions->iMessagesLimitLines))
				updateMessagesLimit();
			// And now the main thing, whether we'll do client/server recycling?
			if ((sOldServerHost != m_pOptions->sServerHost) ||
				(iOldServerPort != m_pOptions->iServerPort) ||
				(iOldServerTimeout != m_pOptions->iServerTimeout) ||
				( bOldServerStart && !m_pOptions->bServerStart) ||
				(!bOldServerStart &&  m_pOptions->bServerStart) ||
				(sOldServerCmdLine != m_pOptions->sServerCmdLine
				&& m_pOptions->bServerStart))
				fileRestart();
		}
		// Done.
		delete pOptionsForm;
	}

	// This makes it.
	stabilizeForm();
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Channels action slots.

// Arrange channel strips.
void MainForm::channelsArrange (void)
{
	// Full width vertical tiling
	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	if (wlist.isEmpty())
		return;

	m_pWorkspace->setUpdatesEnabled(false);
	int y = 0;
	foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
		pMdiSubWindow->adjustSize();
		const QRect& frameRect
			= pMdiSubWindow->frameGeometry();
		int w = m_pWorkspace->width();
		if (w < frameRect.width())
			w = frameRect.width();
		const int h = frameRect.height();
		pMdiSubWindow->setGeometry(0, y, w, h);
		y += h;
	}
	m_pWorkspace->setUpdatesEnabled(true);

	stabilizeForm();
}


// Auto-arrange channel strips.
void MainForm::channelsAutoArrange ( bool bOn )
{
	if (m_pOptions == nullptr)
		return;

	// Toggle the auto-arrange flag.
	m_pOptions->bAutoArrange = bOn;

	// If on, update whole workspace...
	channelsArrangeAuto();
}


void MainForm::channelsArrangeAuto (void)
{
	if (m_pOptions && m_pOptions->bAutoArrange)
		channelsArrange();
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Help Action slots.

// Show information about the Qt toolkit.
void MainForm::helpAboutQt (void)
{
	QMessageBox::aboutQt(this);
}


// Show information about application program.
void MainForm::helpAbout (void)
{
	QStringList list;
#ifdef CONFIG_DEBUG
	list << tr("Debugging option enabled.");
#endif
#ifndef CONFIG_LIBGIG
	list << tr("GIG (libgig) file support disabled.");
#endif
#ifndef CONFIG_INSTRUMENT_NAME
	list << tr("LSCP (liblscp) instrument_name support disabled.");
#endif
#ifndef CONFIG_MUTE_SOLO
	list << tr("Sampler channel Mute/Solo support disabled.");
#endif
#ifndef CONFIG_AUDIO_ROUTING
	list << tr("LSCP (liblscp) audio_routing support disabled.");
#endif
#ifndef CONFIG_FXSEND
	list << tr("Sampler channel Effect Sends support disabled.");
#endif
#ifndef CONFIG_VOLUME
	list << tr("Global volume support disabled.");
#endif
#ifndef CONFIG_MIDI_INSTRUMENT
	list << tr("MIDI instrument mapping support disabled.");
#endif
#ifndef CONFIG_EDIT_INSTRUMENT
	list << tr("Instrument editing support disabled.");
#endif
#ifndef CONFIG_EVENT_CHANNEL_MIDI
	list << tr("Channel MIDI event support disabled.");
#endif
#ifndef CONFIG_EVENT_DEVICE_MIDI
	list << tr("Device MIDI event support disabled.");
#endif
#ifndef CONFIG_MAX_VOICES
	list << tr("Runtime max. voices / disk streams support disabled.");
#endif

	// Stuff the about box text...
	QString sText = "<p>\n";
	sText += "<b>" QSAMPLER_TITLE " - " + tr(QSAMPLER_SUBTITLE) + "</b><br />\n";
	sText += "<br />\n";
	sText += tr("Version") + ": <b>" CONFIG_BUILD_VERSION "</b><br />\n";
//	sText += "<small>" + tr("Build") + ": " CONFIG_BUILD_DATE "</small><br />\n";
	if (!list.isEmpty()) {
		sText += "<small><font color=\"red\">";
		sText += list.join("<br />\n");
		sText += "</font></small>";
	}
	sText += "<br />\n";
	sText += tr("Using") + ": ";
	sText += ::lscp_client_package();
	sText += " ";
	sText += ::lscp_client_version();
#ifdef CONFIG_LIBGIG
	sText += ", ";
	sText += gig::libraryName().c_str();
	sText += " ";
	sText += gig::libraryVersion().c_str();
#endif
	sText += "<br />\n";
	sText += "<br />\n";
	sText += tr("Website") + ": <a href=\"" QSAMPLER_WEBSITE "\">" QSAMPLER_WEBSITE "</a><br />\n";
	sText += "<br />\n";
	sText += "<small>";
	sText += QSAMPLER_COPYRIGHT "<br />\n";
	sText += QSAMPLER_COPYRIGHT2 "<br />\n";
	sText += "<br />\n";
	sText += tr("This program is free software; you can redistribute it and/or modify it") + "<br />\n";
	sText += tr("under the terms of the GNU General Public License version 2 or later.");
	sText += "</small>";
	sText += "</p>\n";

	QMessageBox::about(this, tr("About"), sText);
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Main window stabilization.

void MainForm::stabilizeForm (void)
{
	// Update the main application caption...
	QString sSessionName = sessionName(m_sFilename);
	if (m_iDirtyCount > 0)
		sSessionName += " *";
	setWindowTitle(sSessionName);

	// Update the main menu state...
	ChannelStrip *pChannelStrip = activeChannelStrip();
	const QList<QMdiSubWindow *>& wlist = m_pWorkspace->subWindowList();
	const bool bHasClient = (m_pOptions != nullptr && m_pClient != nullptr);
	const bool bHasChannel = (bHasClient && pChannelStrip != nullptr);
	const bool bHasChannels = (bHasClient && wlist.count() > 0);
	m_ui.fileNewAction->setEnabled(bHasClient);
	m_ui.fileOpenAction->setEnabled(bHasClient);
	m_ui.fileSaveAction->setEnabled(bHasClient && m_iDirtyCount > 0);
	m_ui.fileSaveAsAction->setEnabled(bHasClient);
	m_ui.fileResetAction->setEnabled(bHasClient);
	m_ui.fileRestartAction->setEnabled(bHasClient || m_pServer == nullptr);
	m_ui.editAddChannelAction->setEnabled(bHasClient);
	m_ui.editRemoveChannelAction->setEnabled(bHasChannel);
	m_ui.editSetupChannelAction->setEnabled(bHasChannel);
#ifdef CONFIG_EDIT_INSTRUMENT
	m_ui.editEditChannelAction->setEnabled(bHasChannel);
#else
	m_ui.editEditChannelAction->setEnabled(false);
#endif
	m_ui.editResetChannelAction->setEnabled(bHasChannel);
	m_ui.editResetAllChannelsAction->setEnabled(bHasChannels);
	m_ui.viewMessagesAction->setChecked(m_pMessages && m_pMessages->isVisible());
#ifdef CONFIG_MIDI_INSTRUMENT
	m_ui.viewInstrumentsAction->setChecked(m_pInstrumentListForm
		&& m_pInstrumentListForm->isVisible());
	m_ui.viewInstrumentsAction->setEnabled(bHasClient);
#else
	m_ui.viewInstrumentsAction->setEnabled(false);
#endif
	m_ui.viewDevicesAction->setChecked(m_pDeviceForm
		&& m_pDeviceForm->isVisible());
	m_ui.viewDevicesAction->setEnabled(bHasClient);
	m_ui.viewMidiDeviceStatusMenu->setEnabled(
		DeviceStatusForm::getInstances().size() > 0);
	m_ui.channelsArrangeAction->setEnabled(bHasChannels);

#ifdef CONFIG_VOLUME
	// Toolbar widgets are also affected...
	m_pVolumeSlider->setEnabled(bHasClient);
	m_pVolumeSpinBox->setEnabled(bHasClient);
#endif

	// Client/Server status...
	if (bHasClient) {
		m_statusItem[QSAMPLER_STATUS_CLIENT]->setText(tr("Connected"));
		m_statusItem[QSAMPLER_STATUS_SERVER]->setText(m_pOptions->sServerHost
			+ ':' + QString::number(m_pOptions->iServerPort));
	} else {
		m_statusItem[QSAMPLER_STATUS_CLIENT]->clear();
		m_statusItem[QSAMPLER_STATUS_SERVER]->clear();
	}
	// Channel status...
	if (bHasChannel)
		m_statusItem[QSAMPLER_STATUS_CHANNEL]->setText(pChannelStrip->windowTitle());
	else
		m_statusItem[QSAMPLER_STATUS_CHANNEL]->clear();
	// Session status...
	if (m_iDirtyCount > 0)
		m_statusItem[QSAMPLER_STATUS_SESSION]->setText(tr("MOD"));
	else
		m_statusItem[QSAMPLER_STATUS_SESSION]->clear();

	// Recent files menu.
	m_ui.fileOpenRecentMenu->setEnabled(m_pOptions->recentFiles.count() > 0);
}


// Global volume change receiver slot.
void MainForm::volumeChanged ( int iVolume )
{
#ifdef CONFIG_VOLUME

	if (m_iVolumeChanging > 0)
		return;

	m_iVolumeChanging++;

	// Update the toolbar widgets...
	if (m_pVolumeSlider->value()  != iVolume)
		m_pVolumeSlider->setValue(iVolume);
	if (m_pVolumeSpinBox->value() != iVolume)
		m_pVolumeSpinBox->setValue(iVolume);

	// Do it as commanded...
	const float fVolume = 0.01f * float(iVolume);
	if (::lscp_set_volume(m_pClient, fVolume) == LSCP_OK)
		appendMessages(QObject::tr("Volume: %1.").arg(fVolume));
	else
		appendMessagesClient("lscp_set_volume");

	m_iVolumeChanging--;

	m_iDirtyCount++;
	stabilizeForm();

#endif
}


// Channel change receiver slot.
void MainForm::channelStripChanged ( ChannelStrip *pChannelStrip )
{
	// Add this strip to the changed list...
	if (!m_changedStrips.contains(pChannelStrip)) {
		m_changedStrips.append(pChannelStrip);
		pChannelStrip->resetErrorCount();
	}

	// Just mark the dirty form.
	m_iDirtyCount++;
	// and update the form status...
	stabilizeForm();
}


// Grab and restore current sampler channels session.
void MainForm::updateSession (void)
{
#ifdef CONFIG_VOLUME
	const int iVolume = ::lroundf(100.0f * ::lscp_get_volume(m_pClient));
	m_iVolumeChanging++;
	m_pVolumeSlider->setValue(iVolume);
	m_pVolumeSpinBox->setValue(iVolume);
	m_iVolumeChanging--;
#endif
#ifdef CONFIG_MIDI_INSTRUMENT
	// FIXME: Make some room for default instrument maps...
	const int iMaps = ::lscp_get_midi_instrument_maps(m_pClient);
	if (iMaps < 0)
		appendMessagesClient("lscp_get_midi_instrument_maps");
	else if (iMaps < 1) {
		::lscp_add_midi_instrument_map(m_pClient,
			tr("Chromatic").toUtf8().constData());
		::lscp_add_midi_instrument_map(m_pClient,
			tr("Drum Kits").toUtf8().constData());
	}
#endif

	updateAllChannelStrips(false);

	// Do we auto-arrange?
	channelsArrangeAuto();

	// Remember to refresh devices and instruments...
	if (m_pInstrumentListForm)
		m_pInstrumentListForm->refreshInstruments();
	if (m_pDeviceForm)
		m_pDeviceForm->refreshDevices();
}


void MainForm::updateAllChannelStrips ( bool bRemoveDeadStrips )
{
	// Skip if setting up a new channel strip...
	if (m_iDirtySetup > 0)
		return;

	// Retrieve the current channel list.
	int *piChannelIDs = ::lscp_list_channels(m_pClient);
	if (piChannelIDs == nullptr) {
		if (::lscp_client_get_errno(m_pClient)) {
			appendMessagesClient("lscp_list_channels");
			appendMessagesError(
				tr("Could not get current list of channels.\n\nSorry."));
		}
	} else {
		// Try to (re)create each channel.
		m_pWorkspace->setUpdatesEnabled(false);
		for (int iChannel = 0; piChannelIDs[iChannel] >= 0; ++iChannel) {
			// Check if theres already a channel strip for this one...
			if (!channelStrip(piChannelIDs[iChannel]))
				createChannelStrip(new Channel(piChannelIDs[iChannel]));
		}
		// Do we auto-arrange?
		channelsArrangeAuto();
		// remove dead channel strips
		if (bRemoveDeadStrips) {
			const QList<QMdiSubWindow *>& wlist
				= m_pWorkspace->subWindowList();
			foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
				ChannelStrip *pChannelStrip
					= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
				if (pChannelStrip) {
					bool bExists = false;
					for (int iChannel = 0; piChannelIDs[iChannel] >= 0; ++iChannel) {
						Channel *pChannel = pChannelStrip->channel();
						if (pChannel == nullptr)
							break;
						if (piChannelIDs[iChannel] == pChannel->channelID()) {
							// strip exists, don't touch it
							bExists = true;
							break;
						}
					}
					if (!bExists)
						destroyChannelStrip(pChannelStrip);
				}
			}
		}
		m_pWorkspace->setUpdatesEnabled(true);
	}

	stabilizeForm();
}


// Update the recent files list and menu.
void MainForm::updateRecentFiles ( const QString& sFilename )
{
	if (m_pOptions == nullptr)
		return;

	// Remove from list if already there (avoid duplicates)
	const int iIndex = m_pOptions->recentFiles.indexOf(sFilename);
	if (iIndex >= 0)
		m_pOptions->recentFiles.removeAt(iIndex);
	// Put it to front...
	m_pOptions->recentFiles.push_front(sFilename);
}


// Update the recent files list and menu.
void MainForm::updateRecentFilesMenu (void)
{
	if (m_pOptions == nullptr)
		return;

	// Time to keep the list under limits.
	int iRecentFiles = m_pOptions->recentFiles.count();
	while (iRecentFiles > m_pOptions->iMaxRecentFiles) {
		m_pOptions->recentFiles.pop_back();
		iRecentFiles--;
	}

	// Rebuild the recent files menu...
	m_ui.fileOpenRecentMenu->clear();
	for (int i = 0; i < iRecentFiles; i++) {
		const QString& sFilename = m_pOptions->recentFiles[i];
		if (QFileInfo(sFilename).exists()) {
			QAction *pAction = m_ui.fileOpenRecentMenu->addAction(
				QString("&%1 %2").arg(i + 1).arg(sessionName(sFilename)),
				this, SLOT(fileOpenRecent()));
			pAction->setData(i);
		}
	}
}


// Force update of the channels instrument names mode.
void MainForm::updateInstrumentNames (void)
{
	// Full channel list update...
	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	if (wlist.isEmpty())
		return;

	m_pWorkspace->setUpdatesEnabled(false);
	foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
		ChannelStrip *pChannelStrip
			= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
		if (pChannelStrip)
			pChannelStrip->updateInstrumentName(true);
	}
	m_pWorkspace->setUpdatesEnabled(true);
}


// Force update of the channels display font.
void MainForm::updateDisplayFont (void)
{
	if (m_pOptions == nullptr)
		return;

	// Check if display font is legal.
	if (m_pOptions->sDisplayFont.isEmpty())
		return;

	// Realize it.
	QFont font;
	if (!font.fromString(m_pOptions->sDisplayFont))
		return;

	// Full channel list update...
	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	if (wlist.isEmpty())
		return;

	m_pWorkspace->setUpdatesEnabled(false);
	foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
		ChannelStrip *pChannelStrip
			= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
		if (pChannelStrip)
			pChannelStrip->setDisplayFont(font);
	}
	m_pWorkspace->setUpdatesEnabled(true);
}


// Update channel strips background effect.
void MainForm::updateDisplayEffect (void)
{
	// Full channel list update...
	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	if (wlist.isEmpty())
		return;

	m_pWorkspace->setUpdatesEnabled(false);
	foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
		ChannelStrip *pChannelStrip
			= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
		if (pChannelStrip)
			pChannelStrip->setDisplayEffect(m_pOptions->bDisplayEffect);
	}
	m_pWorkspace->setUpdatesEnabled(true);
}


// Force update of the channels maximum volume setting.
void MainForm::updateMaxVolume (void)
{
	if (m_pOptions == nullptr)
		return;

#ifdef CONFIG_VOLUME
	m_iVolumeChanging++;
	m_pVolumeSlider->setMaximum(m_pOptions->iMaxVolume);
	m_pVolumeSpinBox->setMaximum(m_pOptions->iMaxVolume);
	m_iVolumeChanging--;
#endif

	// Full channel list update...
	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	if (wlist.isEmpty())
		return;

	m_pWorkspace->setUpdatesEnabled(false);
	foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
		ChannelStrip *pChannelStrip
			= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
		if (pChannelStrip)
			pChannelStrip->setMaxVolume(m_pOptions->iMaxVolume);
	}
	m_pWorkspace->setUpdatesEnabled(true);
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Messages window form handlers.

// Messages output methods.
void MainForm::appendMessages( const QString& s )
{
	if (m_pMessages)
		m_pMessages->appendMessages(s);

	statusBar()->showMessage(s, 3000);
}

void MainForm::appendMessagesColor( const QString& s, const QString& c )
{
	if (m_pMessages)
		m_pMessages->appendMessagesColor(s, c);

	statusBar()->showMessage(s, 3000);
}

void MainForm::appendMessagesText( const QString& s )
{
	if (m_pMessages)
		m_pMessages->appendMessagesText(s);
}

void MainForm::appendMessagesError( const QString& sText )
{
	if (m_pMessages)
		m_pMessages->show();

	appendMessagesColor(sText.simplified(), "#ff0000");

	// Make it look responsive...:)
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	if (m_pOptions && m_pOptions->bConfirmError) {
		const QString& sTitle = tr("Error");
	#if 0
		QMessageBox::critical(this, sTitle, sText, QMessageBox::Cancel);
	#else
		QMessageBox mbox(this);
		mbox.setIcon(QMessageBox::Critical);
		mbox.setWindowTitle(sTitle);
		mbox.setText(sText);
		mbox.setStandardButtons(QMessageBox::Cancel);
		QCheckBox cbox(tr("Don't show this again"));
		cbox.setChecked(false);
		cbox.blockSignals(true);
		mbox.addButton(&cbox, QMessageBox::ActionRole);
		if (mbox.exec() && cbox.isChecked())
			m_pOptions->bConfirmError = false;
	#endif
	}
}


// This is a special message format, just for client results.
void MainForm::appendMessagesClient( const QString& s )
{
	if (m_pClient == nullptr)
		return;

	appendMessagesColor(s + QString(": %1 (errno=%2)")
		.arg(::lscp_client_get_result(m_pClient))
		.arg(::lscp_client_get_errno(m_pClient)), "#996666");

	// Make it look responsive...:)
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}


// Force update of the messages font.
void MainForm::updateMessagesFont (void)
{
	if (m_pOptions == nullptr)
		return;

	if (m_pMessages && !m_pOptions->sMessagesFont.isEmpty()) {
		QFont font;
		if (font.fromString(m_pOptions->sMessagesFont))
			m_pMessages->setMessagesFont(font);
	}
}


// Update messages window line limit.
void MainForm::updateMessagesLimit (void)
{
	if (m_pOptions == nullptr)
		return;

	if (m_pMessages) {
		if (m_pOptions->bMessagesLimit)
			m_pMessages->setMessagesLimit(m_pOptions->iMessagesLimitLines);
		else
			m_pMessages->setMessagesLimit(-1);
	}
}


// Enablement of the messages capture feature.
void MainForm::updateMessagesCapture (void)
{
	if (m_pOptions == nullptr)
		return;

	if (m_pMessages)
		m_pMessages->setCaptureEnabled(m_pOptions->bStdoutCapture);
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- MDI channel strip management.

// The channel strip creation executive.
ChannelStrip *MainForm::createChannelStrip ( Channel *pChannel )
{
	if (m_pClient == nullptr || pChannel == nullptr)
		return nullptr;

	// Add a new channel itema...
	ChannelStrip *pChannelStrip = new ChannelStrip();
	if (pChannelStrip == nullptr)
		return nullptr;

	// Set some initial channel strip options...
	if (m_pOptions) {
		// Background display effect...
		pChannelStrip->setDisplayEffect(m_pOptions->bDisplayEffect);
		// We'll need a display font.
		QFont font;
		if (!m_pOptions->sDisplayFont.isEmpty() &&
			font.fromString(m_pOptions->sDisplayFont))
			pChannelStrip->setDisplayFont(font);
		// Maximum allowed volume setting.
		pChannelStrip->setMaxVolume(m_pOptions->iMaxVolume);
	}

	// Add it to workspace...
	QMdiSubWindow *pMdiSubWindow
		= m_pWorkspace->addSubWindow(pChannelStrip,
			Qt::SubWindow | Qt::FramelessWindowHint);
	pMdiSubWindow->setAttribute(Qt::WA_DeleteOnClose);

	// Actual channel strip setup...
	pChannelStrip->setup(pChannel);

	QObject::connect(pChannelStrip,
		SIGNAL(channelChanged(ChannelStrip *)),
		SLOT(channelStripChanged(ChannelStrip *)));

	// Now we show up us to the world.
	pChannelStrip->show();

	// This is pretty new, so we'll watch for it closely.
	channelStripChanged(pChannelStrip);

	// Return our successful reference...
	return pChannelStrip;
}


void MainForm::destroyChannelStrip ( ChannelStrip *pChannelStrip )
{
	QMdiSubWindow *pMdiSubWindow
		= static_cast<QMdiSubWindow *> (pChannelStrip->parentWidget());
	if (pMdiSubWindow == nullptr)
		return;

	// Just delete the channel strip.
	delete pChannelStrip;
	delete pMdiSubWindow;

	// Do we auto-arrange?
	channelsArrangeAuto();
}


// Retrieve the active channel strip.
ChannelStrip *MainForm::activeChannelStrip (void)
{
	QMdiSubWindow *pMdiSubWindow = m_pWorkspace->activeSubWindow();
	if (pMdiSubWindow)
		return static_cast<ChannelStrip *> (pMdiSubWindow->widget());
	else
		return nullptr;
}


// Retrieve a channel strip by index.
ChannelStrip *MainForm::channelStripAt ( int iStrip )
{
	if (!m_pWorkspace) return nullptr;

	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	if (wlist.isEmpty())
		return nullptr;

	if (iStrip < 0 || iStrip >= wlist.count())
		return nullptr;

	QMdiSubWindow *pMdiSubWindow = wlist.at(iStrip);
	if (pMdiSubWindow)
		return static_cast<ChannelStrip *> (pMdiSubWindow->widget());
	else
		return nullptr;
}


// Retrieve a channel strip by sampler channel id.
ChannelStrip *MainForm::channelStrip ( int iChannelID )
{
	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	if (wlist.isEmpty())
		return nullptr;

	foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
		ChannelStrip *pChannelStrip
			= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
		if (pChannelStrip) {
			Channel *pChannel = pChannelStrip->channel();
			if (pChannel && pChannel->channelID() == iChannelID)
				return pChannelStrip;
		}
	}

	// Not found.
	return nullptr;
}


// Construct the windows menu.
void MainForm::channelsMenuAboutToShow (void)
{
	m_ui.channelsMenu->clear();
	m_ui.channelsMenu->addAction(m_ui.channelsArrangeAction);
	m_ui.channelsMenu->addAction(m_ui.channelsAutoArrangeAction);

	const QList<QMdiSubWindow *>& wlist
		= m_pWorkspace->subWindowList();
	if (!wlist.isEmpty()) {
		m_ui.channelsMenu->addSeparator();
		int iStrip = 0;
		foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
			ChannelStrip *pChannelStrip
				= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
			if (pChannelStrip) {
				QAction *pAction = m_ui.channelsMenu->addAction(
					pChannelStrip->windowTitle(),
					this, SLOT(channelsMenuActivated()));
				pAction->setCheckable(true);
				pAction->setChecked(activeChannelStrip() == pChannelStrip);
				pAction->setData(iStrip);
			}
			++iStrip;
		}
	}
}


// Windows menu activation slot
void MainForm::channelsMenuActivated (void)
{
	// Retrive channel index from action data...
	QAction *pAction = qobject_cast<QAction *> (sender());
	if (pAction == nullptr)
		return;

	ChannelStrip *pChannelStrip = channelStripAt(pAction->data().toInt());
	if (pChannelStrip) {
		pChannelStrip->showNormal();
		pChannelStrip->setFocus();
	}
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Timer stuff.

// Set the pseudo-timer delay schedule.
void MainForm::startSchedule ( int iStartDelay )
{
	m_iStartDelay  = 1 + (iStartDelay * 1000);
	m_iTimerDelay  = 0;
}

// Suspend the pseudo-timer delay schedule.
void MainForm::stopSchedule (void)
{
	m_iStartDelay  = 0;
	m_iTimerDelay  = 0;
}

// Timer slot funtion.
void MainForm::timerSlot (void)
{
	if (m_pOptions == nullptr)
		return;

	// Is it the first shot on server start after a few delay?
	if (m_iTimerDelay < m_iStartDelay) {
		m_iTimerDelay += QSAMPLER_TIMER_MSECS;
		if (m_iTimerDelay >= m_iStartDelay) {
			// If we cannot start it now, maybe a lil'mo'later ;)
			if (!startClient()) {
				m_iStartDelay += m_iTimerDelay;
				m_iTimerDelay  = 0;
			}
		}
	}

	if (m_pClient) {
		// Update the channel information for each pending strip...
		QListIterator<ChannelStrip *> iter(m_changedStrips);
		while (iter.hasNext()) {
			ChannelStrip *pChannelStrip = iter.next();
			// If successfull, remove from pending list...
			if (pChannelStrip->updateChannelInfo()) {
				const int iChannelStrip = m_changedStrips.indexOf(pChannelStrip);
				if (iChannelStrip >= 0)
					m_changedStrips.removeAt(iChannelStrip);
			}
		}
		// Refresh each channel usage, on each period...
		if (m_pOptions->bAutoRefresh) {
			m_iTimerSlot += QSAMPLER_TIMER_MSECS;
			if (m_iTimerSlot >= m_pOptions->iAutoRefreshTime)  {
				m_iTimerSlot = 0;
				// Update the channel stream usage for each strip...
				const QList<QMdiSubWindow *>& wlist
					= m_pWorkspace->subWindowList();
				foreach (QMdiSubWindow *pMdiSubWindow, wlist) {
					ChannelStrip *pChannelStrip
						= static_cast<ChannelStrip *> (pMdiSubWindow->widget());
					if (pChannelStrip && pChannelStrip->isVisible())
						pChannelStrip->updateChannelUsage();
				}
			}
		}

	#if CONFIG_LSCP_CLIENT_CONNECTION_LOST
		// If we lost connection to server: Try to automatically reconnect if we
		// did not start the server.
		//
		// TODO: If we started the server, then we might inform the user that
		// the server probably crashed and asking user ONCE whether we should
		// restart the server.
		if (lscp_client_connection_lost(m_pClient) && !m_pServer)
			startAutoReconnectClient();
	#endif // CONFIG_LSCP_CLIENT_CONNECTION_LOST
	}

	// Register the next timer slot.
	QTimer::singleShot(QSAMPLER_TIMER_MSECS, this, SLOT(timerSlot()));
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Server stuff.

// Start linuxsampler server...
void MainForm::startServer (void)
{
	if (m_pOptions == nullptr)
		return;

	// Aren't already a client, are we?
	if (!m_pOptions->bServerStart || m_pClient)
		return;

	// Is the server process instance still here?
	if (m_pServer) {
		if (QMessageBox::warning(this,
			tr("Warning"),
			tr("Could not start the LinuxSampler server.\n\n"
			"Maybe it is already started."),
			QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
			m_pServer->terminate();
			m_pServer->kill();
		}
		return;
	}

	// Reset our timer counters...
	stopSchedule();

	// Verify we have something to start with...
	if (m_pOptions->sServerCmdLine.isEmpty())
		return;

	// OK. Let's build the startup process...
	m_pServer = new QProcess();
	m_bForceServerStop = true;

	// Setup stdout/stderr capture...
	m_pServer->setProcessChannelMode(QProcess::ForwardedChannels);
	QObject::connect(m_pServer,
		SIGNAL(readyReadStandardOutput()),
		SLOT(readServerStdout()));
	QObject::connect(m_pServer,
		SIGNAL(readyReadStandardError()),
		SLOT(readServerStdout()));

	// The unforgiveable signal communication...
	QObject::connect(m_pServer,
		SIGNAL(finished(int, QProcess::ExitStatus)),
		SLOT(processServerExit()));

	// Build process arguments...
	QStringList args = m_pOptions->sServerCmdLine.split(' ');
	QString sCommand = args[0];
	args.removeAt(0);

	appendMessages(tr("Server is starting..."));
	appendMessagesColor(m_pOptions->sServerCmdLine, "#990099");

	// Go linuxsampler, go...
	m_pServer->start(sCommand, args);
	if (!m_pServer->waitForStarted()) {
		appendMessagesError(tr("Could not start server.\n\nSorry."));
		processServerExit();
		return;
	}

	// Show startup results...
	appendMessages(
		tr("Server was started with PID=%1.").arg((long) m_pServer->pid()));

	// Reset (yet again) the timer counters,
	// but this time is deferred as the user opted.
	startSchedule(m_pOptions->iStartDelay);
	stabilizeForm();
}


// Stop linuxsampler server...
void MainForm::stopServer ( bool bInteractive )
{
	// Stop client code.
	stopClient();

	if (m_pServer && bInteractive) {
		if (QMessageBox::question(this,
			tr("The backend's fate ..."),
			tr("You have the option to keep the sampler backend (LinuxSampler)\n"
			"running in the background. The sampler would continue to work\n"
			"according to your current sampler session and you could alter the\n"
			"sampler session at any time by relaunching QSampler.\n\n"
			"Do you want LinuxSampler to stop?"),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::Yes) == QMessageBox::No) {
			m_bForceServerStop = false;
		}
	}

	bool bGraceWait = true;

	// And try to stop server.
	if (m_pServer && m_bForceServerStop) {
		appendMessages(tr("Server is stopping..."));
		if (m_pServer->state() == QProcess::Running) {
		#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
			// Try harder...
			m_pServer->kill();
		#else
			// Try softly...
			m_pServer->terminate();
			bool bFinished = m_pServer->waitForFinished(QSAMPLER_TIMER_MSECS * 1000);
			if (bFinished) bGraceWait = false;
		#endif
		}
	}	// Do final processing anyway.
	else processServerExit();

	// Give it some time to terminate gracefully and stabilize...
	if (bGraceWait) {
		QElapsedTimer timer;
		timer.start();
		while (timer.elapsed() < QSAMPLER_TIMER_MSECS)
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}


// Stdout handler...
void MainForm::readServerStdout (void)
{
	if (m_pMessages)
		m_pMessages->appendStdoutBuffer(m_pServer->readAllStandardOutput());
}


// Linuxsampler server cleanup.
void MainForm::processServerExit (void)
{
	// Force client code cleanup.
	stopClient();

	// Flush anything that maybe pending...
	if (m_pMessages)
		m_pMessages->flushStdoutBuffer();

	if (m_pServer && m_bForceServerStop) {
		if (m_pServer->state() != QProcess::NotRunning) {
			appendMessages(tr("Server is being forced..."));
			// Force final server shutdown...
			m_pServer->kill();
			// Give it some time to terminate gracefully and stabilize...
			QElapsedTimer timer;
			timer.start();
			while (timer.elapsed() < QSAMPLER_TIMER_MSECS)
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		}
		// Force final server shutdown...
		appendMessages(
			tr("Server was stopped with exit status %1.")
			.arg(m_pServer->exitStatus()));
		delete m_pServer;
		m_pServer = nullptr;
	}

	// Again, make status visible stable.
	stabilizeForm();
}


//-------------------------------------------------------------------------
// QSampler::MainForm -- Client stuff.

// The LSCP client callback procedure.
lscp_status_t qsampler_client_callback ( lscp_client_t */*pClient*/,
	lscp_event_t event, const char *pchData, int cchData, void *pvData )
{
	MainForm* pMainForm = (MainForm *) pvData;
	if (pMainForm == nullptr)
		return LSCP_FAILED;

	// ATTN: DO NOT EVER call any GUI code here,
	// as this is run under some other thread context.
	// A custom event must be posted here...
	QApplication::postEvent(pMainForm,
		new LscpEvent(event, pchData, cchData));

	return LSCP_OK;
}


// Start our almighty client...
bool MainForm::startClient (bool bReconnectOnly)
{
	// Have it a setup?
	if (m_pOptions == nullptr)
		return false;

	// Aren't we already started, are we?
	if (m_pClient)
		return true;

	// Log prepare here.
	appendMessages(tr("Client connecting..."));

	// Create the client handle...
	m_pClient = ::lscp_client_create(
		m_pOptions->sServerHost.toUtf8().constData(),
		m_pOptions->iServerPort, qsampler_client_callback, this);
	if (m_pClient == nullptr) {
		// Is this the first try?
		// maybe we need to start a local server...
		if ((m_pServer && m_pServer->state() == QProcess::Running)
			|| !m_pOptions->bServerStart || bReconnectOnly)
		{
			// if this method is called from autoReconnectClient()
			// then don't bother user with an error message...
			if (!bReconnectOnly) {
				appendMessagesError(
					tr("Could not connect to server as client.\n\nSorry.")
				);
			}
		} else {
			startServer();
		}
		// This is always a failure.
		stabilizeForm();
		return false;
	}

	// Just set receive timeout value, blindly.
	::lscp_client_set_timeout(m_pClient, m_pOptions->iServerTimeout);
	appendMessages(
		tr("Client receive timeout is set to %1 msec.")
		.arg(::lscp_client_get_timeout(m_pClient)));

	// Subscribe to channel info change notifications...
	if (::lscp_client_subscribe(m_pClient, LSCP_EVENT_CHANNEL_COUNT) != LSCP_OK)
		appendMessagesClient("lscp_client_subscribe(CHANNEL_COUNT)");
	if (::lscp_client_subscribe(m_pClient, LSCP_EVENT_CHANNEL_INFO) != LSCP_OK)
		appendMessagesClient("lscp_client_subscribe(CHANNEL_INFO)");

	DeviceStatusForm::onDevicesChanged(); // initialize
	updateViewMidiDeviceStatusMenu();
	if (::lscp_client_subscribe(m_pClient, LSCP_EVENT_MIDI_INPUT_DEVICE_COUNT) != LSCP_OK)
		appendMessagesClient("lscp_client_subscribe(MIDI_INPUT_DEVICE_COUNT)");
	if (::lscp_client_subscribe(m_pClient, LSCP_EVENT_MIDI_INPUT_DEVICE_INFO) != LSCP_OK)
		appendMessagesClient("lscp_client_subscribe(MIDI_INPUT_DEVICE_INFO)");
	if (::lscp_client_subscribe(m_pClient, LSCP_EVENT_AUDIO_OUTPUT_DEVICE_COUNT) != LSCP_OK)
		appendMessagesClient("lscp_client_subscribe(AUDIO_OUTPUT_DEVICE_COUNT)");
	if (::lscp_client_subscribe(m_pClient, LSCP_EVENT_AUDIO_OUTPUT_DEVICE_INFO) != LSCP_OK)
		appendMessagesClient("lscp_client_subscribe(AUDIO_OUTPUT_DEVICE_INFO)");

#if CONFIG_EVENT_CHANNEL_MIDI
	// Subscribe to channel MIDI data notifications...
	if (::lscp_client_subscribe(m_pClient, LSCP_EVENT_CHANNEL_MIDI) != LSCP_OK)
		appendMessagesClient("lscp_client_subscribe(CHANNEL_MIDI)");
#endif

#if CONFIG_EVENT_DEVICE_MIDI
	// Subscribe to channel MIDI data notifications...
	if (::lscp_client_subscribe(m_pClient, LSCP_EVENT_DEVICE_MIDI) != LSCP_OK)
		appendMessagesClient("lscp_client_subscribe(DEVICE_MIDI)");
#endif

	// We may stop scheduling around.
	stopSchedule();

	// We'll accept drops from now on...
	setAcceptDrops(true);

	// Log success here.
	appendMessages(tr("Client connected."));

	// Hard-notify instrumnet and device configuration forms,
	// if visible, that we're ready...
	if (m_pInstrumentListForm)
		m_pInstrumentListForm->refreshInstruments();
	if (m_pDeviceForm)
		m_pDeviceForm->refreshDevices();

	// Is any session pending to be loaded?
	if (!m_pOptions->sSessionFile.isEmpty()) {
		// Just load the prabably startup session...
		if (loadSessionFile(m_pOptions->sSessionFile)) {
			m_pOptions->sSessionFile = QString();
			return true;
		}
	}

	// send the current / loaded fine tuning settings to the sampler
	m_pOptions->sendFineTuningSettings();

	// Make a new session
	return newSession();
}


// Stop client...
void MainForm::stopClient (void)
{
	if (m_pClient == nullptr)
		return;

	// Log prepare here.
	appendMessages(tr("Client disconnecting..."));

	// Clear timer counters...
	stopSchedule();

	// We'll reject drops from now on...
	setAcceptDrops(false);

	// Force any channel strips around, but
	// but avoid removing the corresponding
	// channels from the back-end server.
	m_iDirtyCount = 0;
	closeSession(false);

	// Close us as a client...
#if CONFIG_EVENT_DEVICE_MIDI
	::lscp_client_unsubscribe(m_pClient, LSCP_EVENT_DEVICE_MIDI);
#endif
#if CONFIG_EVENT_CHANNEL_MIDI
	::lscp_client_unsubscribe(m_pClient, LSCP_EVENT_CHANNEL_MIDI);
#endif
	::lscp_client_unsubscribe(m_pClient, LSCP_EVENT_AUDIO_OUTPUT_DEVICE_INFO);
	::lscp_client_unsubscribe(m_pClient, LSCP_EVENT_AUDIO_OUTPUT_DEVICE_COUNT);
	::lscp_client_unsubscribe(m_pClient, LSCP_EVENT_MIDI_INPUT_DEVICE_INFO);
	::lscp_client_unsubscribe(m_pClient, LSCP_EVENT_MIDI_INPUT_DEVICE_COUNT);
	::lscp_client_unsubscribe(m_pClient, LSCP_EVENT_CHANNEL_INFO);
	::lscp_client_unsubscribe(m_pClient, LSCP_EVENT_CHANNEL_COUNT);
	::lscp_client_destroy(m_pClient);
	m_pClient = nullptr;

	// Hard-notify instrumnet and device configuration forms,
	// if visible, that we're running out...
	if (m_pInstrumentListForm)
		m_pInstrumentListForm->refreshInstruments();
	if (m_pDeviceForm)
		m_pDeviceForm->refreshDevices();

	// Log final here.
	appendMessages(tr("Client disconnected."));

	// Make visible status.
	stabilizeForm();
}


void MainForm::startAutoReconnectClient (void)
{
	stopClient();
	appendMessages(tr("Trying to reconnect..."));
	QTimer::singleShot(QSAMPLER_TIMER_MSECS, this, SLOT(autoReconnectClient()));
}


void MainForm::autoReconnectClient (void)
{
	const bool bSuccess = startClient(true);
	if (!bSuccess)
		QTimer::singleShot(QSAMPLER_TIMER_MSECS, this, SLOT(autoReconnectClient()));
}


// Channel strip activation/selection.
void MainForm::activateStrip ( QMdiSubWindow *pMdiSubWindow )
{
	ChannelStrip *pChannelStrip = nullptr;
	if (pMdiSubWindow)
		pChannelStrip = static_cast<ChannelStrip *> (pMdiSubWindow->widget());
	if (pChannelStrip)
		pChannelStrip->setSelected(true);

	stabilizeForm();
}


// Channel toolbar orientation change.
void MainForm::channelsToolbarOrientation ( Qt::Orientation orientation )
{
#ifdef CONFIG_VOLUME
	m_pVolumeSlider->setOrientation(orientation);
	if (orientation == Qt::Horizontal) {
		m_pVolumeSlider->setMinimumHeight(24);
		m_pVolumeSlider->setMaximumHeight(32);
		m_pVolumeSlider->setMinimumWidth(120);
		m_pVolumeSlider->setMaximumWidth(640);
		m_pVolumeSpinBox->setMaximumWidth(64);
		m_pVolumeSpinBox->setButtonSymbols(QSpinBox::UpDownArrows);
	} else {
		m_pVolumeSlider->setMinimumHeight(120);
		m_pVolumeSlider->setMaximumHeight(480);
		m_pVolumeSlider->setMinimumWidth(24);
		m_pVolumeSlider->setMaximumWidth(32);
		m_pVolumeSpinBox->setMaximumWidth(32);
		m_pVolumeSpinBox->setButtonSymbols(QSpinBox::NoButtons);
	}
#endif
}


} // namespace QSampler


// end of qsamplerMainForm.cpp
