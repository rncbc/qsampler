// qsamplerMainForm.cpp
//
/****************************************************************************
   Copyright (C) 2004-2007, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007, Christian Schoenebeck

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

#include "qsamplerMainForm.h"

#include <qapplication.h>
#include <qeventloop.h>
#include <qworkspace.h>
#include <qprocess.h>
#include <qmessagebox.h>
//#include <qdragobject.h>
#include <qregexp.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qstatusbar.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qtooltip.h>

#include "qsamplerAbout.h"
#include "qsamplerOptions.h"
#include "qsamplerChannel.h"
#include "qsamplerMessages.h"

#include "qsamplerChannelStrip.h"
#include "qsamplerInstrumentList.h"

#include "qsamplerInstrumentListForm.h"
#include "qsamplerDeviceForm.h"
#include "qsamplerOptionsForm.h"

#ifdef HAVE_SIGNAL_H
#include <signal.h>
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

// Timer constant stuff.
#define QSAMPLER_TIMER_MSECS    200

// Status bar item indexes
#define QSAMPLER_STATUS_CLIENT  0       // Client connection state.
#define QSAMPLER_STATUS_SERVER  1       // Currenr server address (host:port)
#define QSAMPLER_STATUS_CHANNEL 2       // Active channel caption.
#define QSAMPLER_STATUS_SESSION 3       // Current session modification state.


// All winsock apps needs this.
#if defined(WIN32)
static WSADATA _wsaData;
#endif


//-------------------------------------------------------------------------
// qsamplerCustomEvent -- specialty for callback comunication.

#define QSAMPLER_CUSTOM_EVENT   QEvent::Type(QEvent::User + 0)

class qsamplerCustomEvent : public QEvent
{
public:

    // Constructor.
    qsamplerCustomEvent(lscp_event_t event, const char *pchData, int cchData)
        : QEvent(QSAMPLER_CUSTOM_EVENT)
    {
        m_event = event;
        m_data.setLatin1(pchData, cchData);
    }

    // Accessors.
    lscp_event_t event() { return m_event; }
    QString&     data()  { return m_data;  }

private:

    // The proper event type.
    lscp_event_t m_event;
    // The event data as a string.
    QString      m_data;
};


//-------------------------------------------------------------------------
// qsamplerMainForm -- Main window form implementation.

namespace QSampler {

// Kind of singleton reference.
MainForm* MainForm::g_pMainForm = NULL;

MainForm::MainForm(QWidget* parent) : QMainWindow(parent) {
    ui.setupUi(this);

    fileToolbar     = addToolBar(tr("File"));
    editToolbar     = addToolBar(tr("Edit"));
    channelsToolbar = addToolBar(tr("Channels"));

	// Pseudo-singleton reference setup.
	g_pMainForm = this;

    // Initialize some pointer references.
    m_pOptions = NULL;

    // All child forms are to be created later, not earlier than setup.
    m_pMessages = NULL;
    m_pInstrumentListForm = NULL;
    m_pDeviceForm = NULL;

    // We'll start clean.
    m_iUntitled   = 0;
    m_iDirtyCount = 0;

    m_pServer = NULL;
    m_pClient = NULL;

    m_iStartDelay = 0;
    m_iTimerDelay = 0;

    m_iTimerSlot = 0;

#ifdef HAVE_SIGNAL_H
	// Set to ignore any fatal "Broken pipe" signals.
	::signal(SIGPIPE, SIG_IGN);
#endif

#ifdef CONFIG_VOLUME
    // Make some extras into the toolbar...
	const QString& sVolumeText = tr("Master volume");
	m_iVolumeChanging = 0;
	// Volume slider...
	channelsToolbar->addSeparator();
	m_pVolumeSlider = new QSlider(Qt::Horizontal, channelsToolbar);
	m_pVolumeSlider->setTickmarks(QSlider::Below);
	m_pVolumeSlider->setTickInterval(10);
	m_pVolumeSlider->setPageStep(10);
	m_pVolumeSlider->setRange(0, 100);
	m_pVolumeSlider->setMaximumHeight(22);
	m_pVolumeSlider->setMinimumWidth(160);
	QToolTip::add(m_pVolumeSlider, sVolumeText);
	QObject::connect(m_pVolumeSlider,
		SIGNAL(valueChanged(int)),
		SLOT(volumeChanged(int)));
	//channelsToolbar->setHorizontallyStretchable(true);
	//channelsToolbar->setStretchableWidget(m_pVolumeSlider);
    channelsToolbar->addWidget(m_pVolumeSlider);
	// Volume spin-box
	channelsToolbar->addSeparator();
	m_pVolumeSpinBox = new QSpinBox(channelsToolbar);
	m_pVolumeSpinBox->setSuffix(" %");
	m_pVolumeSpinBox->setRange(0, 100);
	QToolTip::add(m_pVolumeSpinBox, sVolumeText);
	QObject::connect(m_pVolumeSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(volumeChanged(int)));
#endif

    // Make it an MDI workspace.
    m_pWorkspace = new QWorkspace(this);
    m_pWorkspace->setScrollBarsEnabled(true);
	// Set the activation connection.
	QObject::connect(m_pWorkspace,
		SIGNAL(windowActivated(QWidget *)),
		SLOT(stabilizeForm()));
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

    // Create the recent files sub-menu.
    m_pRecentFilesMenu = new Q3PopupMenu(this);
    ui.fileMenu->insertSeparator(4);
    ui.fileMenu->insertItem(tr("Recent &Files"), m_pRecentFilesMenu, 0, 5);

#if defined(WIN32)
    WSAStartup(MAKEWORD(1, 1), &_wsaData);
#endif
}

// Destructor.
MainForm::~MainForm()
{
    // Do final processing anyway.
    processServerExit();

#if defined(WIN32)
    WSACleanup();
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

    // Delete recentfiles menu.
    if (m_pRecentFilesMenu)
        delete m_pRecentFilesMenu;

	// Pseudo-singleton reference shut-down.
	g_pMainForm = NULL;
}


// Make and set a proper setup options step.
void MainForm::setup ( qsamplerOptions *pOptions )
{
    // We got options?
    m_pOptions = pOptions;

    // What style do we create these forms?
	Qt::WFlags wflags = Qt::WStyle_Customize
		| Qt::WStyle_NormalBorder
		| Qt::WStyle_Title
		| Qt::WStyle_SysMenu
		| Qt::WStyle_MinMax
		| Qt::WType_TopLevel;
    if (m_pOptions->bKeepOnTop)
        wflags |= Qt::WStyle_Tool;
    // Some child forms are to be created right now.
    m_pMessages = new qsamplerMessages(this);
    m_pDeviceForm = new DeviceForm(this, wflags);
#ifdef CONFIG_MIDI_INSTRUMENT
    m_pInstrumentListForm = new InstrumentListForm(this, wflags);
	QObject::connect(&m_pInstrumentListForm->model,
		SIGNAL(instrumentsChanged()),
		SLOT(sessionDirty()));
#else
	viewInstrumentsAction->setEnabled(false);
#endif
    // Set message defaults...
    updateMessagesFont();
    updateMessagesLimit();
    updateMessagesCapture();
    // Set the visibility signal.
	QObject::connect(m_pMessages,
		SIGNAL(visibilityChanged(bool)),
		SLOT(stabilizeForm()));

    // Initial decorations toggle state.
    ui.viewMenubarAction->setOn(m_pOptions->bMenubar);
    ui.viewToolbarAction->setOn(m_pOptions->bToolbar);
    ui.viewStatusbarAction->setOn(m_pOptions->bStatusbar);
    ui.channelsAutoArrangeAction->setOn(m_pOptions->bAutoArrange);

    // Initial decorations visibility state.
    viewMenubar(m_pOptions->bMenubar);
    viewToolbar(m_pOptions->bToolbar);
    viewStatusbar(m_pOptions->bStatusbar);

    addDockWidget(Qt::BottomDockWidgetArea, m_pMessages);

    // Restore whole toolbar & dock windows states.
    QString sDockables = m_pOptions->settings().readEntry("/Layout/DockWindowsBase64" , QString::null);
    if (!sDockables.isEmpty()) {
        restoreState(QByteArray::fromBase64(sDockables.toAscii()));
    }
    // Try to restore old window positioning and initial visibility.
    m_pOptions->loadWidgetGeometry(this);
    m_pOptions->loadWidgetGeometry(m_pInstrumentListForm);
    m_pOptions->loadWidgetGeometry(m_pDeviceForm);

    // Final startup stabilization...
    updateMaxVolume();
    updateRecentFilesMenu();
    stabilizeForm();

    // Make it ready :-)
    statusBar()->message(tr("Ready"), 3000);

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
            m_pOptions->bMenubar = ui.MenuBar->isVisible();
            m_pOptions->bToolbar = (fileToolbar->isVisible() || editToolbar->isVisible() || channelsToolbar->isVisible());
            m_pOptions->bStatusbar = statusBar()->isVisible();
            // Save the dock windows state.
            const QString sDockables = saveState().toBase64().data();
            m_pOptions->settings().writeEntry("/Layout/DockWindowsBase64", sDockables);
            // And the children, and the main windows state,.
			m_pOptions->saveWidgetGeometry(m_pDeviceForm);
			m_pOptions->saveWidgetGeometry(m_pInstrumentListForm);
			m_pOptions->saveWidgetGeometry(this);
			// Close popup widgets.
			if (m_pInstrumentListForm)
				m_pInstrumentListForm->close();
			if (m_pDeviceForm)
				m_pDeviceForm->close();
            // Stop client and/or server, gracefully.
            stopServer();
        }
    }

    return bQueryClose;
}


void MainForm::closeEvent ( QCloseEvent *pCloseEvent )
{
    if (queryClose())
        pCloseEvent->accept();
    else
        pCloseEvent->ignore();
}


// Drag'n'drop file handler.
bool MainForm::decodeDragFiles ( const QMimeSource *pEvent, QStringList& files )
{
    bool bDecode = false;

    if (Q3TextDrag::canDecode(pEvent)) {
        QString sText;
        bDecode = Q3TextDrag::decode(pEvent, sText);
        if (bDecode) {
            files = QStringList::split('\n', sText);
            for (QStringList::Iterator iter = files.begin(); iter != files.end(); iter++)
                *iter = QUrl((*iter).stripWhiteSpace().replace(QRegExp("^file:"), QString::null)).path();
        }
    }

    return bDecode;
}


// Window drag-n-drop event handlers.
void MainForm::dragEnterEvent ( QDragEnterEvent* pDragEnterEvent )
{
	QStringList files;
	pDragEnterEvent->accept(decodeDragFiles(pDragEnterEvent, files));
}


void MainForm::dropEvent ( QDropEvent* pDropEvent )
{
    QStringList files;

    if (!decodeDragFiles(pDropEvent, files))
        return;

	for (QStringList::Iterator iter = files.begin(); iter != files.end(); iter++) {
		const QString& sPath = *iter;
		if (qsamplerChannel::isInstrumentFile(sPath)) {
			// Try to create a new channel from instrument file...
			qsamplerChannel *pChannel = new qsamplerChannel();
			if (pChannel == NULL)
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
		// Make it look responsive...:)
		QApplication::processEvents(QEventLoop::ExcludeUserInput);
	}
}

// Custome event handler.
void MainForm::customEvent(QEvent* pCustomEvent)
{
    // For the time being, just pump it to messages.
    if (pCustomEvent->type() == QSAMPLER_CUSTOM_EVENT) {
        qsamplerCustomEvent *pEvent = (qsamplerCustomEvent *) pCustomEvent;
		if (pEvent->event() == LSCP_EVENT_CHANNEL_INFO) {
			int iChannelID = pEvent->data().toInt();
			ChannelStrip *pChannelStrip = channelStrip(iChannelID);
			if (pChannelStrip)
				channelStripChanged(pChannelStrip);
		} else {
			appendMessagesColor(tr("Notify event: %1 data: %2")
				.arg(::lscp_event_to_text(pEvent->event()))
				.arg(pEvent->data()), "#996699");
		}
    }
}

// Context menu event handler.
void MainForm::contextMenuEvent( QContextMenuEvent *pEvent )
{
    stabilizeForm();

    ui.editMenu->exec(pEvent->globalPos());
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Brainless public property accessors.

// The global options settings property.
qsamplerOptions *MainForm::options (void)
{
    return m_pOptions;
}


// The LSCP client descriptor property.
lscp_client_t *MainForm::client (void)
{
    return m_pClient;
}


// The pseudo-singleton instance accessor.
MainForm *MainForm::getInstance (void)
{
	return g_pMainForm;
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Session file stuff.

// Format the displayable session filename.
QString MainForm::sessionName ( const QString& sFilename )
{
    bool bCompletePath = (m_pOptions && m_pOptions->bCompletePath);
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
    m_sFilename = QString::null;
    m_iDirtyCount = 0;
    appendMessages(tr("New session: \"%1\".").arg(sessionName(m_sFilename)));
    stabilizeForm();

    return true;
}


// Open an existing sampler session.
bool MainForm::openSession (void)
{
    if (m_pOptions == NULL)
        return false;

    // Ask for the filename to open...
    QString sFilename = QFileDialog::getOpenFileName(
		m_pOptions->sSessionDir,                // Start here.
		tr("LSCP Session files") + " (*.lscp)", // Filter (LSCP files)
		this, 0,                                // Parent and name (none)
		QSAMPLER_TITLE ": " + tr("Open Session")	// Caption.
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
    if (m_pOptions == NULL)
        return false;

    QString sFilename = m_sFilename;

    // Ask for the file to save, if there's none...
    if (bPrompt || sFilename.isEmpty()) {
        // If none is given, assume default directory.
        if (sFilename.isEmpty())
            sFilename = m_pOptions->sSessionDir;
        // Prompt the guy...
        sFilename = QFileDialog::getSaveFileName(
			sFilename,                              // Start here.
			tr("LSCP Session files") + " (*.lscp)", // Filter (LSCP files)
			this, 0,                                // Parent and name (none)
			QSAMPLER_TITLE ": " + tr("Save Session")	// Caption.
        );
        // Have we cancelled it?
        if (sFilename.isEmpty())
            return false;
        // Enforce .lscp extension...
        if (QFileInfo(sFilename).extension().isEmpty())
            sFilename += ".lscp";
        // Check if already exists...
        if (sFilename != m_sFilename && QFileInfo(sFilename).exists()) {
            if (QMessageBox::warning(this,
				QSAMPLER_TITLE ": " + tr("Warning"),
                tr("The file already exists:\n\n"
                   "\"%1\"\n\n"
                   "Do you want to replace it?")
                   .arg(sFilename),
                tr("Replace"), tr("Cancel")) > 0)
                return false;
        }
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
			QSAMPLER_TITLE ": " + tr("Warning"),
            tr("The current session has been changed:\n\n"
            "\"%1\"\n\n"
            "Do you want to save the changes?")
            .arg(sessionName(m_sFilename)),
            tr("Save"), tr("Discard"), tr("Cancel"))) {
        case 0:     // Save...
            bClose = saveSession(false);
            // Fall thru....
        case 1:     // Discard
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
        QWidgetList wlist = m_pWorkspace->windowList();
        for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
            ChannelStrip *pChannelStrip = (ChannelStrip*) wlist.at(iChannel);
            if (pChannelStrip) {
                qsamplerChannel *pChannel = pChannelStrip->channel();
                if (bForce && pChannel)
                    pChannel->removeChannel();
                delete pChannelStrip;
            }
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
    if (m_pClient == NULL)
        return false;

    // Open and read from real file.
    QFile file(sFilename);
    if (!file.open(IO_ReadOnly)) {
        appendMessagesError(tr("Could not open \"%1\" session file.\n\nSorry.").arg(sFilename));
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
        QString sCommand = ts.readLine().stripWhiteSpace();
		iLine++;
        // If not empty, nor a comment, call the server...
        if (!sCommand.isEmpty() && sCommand[0] != '#') {
			// Remember that, no matter what,
			// all LSCP commands are CR/LF terminated.
			sCommand += "\r\n";
			if (::lscp_client_query(m_pClient, sCommand.latin1()) != LSCP_OK) {
				appendMessagesColor(QString("%1(%2): %3")
					.arg(QFileInfo(sFilename).fileName()).arg(iLine)
					.arg(sCommand.simplifyWhiteSpace()), "#996633");
				appendMessagesClient("lscp_client_query");
				iErrors++;
			}
        }
        // Try to make it snappy :)
        QApplication::processEvents(QEventLoop::ExcludeUserInput);
    }

    // Ok. we've read it.
    file.close();

	// Now we'll try to create (update) the whole GUI session.
	updateSession();

	// We're fornerly done.
	QApplication::restoreOverrideCursor();

	// Have we any errors?
	if (iErrors > 0)
		appendMessagesError(tr("Session loaded with errors\nfrom \"%1\".\n\nSorry.").arg(sFilename));

    // Save as default session directory.
    if (m_pOptions)
        m_pOptions->sSessionDir = QFileInfo(sFilename).dirPath(true);
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
	if (m_pClient == NULL)
		return false;

	// Check whether server is apparently OK...
	if (::lscp_get_channels(m_pClient) < 0) {
		appendMessagesClient("lscp_get_channels");
		return false;
	}

    // Open and write into real file.
    QFile file(sFilename);
    if (!file.open(IO_WriteOnly | IO_Truncate)) {
        appendMessagesError(tr("Could not open \"%1\" session file.\n\nSorry.").arg(sFilename));
        return false;
    }

	// Tell the world we'll take some time...
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Write the file.
    int  iErrors = 0;
    QTextStream ts(&file);
    ts << "# " << QSAMPLER_TITLE " - " << tr(QSAMPLER_SUBTITLE) << endl;
    ts << "# " << tr("Version")
       << ": " QSAMPLER_VERSION << endl;
    ts << "# " << tr("Build")
       << ": " __DATE__ " " __TIME__ << endl;
    ts << "#"  << endl;
    ts << "# " << tr("File")
       << ": " << QFileInfo(sFilename).fileName() << endl;
    ts << "# " << tr("Date")
       << ": " << QDate::currentDate().toString("MMM dd yyyy")
       << " "  << QTime::currentTime().toString("hh:mm:ss") << endl;
    ts << "#"  << endl;
    ts << endl;

	// It is assumed that this new kind of device+session file
	// will be loaded from a complete initialized server...
	int *piDeviceIDs;
	int  iDevice;
	ts << "RESET" << endl;

	// Audio device mapping.
	QMap<int, int> audioDeviceMap;
	piDeviceIDs = qsamplerDevice::getDevices(m_pClient, qsamplerDevice::Audio);
	for (iDevice = 0; piDeviceIDs && piDeviceIDs[iDevice] >= 0; iDevice++) {
		ts << endl;
		qsamplerDevice device(qsamplerDevice::Audio, piDeviceIDs[iDevice]);
		// Audio device specification...
		ts << "# " << device.deviceTypeName() << " " << device.driverName()
			<< " " << tr("Device") << " " << iDevice << endl;
		ts << "CREATE AUDIO_OUTPUT_DEVICE " << device.driverName();
		qsamplerDeviceParamMap::ConstIterator deviceParam;
		for (deviceParam = device.params().begin();
				deviceParam != device.params().end();
					++deviceParam) {
			const qsamplerDeviceParam& param = deviceParam.data();
			if (param.value.isEmpty()) ts << "# ";
			ts << " " << deviceParam.key() << "='" << param.value << "'";
		}
		ts << endl;
		// Audio channel parameters...
		int iPort = 0;
		for (qsamplerDevicePort *pPort = device.ports().first();
				pPort;
					pPort = device.ports().next(), ++iPort) {
			qsamplerDeviceParamMap::ConstIterator portParam;
			for (portParam = pPort->params().begin();
					portParam != pPort->params().end();
						++portParam) {
				const qsamplerDeviceParam& param = portParam.data();
				if (param.fix || param.value.isEmpty()) ts << "# ";
				ts << "SET AUDIO_OUTPUT_CHANNEL_PARAMETER " << iDevice
					<< " " << iPort << " " << portParam.key()
					<< "='" << param.value << "'" << endl;
			}
		}
		// Audio device index/id mapping.
		audioDeviceMap[device.deviceID()] = iDevice;
		// Try to keep it snappy :)
		QApplication::processEvents(QEventLoop::ExcludeUserInput);
	}

	// MIDI device mapping.
	QMap<int, int> midiDeviceMap;
	piDeviceIDs = qsamplerDevice::getDevices(m_pClient, qsamplerDevice::Midi);
	for (iDevice = 0; piDeviceIDs && piDeviceIDs[iDevice] >= 0; iDevice++) {
		ts << endl;
		qsamplerDevice device(qsamplerDevice::Midi, piDeviceIDs[iDevice]);
		// MIDI device specification...
		ts << "# " << device.deviceTypeName() << " " << device.driverName()
			<< " " << tr("Device") << " " << iDevice << endl;
		ts << "CREATE MIDI_INPUT_DEVICE " << device.driverName();
		qsamplerDeviceParamMap::ConstIterator deviceParam;
		for (deviceParam = device.params().begin();
				deviceParam != device.params().end();
					++deviceParam) {
			const qsamplerDeviceParam& param = deviceParam.data();
			if (param.value.isEmpty()) ts << "# ";
			ts << " " << deviceParam.key() << "='" << param.value << "'";
		}
		ts << endl;
		// MIDI port parameters...
		int iPort = 0;
		for (qsamplerDevicePort *pPort = device.ports().first();
				pPort;
					pPort = device.ports().next(), ++iPort) {
			qsamplerDeviceParamMap::ConstIterator portParam;
			for (portParam = pPort->params().begin();
					portParam != pPort->params().end();
						++portParam) {
				const qsamplerDeviceParam& param = portParam.data();
				if (param.fix || param.value.isEmpty()) ts << "# ";
				ts << "SET MIDI_INPUT_PORT_PARAMETER " << iDevice
				   << " " << iPort << " " << portParam.key()
				   << "='" << param.value << "'" << endl;
			}
		}
		// MIDI device index/id mapping.
		midiDeviceMap[device.deviceID()] = iDevice;
		// Try to keep it snappy :)
		QApplication::processEvents(QEventLoop::ExcludeUserInput);
	}
	ts << endl;

#ifdef CONFIG_MIDI_INSTRUMENT
	// MIDI instrument mapping...
	QMap<int, int> midiInstrumentMap;
	int *piMaps = ::lscp_list_midi_instrument_maps(m_pClient);
	for (int iMap = 0; piMaps && piMaps[iMap] >= 0; iMap++) {
		int iMidiMap = piMaps[iMap];
		const char *pszMapName
			= ::lscp_get_midi_instrument_map_name(m_pClient, iMidiMap);
		ts << "# " << tr("MIDI instrument map") << " " << iMap;
		if (pszMapName)
			ts << " - " << pszMapName;
		ts << endl;
		ts << "ADD MIDI_INSTRUMENT_MAP";
		if (pszMapName)
			ts << " '" << pszMapName << "'";
		ts << endl;
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
				ts << endl;
			}	// Check for errors...
			else if (::lscp_client_get_errno(m_pClient)) {
				appendMessagesClient("lscp_get_midi_instrument_info");
				iErrors++;
			}
			// Try to keep it snappy :)
			QApplication::processEvents(QEventLoop::ExcludeUserInput);
		}
		ts << endl;
		// Check for errors...
		if (pInstrs == NULL && ::lscp_client_get_errno(m_pClient)) {
			appendMessagesClient("lscp_list_midi_instruments");
			iErrors++;
		}
		// MIDI strument index/id mapping.
		midiInstrumentMap[iMidiMap] = iMap;
	}
	// Check for errors...
	if (piMaps == NULL && ::lscp_client_get_errno(m_pClient)) {
		appendMessagesClient("lscp_list_midi_instrument_maps");
		iErrors++;
	}
#endif	// CONFIG_MIDI_INSTRUMENT

	// Sampler channel mapping.
    QWidgetList wlist = m_pWorkspace->windowList();
    for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
        ChannelStrip* pChannelStrip
			= static_cast<ChannelStrip *> (wlist.at(iChannel));
        if (pChannelStrip) {
            qsamplerChannel *pChannel = pChannelStrip->channel();
            if (pChannel) {
                ts << "# " << tr("Channel") << " " << iChannel << endl;
                ts << "ADD CHANNEL" << endl;
				if (audioDeviceMap.isEmpty()) {
					ts << "SET CHANNEL AUDIO_OUTPUT_TYPE " << iChannel
						<< " " << pChannel->audioDriver() << endl;
				} else {
					ts << "SET CHANNEL AUDIO_OUTPUT_DEVICE " << iChannel
						<< " " << audioDeviceMap[pChannel->audioDevice()] << endl;
				}
				if (midiDeviceMap.isEmpty()) {
					ts << "SET CHANNEL MIDI_INPUT_TYPE " << iChannel
						<< " " << pChannel->midiDriver() << endl;
				} else {
					ts << "SET CHANNEL MIDI_INPUT_DEVICE " << iChannel
						<< " " << midiDeviceMap[pChannel->midiDevice()] << endl;
				}
				ts << "SET CHANNEL MIDI_INPUT_PORT " << iChannel
					<< " " << pChannel->midiPort() << endl;
                ts << "SET CHANNEL MIDI_INPUT_CHANNEL " << iChannel << " ";
                if (pChannel->midiChannel() == LSCP_MIDI_CHANNEL_ALL)
                    ts << "ALL";
                else
                    ts << pChannel->midiChannel();
                ts << endl;
                ts << "LOAD ENGINE " << pChannel->engineName() << " " << iChannel << endl;
				if (pChannel->instrumentStatus() < 100) ts << "# ";
				ts << "LOAD INSTRUMENT NON_MODAL '" << pChannel->instrumentFile() << "' "
					<< pChannel->instrumentNr() << " " << iChannel << endl;
				qsamplerChannelRoutingMap::ConstIterator audioRoute;
				for (audioRoute = pChannel->audioRouting().begin();
						audioRoute != pChannel->audioRouting().end();
							++audioRoute) {
					ts << "SET CHANNEL AUDIO_OUTPUT_CHANNEL " << iChannel
						<< " " << audioRoute.key()
						<< " " << audioRoute.data() << endl;
				}
				ts << "SET CHANNEL VOLUME " << iChannel
					<< " " << pChannel->volume() << endl;
				if (pChannel->channelMute())
					ts << "SET CHANNEL MUTE " << iChannel << " 1" << endl;
				if (pChannel->channelSolo())
					ts << "SET CHANNEL SOLO " << iChannel << " 1" << endl;
#ifdef CONFIG_MIDI_INSTRUMENT
				if (pChannel->midiMap() >= 0) {
					ts << "SET CHANNEL MIDI_INSTRUMENT_MAP " << iChannel
						<< " " << midiInstrumentMap[pChannel->midiMap()] << endl;
				}
#endif
#ifdef CONFIG_FXSEND
				int iChannelID = pChannel->channelID();
				int *piFxSends = ::lscp_list_fxsends(m_pClient, iChannelID);
				for (int iFxSend = 0;
						piFxSends && piFxSends[iFxSend] >= 0;
							iFxSend++) {
					lscp_fxsend_info_t *pFxSendInfo	= ::lscp_get_fxsend_info(
						m_pClient, iChannelID, piFxSends[iFxSend]);
					if (pFxSendInfo) {
						ts << "CREATE FX_SEND " << iChannel
							<< " " << pFxSendInfo->midi_controller;
						if (pFxSendInfo->name)
							ts << " '" << pFxSendInfo->name << "'";
						ts << endl;
						int *piRouting = pFxSendInfo->audio_routing;
						for (int iAudioSrc = 0;
								piRouting && piRouting[iAudioSrc] >= 0;
									iAudioSrc++) {
							ts << "SET FX_SEND AUDIO_OUTPUT_CHANNEL "
								<< iChannel
								<< " " << iFxSend
								<< " " << iAudioSrc
								<< " " << piRouting[iAudioSrc] << endl;
						}
#ifdef CONFIG_FXSEND_LEVEL
						ts << "SET FX_SEND LEVEL " << iChannel
							<< " " << iFxSend
							<< " " << pFxSendInfo->level << endl;
#endif
					}	// Check for errors...
					else if (::lscp_client_get_errno(m_pClient)) {
						appendMessagesClient("lscp_get_fxsend_info");
						iErrors++;
					}
				}
#endif
                ts << endl;
            }
        }
        // Try to keep it snappy :)
        QApplication::processEvents(QEventLoop::ExcludeUserInput);
    }

#ifdef CONFIG_VOLUME
	ts << "# " << tr("Global volume level") << endl;
	ts << "SET VOLUME " << ::lscp_get_volume(m_pClient) << endl;
	ts << endl;
#endif

    // Ok. we've wrote it.
    file.close();

	// We're fornerly done.
	QApplication::restoreOverrideCursor();

    // Have we any errors?
    if (iErrors > 0)
        appendMessagesError(tr("Some settings could not be saved\nto \"%1\" session file.\n\nSorry.").arg(sFilename));

    // Save as default session directory.
    if (m_pOptions)
        m_pOptions->sSessionDir = QFileInfo(sFilename).dirPath(true);
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
// qsamplerMainForm -- File Action slots.

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
void MainForm::fileOpenRecent ( int iIndex )
{
    // Check if we can safely close the current session...
    if (m_pOptions && closeSession(true)) {
        QString sFilename = m_pOptions->recentFiles[iIndex];
        loadSessionFile(sFilename);
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
    if (m_pClient == NULL)
        return;

    // Ask user whether he/she want's an internal sampler reset...
    if (QMessageBox::warning(this,
		QSAMPLER_TITLE ": " + tr("Warning"),
        tr("Resetting the sampler instance will close\n"
           "all device and channel configurations.\n\n"
           "Please note that this operation may cause\n"
           "temporary MIDI and Audio disruption.\n\n"
           "Do you want to reset the sampler engine now?"),
        tr("Reset"), tr("Cancel")) > 0)
        return;

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
    if (m_pOptions == NULL)
        return;

    bool bRestart = true;

    // Ask user whether he/she want's a complete restart...
    // (if we're currently up and running)
    if (bRestart && m_pClient) {
        bRestart = (QMessageBox::warning(this,
			QSAMPLER_TITLE ": " + tr("Warning"),
            tr("New settings will be effective after\n"
               "restarting the client/server connection.\n\n"
               "Please note that this operation may cause\n"
               "temporary MIDI and Audio disruption.\n\n"
               "Do you want to restart the connection now?"),
            tr("Restart"), tr("Cancel")) == 0);
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
// qsamplerMainForm -- Edit Action slots.

// Add a new sampler channel.
void MainForm::editAddChannel (void)
{
    if (m_pClient == NULL)
        return;

    // Just create the channel instance...
    qsamplerChannel *pChannel = new qsamplerChannel();
    if (pChannel == NULL)
        return;

    // Before we show it up, may be we'll
    // better ask for some initial values?
    if (!pChannel->channelSetup(this)) {
        delete pChannel;
        return;
    }

    // And give it to the strip (will own the channel instance, if successful).
    if (!createChannelStrip(pChannel)) {
        delete pChannel;
        return;
    }

    // Make that an overall update.
    m_iDirtyCount++;
    stabilizeForm();
}


// Remove current sampler channel.
void MainForm::editRemoveChannel (void)
{
    if (m_pClient == NULL)
        return;

    ChannelStrip* pChannelStrip = activeChannelStrip();
    if (pChannelStrip == NULL)
        return;

    qsamplerChannel *pChannel = pChannelStrip->channel();
    if (pChannel == NULL)
        return;

    // Prompt user if he/she's sure about this...
    if (m_pOptions && m_pOptions->bConfirmRemove) {
        if (QMessageBox::warning(this,
			QSAMPLER_TITLE ": " + tr("Warning"),
            tr("About to remove channel:\n\n"
               "%1\n\n"
               "Are you sure?")
               .arg(pChannelStrip->caption()),
            tr("OK"), tr("Cancel")) > 0)
            return;
    }

    // Remove the existing sampler channel.
    if (!pChannel->removeChannel())
        return;

    // Just delete the channel strip.
    delete pChannelStrip;

    // Do we auto-arrange?
    if (m_pOptions && m_pOptions->bAutoArrange)
        channelsArrange();

    // We'll be dirty, for sure...
    m_iDirtyCount++;
    stabilizeForm();
}


// Setup current sampler channel.
void MainForm::editSetupChannel (void)
{
    if (m_pClient == NULL)
        return;

    ChannelStrip* pChannelStrip = activeChannelStrip();
    if (pChannelStrip == NULL)
        return;

    // Just invoque the channel strip procedure.
    pChannelStrip->channelSetup();
}


// Edit current sampler channel.
void MainForm::editEditChannel (void)
{
    if (m_pClient == NULL)
        return;

    ChannelStrip* pChannelStrip = activeChannelStrip();
    if (pChannelStrip == NULL)
        return;

    // Just invoque the channel strip procedure.
    pChannelStrip->channelEdit();
}


// Reset current sampler channel.
void MainForm::editResetChannel (void)
{
    if (m_pClient == NULL)
        return;

    ChannelStrip* pChannelStrip = activeChannelStrip();
    if (pChannelStrip == NULL)
        return;

    // Just invoque the channel strip procedure.
    pChannelStrip->channelReset();
}


// Reset all sampler channels.
void MainForm::editResetAllChannels (void)
{
	if (m_pClient == NULL)
		return;

	// Invoque the channel strip procedure,
	// for all channels out there...
	m_pWorkspace->setUpdatesEnabled(false);
	QWidgetList wlist = m_pWorkspace->windowList();
	for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
		ChannelStrip* pChannelStrip = (ChannelStrip*) wlist.at(iChannel);
		if (pChannelStrip)
			pChannelStrip->channelReset();
	}
	m_pWorkspace->setUpdatesEnabled(true);
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- View Action slots.

// Show/hide the main program window menubar.
void MainForm::viewMenubar ( bool bOn )
{
    if (bOn)
        ui.MenuBar->show();
    else
        ui.MenuBar->hide();
}


// Show/hide the main program window toolbar.
void MainForm::viewToolbar ( bool bOn )
{
    if (bOn) {
        fileToolbar->show();
        editToolbar->show();
        channelsToolbar->show();
    } else {
        fileToolbar->hide();
        editToolbar->hide();
        channelsToolbar->hide();
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
	if (m_pOptions == NULL)
		return;

	if (m_pInstrumentListForm) {
		m_pOptions->saveWidgetGeometry(m_pInstrumentListForm);
		if (m_pInstrumentListForm->isVisible()) {
			m_pInstrumentListForm->hide();
		} else {
			m_pInstrumentListForm->show();
			m_pInstrumentListForm->raise();
			m_pInstrumentListForm->setActiveWindow();
		}
	}
}


// Show/hide the device configurator form.
void MainForm::viewDevices (void)
{
	if (m_pOptions == NULL)
		return;

	if (m_pDeviceForm) {
		m_pOptions->saveWidgetGeometry(m_pDeviceForm);
		if (m_pDeviceForm->isVisible()) {
			m_pDeviceForm->hide();
		} else {
			m_pDeviceForm->show();
			m_pDeviceForm->raise();
			m_pDeviceForm->setActiveWindow();
		}
	}
}


// Show options dialog.
void MainForm::viewOptions (void)
{
    if (m_pOptions == NULL)
        return;

    OptionsForm* pOptionsForm = new OptionsForm(this);
    if (pOptionsForm) {
        // Check out some initial nullities(tm)...
        ChannelStrip* pChannelStrip = activeChannelStrip();
        if (m_pOptions->sDisplayFont.isEmpty() && pChannelStrip)
            m_pOptions->sDisplayFont = pChannelStrip->displayFont().toString();
        if (m_pOptions->sMessagesFont.isEmpty() && m_pMessages)
            m_pOptions->sMessagesFont = m_pMessages->messagesFont().toString();
        // To track down deferred or immediate changes.
        QString sOldServerHost      = m_pOptions->sServerHost;
        int     iOldServerPort      = m_pOptions->iServerPort;
        int     iOldServerTimeout   = m_pOptions->iServerTimeout;
        bool    bOldServerStart     = m_pOptions->bServerStart;
        QString sOldServerCmdLine   = m_pOptions->sServerCmdLine;
        QString sOldDisplayFont     = m_pOptions->sDisplayFont;
        bool    bOldDisplayEffect   = m_pOptions->bDisplayEffect;
        int     iOldMaxVolume       = m_pOptions->iMaxVolume;
        QString sOldMessagesFont    = m_pOptions->sMessagesFont;
        bool    bOldKeepOnTop       = m_pOptions->bKeepOnTop;
        bool    bOldStdoutCapture   = m_pOptions->bStdoutCapture;
        int     bOldMessagesLimit   = m_pOptions->bMessagesLimit;
        int     iOldMessagesLimitLines = m_pOptions->iMessagesLimitLines;
        bool    bOldCompletePath    = m_pOptions->bCompletePath;
        bool    bOldInstrumentNames = m_pOptions->bInstrumentNames;
        int     iOldMaxRecentFiles  = m_pOptions->iMaxRecentFiles;
        // Load the current setup settings.
        pOptionsForm->setup(m_pOptions);
        // Show the setup dialog...
        if (pOptionsForm->exec()) {
            // Warn if something will be only effective on next run.
            if (( bOldStdoutCapture && !m_pOptions->bStdoutCapture) ||
                (!bOldStdoutCapture &&  m_pOptions->bStdoutCapture) ||
                ( bOldKeepOnTop     && !m_pOptions->bKeepOnTop)     ||
                (!bOldKeepOnTop     &&  m_pOptions->bKeepOnTop)) {
                QMessageBox::information(this,
					QSAMPLER_TITLE ": " + tr("Information"),
                    tr("Some settings may be only effective\n"
                       "next time you start this program."), tr("OK"));
                updateMessagesCapture();
            }
            // Check wheather something immediate has changed.
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
                (sOldServerCmdLine != m_pOptions->sServerCmdLine && m_pOptions->bServerStart))
                fileRestart();
        }
        // Done.
        delete pOptionsForm;
    }

    // This makes it.
    stabilizeForm();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Channels action slots.

// Arrange channel strips.
void MainForm::channelsArrange (void)
{
    // Full width vertical tiling
    QWidgetList wlist = m_pWorkspace->windowList();
    if (wlist.isEmpty())
        return;

    m_pWorkspace->setUpdatesEnabled(false);
    int y = 0;
    for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
        ChannelStrip* pChannelStrip = (ChannelStrip*) wlist.at(iChannel);
    /*  if (pChannelStrip->testWState(WState_Maximized | WState_Minimized)) {
            // Prevent flicker...
            pChannelStrip->hide();
            pChannelStrip->showNormal();
        }   */
        pChannelStrip->adjustSize();
        int iWidth  = m_pWorkspace->width();
        if (iWidth < pChannelStrip->width())
            iWidth = pChannelStrip->width();
    //  int iHeight = pChannelStrip->height() + pChannelStrip->parentWidget()->baseSize().height();
        int iHeight = pChannelStrip->parentWidget()->frameGeometry().height();
        pChannelStrip->parentWidget()->setGeometry(0, y, iWidth, iHeight);
        y += iHeight;
    }
    m_pWorkspace->setUpdatesEnabled(true);

    stabilizeForm();
}


// Auto-arrange channel strips.
void MainForm::channelsAutoArrange ( bool bOn )
{
    if (m_pOptions == NULL)
        return;

    // Toggle the auto-arrange flag.
    m_pOptions->bAutoArrange = bOn;

    // If on, update whole workspace...
    if (m_pOptions->bAutoArrange)
        channelsArrange();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Help Action slots.

// Show information about the Qt toolkit.
void MainForm::helpAboutQt (void)
{
    QMessageBox::aboutQt(this);
}


// Show information about application program.
void MainForm::helpAbout (void)
{
    // Stuff the about box text...
    QString sText = "<p>\n";
    sText += "<b>" QSAMPLER_TITLE " - " + tr(QSAMPLER_SUBTITLE) + "</b><br />\n";
    sText += "<br />\n";
    sText += tr("Version") + ": <b>" QSAMPLER_VERSION "</b><br />\n";
    sText += "<small>" + tr("Build") + ": " __DATE__ " " __TIME__ "</small><br />\n";
#ifdef CONFIG_DEBUG
    sText += "<small><font color=\"red\">";
    sText += tr("Debugging option enabled.");
    sText += "</font></small><br />";
#endif
#ifndef CONFIG_LIBGIG
    sText += "<small><font color=\"red\">";
    sText += tr("GIG (libgig) file support disabled.");
    sText += "</font></small><br />";
#endif
#ifndef CONFIG_INSTRUMENT_NAME
    sText += "<small><font color=\"red\">";
    sText += tr("LSCP (liblscp) instrument_name support disabled.");
    sText += "</font></small><br />";
#endif
#ifndef CONFIG_MUTE_SOLO
    sText += "<small><font color=\"red\">";
    sText += tr("Sampler channel Mute/Solo support disabled.");
    sText += "</font></small><br />";
#endif
#ifndef CONFIG_AUDIO_ROUTING
    sText += "<small><font color=\"red\">";
    sText += tr("LSCP (liblscp) audio_routing support disabled.");
    sText += "</font></small><br />";
#endif
#ifndef CONFIG_FXSEND
    sText += "<small><font color=\"red\">";
    sText += tr("Sampler channel Effect Sends support disabled.");
    sText += "</font></small><br />";
#endif
#ifndef CONFIG_VOLUME
    sText += "<small><font color=\"red\">";
    sText += tr("Global volume support disabled.");
    sText += "</font></small><br />";
#endif
#ifndef CONFIG_MIDI_INSTRUMENT
    sText += "<small><font color=\"red\">";
    sText += tr("MIDI instrument mapping support disabled.");
    sText += "</font></small><br />";
#endif
#ifndef CONFIG_EDIT_INSTRUMENT
    sText += "<small><font color=\"red\">";
    sText += tr("Instrument editing support disabled.");
    sText += "</font></small><br />";
#endif
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

    QMessageBox::about(this, tr("About") + " " QSAMPLER_TITLE, sText);
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Main window stabilization.

void MainForm::stabilizeForm (void)
{
    // Update the main application caption...
    QString sSessionName = sessionName(m_sFilename);
    if (m_iDirtyCount > 0)
        sSessionName += " *";
    setCaption(tr(QSAMPLER_TITLE " - [%1]").arg(sSessionName));

    // Update the main menu state...
    ChannelStrip* pChannelStrip = activeChannelStrip();
    bool bHasClient  = (m_pOptions != NULL && m_pClient != NULL);
    bool bHasChannel = (bHasClient && pChannelStrip != NULL);
    ui.fileNewAction->setEnabled(bHasClient);
    ui.fileOpenAction->setEnabled(bHasClient);
    ui.fileSaveAction->setEnabled(bHasClient && m_iDirtyCount > 0);
    ui.fileSaveAsAction->setEnabled(bHasClient);
    ui.fileResetAction->setEnabled(bHasClient);
    ui.fileRestartAction->setEnabled(bHasClient || m_pServer == NULL);
    ui.editAddChannelAction->setEnabled(bHasClient);
    ui.editRemoveChannelAction->setEnabled(bHasChannel);
    ui.editSetupChannelAction->setEnabled(bHasChannel);
#ifdef CONFIG_EDIT_INSTRUMENT
    ui.editEditChannelAction->setEnabled(bHasChannel);
#else
    ui.editEditChannelAction->setEnabled(false);
#endif
    ui.editResetChannelAction->setEnabled(bHasChannel);
    ui.editResetAllChannelsAction->setEnabled(bHasChannel);
    ui.viewMessagesAction->setOn(m_pMessages && m_pMessages->isVisible());
#ifdef CONFIG_MIDI_INSTRUMENT
	ui.viewInstrumentsAction->setOn(m_pInstrumentListForm
		&& m_pInstrumentListForm->isVisible());
	ui.viewInstrumentsAction->setEnabled(bHasClient);
#else
	ui.viewInstrumentsAction->setEnabled(false);
#endif
	ui.viewDevicesAction->setOn(m_pDeviceForm
		&& m_pDeviceForm->isVisible());
    ui.viewDevicesAction->setEnabled(bHasClient);
    ui.channelsArrangeAction->setEnabled(bHasChannel);

#ifdef CONFIG_VOLUME
	// Toolbar widgets are also affected...
    m_pVolumeSlider->setEnabled(bHasClient);
    m_pVolumeSpinBox->setEnabled(bHasClient);
#endif

    // Client/Server status...
    if (bHasClient) {
        m_statusItem[QSAMPLER_STATUS_CLIENT]->setText(tr("Connected"));
        m_statusItem[QSAMPLER_STATUS_SERVER]->setText(m_pOptions->sServerHost + ":" + QString::number(m_pOptions->iServerPort));
    } else {
        m_statusItem[QSAMPLER_STATUS_CLIENT]->clear();
        m_statusItem[QSAMPLER_STATUS_SERVER]->clear();
    }
    // Channel status...
    if (bHasChannel)
        m_statusItem[QSAMPLER_STATUS_CHANNEL]->setText(pChannelStrip->caption());
    else
        m_statusItem[QSAMPLER_STATUS_CHANNEL]->clear();
    // Session status...
    if (m_iDirtyCount > 0)
        m_statusItem[QSAMPLER_STATUS_SESSION]->setText(tr("MOD"));
    else
        m_statusItem[QSAMPLER_STATUS_SESSION]->clear();

    // Recent files menu.
    m_pRecentFilesMenu->setEnabled(bHasClient && m_pOptions->recentFiles.count() > 0);

    // Always make the latest message visible.
    if (m_pMessages)
        m_pMessages->scrollToBottom();
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
	float fVolume = 0.01f * float(iVolume);
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
void MainForm::channelStripChanged(ChannelStrip* pChannelStrip)
{
	// Add this strip to the changed list...
	if (m_changedStrips.containsRef(pChannelStrip) == 0) {
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
	int iVolume = ::lroundf(100.0f * ::lscp_get_volume(m_pClient));
	m_iVolumeChanging++;
	m_pVolumeSlider->setValue(iVolume);
	m_pVolumeSpinBox->setValue(iVolume);
	m_iVolumeChanging--;
#endif
#ifdef CONFIG_MIDI_INSTRUMENT
	// FIXME: Make some room for default instrument maps...
	int iMaps = ::lscp_get_midi_instrument_maps(m_pClient);
	if (iMaps < 0)
		appendMessagesClient("lscp_get_midi_instrument_maps");
	else if (iMaps < 1) {
		::lscp_add_midi_instrument_map(m_pClient, tr("Chromatic").latin1());
		::lscp_add_midi_instrument_map(m_pClient, tr("Drum Kits").latin1());
	}
#endif

	// Retrieve the current channel list.
	int *piChannelIDs = ::lscp_list_channels(m_pClient);
	if (piChannelIDs == NULL) {
		if (::lscp_client_get_errno(m_pClient)) {
			appendMessagesClient("lscp_list_channels");
			appendMessagesError(tr("Could not get current list of channels.\n\nSorry."));
		}
	} else {
		// Try to (re)create each channel.
		m_pWorkspace->setUpdatesEnabled(false);
		for (int iChannel = 0; piChannelIDs[iChannel] >= 0; iChannel++) {
			// Check if theres already a channel strip for this one...
			if (!channelStrip(piChannelIDs[iChannel]))
				createChannelStrip(new qsamplerChannel(piChannelIDs[iChannel]));
		}
		m_pWorkspace->setUpdatesEnabled(true);
	}

    // Do we auto-arrange?
    if (m_pOptions && m_pOptions->bAutoArrange)
        channelsArrange();

	// Remember to refresh devices and instruments...
	if (m_pInstrumentListForm)
	    m_pInstrumentListForm->refreshInstruments();
	if (m_pDeviceForm)
	    m_pDeviceForm->refreshDevices();
}


// Update the recent files list and menu.
void MainForm::updateRecentFiles ( const QString& sFilename )
{
    if (m_pOptions == NULL)
        return;

    // Remove from list if already there (avoid duplicates)
    QStringList::Iterator iter = m_pOptions->recentFiles.find(sFilename);
    if (iter != m_pOptions->recentFiles.end())
        m_pOptions->recentFiles.remove(iter);
    // Put it to front...
    m_pOptions->recentFiles.push_front(sFilename);

    // May update the menu.
    updateRecentFilesMenu();
}


// Update the recent files list and menu.
void MainForm::updateRecentFilesMenu (void)
{
    if (m_pOptions == NULL)
        return;

    // Time to keep the list under limits.
    int iRecentFiles = m_pOptions->recentFiles.count();
    while (iRecentFiles > m_pOptions->iMaxRecentFiles) {
        m_pOptions->recentFiles.pop_back();
        iRecentFiles--;
    }

    // rebuild the recent files menu...
    m_pRecentFilesMenu->clear();
    for (int i = 0; i < iRecentFiles; i++) {
        const QString& sFilename = m_pOptions->recentFiles[i];
        if (QFileInfo(sFilename).exists()) {
            m_pRecentFilesMenu->insertItem(QString("&%1 %2")
                .arg(i + 1).arg(sessionName(sFilename)),
                this, SLOT(fileOpenRecent(int)), 0, i);
        }
    }
}


// Force update of the channels instrument names mode.
void MainForm::updateInstrumentNames (void)
{
    // Full channel list update...
    QWidgetList wlist = m_pWorkspace->windowList();
    if (wlist.isEmpty())
        return;

    m_pWorkspace->setUpdatesEnabled(false);
    for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
        ChannelStrip *pChannelStrip = (ChannelStrip *) wlist.at(iChannel);
        if (pChannelStrip)
            pChannelStrip->updateInstrumentName(true);
    }
    m_pWorkspace->setUpdatesEnabled(true);
}


// Force update of the channels display font.
void MainForm::updateDisplayFont (void)
{
    if (m_pOptions == NULL)
        return;

    // Check if display font is legal.
    if (m_pOptions->sDisplayFont.isEmpty())
        return;
    // Realize it.
    QFont font;
    if (!font.fromString(m_pOptions->sDisplayFont))
        return;

    // Full channel list update...
    QWidgetList wlist = m_pWorkspace->windowList();
    if (wlist.isEmpty())
        return;

    m_pWorkspace->setUpdatesEnabled(false);
    for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
        ChannelStrip* pChannelStrip = (ChannelStrip*) wlist.at(iChannel);
        if (pChannelStrip)
            pChannelStrip->setDisplayFont(font);
    }
    m_pWorkspace->setUpdatesEnabled(true);
}


// Update channel strips background effect.
void MainForm::updateDisplayEffect (void)
{
   QPixmap pm;
    if (m_pOptions->bDisplayEffect)
        pm = QPixmap(":/qsampler/pixmaps/displaybg1.png");

    // Full channel list update...
    QWidgetList wlist = m_pWorkspace->windowList();
    if (wlist.isEmpty())
        return;

    m_pWorkspace->setUpdatesEnabled(false);
    for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
        ChannelStrip* pChannelStrip = (ChannelStrip*) wlist.at(iChannel);
        if (pChannelStrip)
            pChannelStrip->setDisplayBackground(pm);
    }
    m_pWorkspace->setUpdatesEnabled(true);
}


// Force update of the channels maximum volume setting.
void MainForm::updateMaxVolume (void)
{
    if (m_pOptions == NULL)
        return;

#ifdef CONFIG_VOLUME
	m_iVolumeChanging++;
	m_pVolumeSlider->setMaxValue(m_pOptions->iMaxVolume);
	m_pVolumeSpinBox->setMaxValue(m_pOptions->iMaxVolume);
	m_iVolumeChanging--;
#endif

    // Full channel list update...
    QWidgetList wlist = m_pWorkspace->windowList();
    if (wlist.isEmpty())
        return;

    m_pWorkspace->setUpdatesEnabled(false);
    for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
        ChannelStrip* pChannelStrip = (ChannelStrip*) wlist.at(iChannel);
        if (pChannelStrip)
            pChannelStrip->setMaxVolume(m_pOptions->iMaxVolume);
    }
    m_pWorkspace->setUpdatesEnabled(true);
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Messages window form handlers.

// Messages output methods.
void MainForm::appendMessages( const QString& s )
{
    if (m_pMessages)
        m_pMessages->appendMessages(s);

    statusBar()->message(s, 3000);
}

void MainForm::appendMessagesColor( const QString& s, const QString& c )
{
    if (m_pMessages)
        m_pMessages->appendMessagesColor(s, c);

    statusBar()->message(s, 3000);
}

void MainForm::appendMessagesText( const QString& s )
{
    if (m_pMessages)
        m_pMessages->appendMessagesText(s);
}

void MainForm::appendMessagesError( const QString& s )
{
    if (m_pMessages)
        m_pMessages->show();

    appendMessagesColor(s.simplifyWhiteSpace(), "#ff0000");

	// Make it look responsive...:)
	QApplication::processEvents(QEventLoop::ExcludeUserInput);

    QMessageBox::critical(this,
		QSAMPLER_TITLE ": " + tr("Error"), s, tr("Cancel"));
}


// This is a special message format, just for client results.
void MainForm::appendMessagesClient( const QString& s )
{
    if (m_pClient == NULL)
        return;

    appendMessagesColor(s + QString(": %1 (errno=%2)")
        .arg(::lscp_client_get_result(m_pClient))
        .arg(::lscp_client_get_errno(m_pClient)), "#996666");

	// Make it look responsive...:)
	QApplication::processEvents(QEventLoop::ExcludeUserInput);
}


// Force update of the messages font.
void MainForm::updateMessagesFont (void)
{
    if (m_pOptions == NULL)
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
    if (m_pOptions == NULL)
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
    if (m_pOptions == NULL)
        return;

    if (m_pMessages)
        m_pMessages->setCaptureEnabled(m_pOptions->bStdoutCapture);
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- MDI channel strip management.

// The channel strip creation executive.
ChannelStrip* MainForm::createChannelStrip(qsamplerChannel* pChannel)
{
    if (m_pClient == NULL || pChannel == NULL)
        return NULL;

    // Prepare for auto-arrange?
    ChannelStrip* pChannelStrip = NULL;
    int y = 0;
    if (m_pOptions && m_pOptions->bAutoArrange) {
        QWidgetList wlist = m_pWorkspace->windowList();
        for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
            pChannelStrip = (ChannelStrip *) wlist.at(iChannel);
			if (pChannelStrip) {
			//  y += pChannelStrip->height() + pChannelStrip->parentWidget()->baseSize().height();
				y += pChannelStrip->parentWidget()->frameGeometry().height();
			}
        }
    }

    // Add a new channel itema...
    Qt::WFlags wflags = Qt::WStyle_Customize | Qt::WStyle_Tool | Qt::WStyle_Title | Qt::WStyle_NoBorder;
    pChannelStrip = new ChannelStrip(m_pWorkspace, wflags);
    if (pChannelStrip == NULL)
        return NULL;

    // Actual channel strip setup...
    pChannelStrip->setup(pChannel);
	QObject::connect(pChannelStrip,
		SIGNAL(channelChanged(qsamplerChannelStrip *)),
		SLOT(channelStripChanged(qsamplerChannelStrip *)));
    // Set some initial aesthetic options...
    if (m_pOptions) {
        // Background display effect...
        pChannelStrip->setDisplayEffect(m_pOptions->bDisplayEffect);
        // We'll need a display font.
        QFont font;
        if (font.fromString(m_pOptions->sDisplayFont))
            pChannelStrip->setDisplayFont(font);
        // Maximum allowed volume setting.
        pChannelStrip->setMaxVolume(m_pOptions->iMaxVolume);
    }

    // Now we show up us to the world.
    pChannelStrip->show();
    // Only then, we'll auto-arrange...
    if (m_pOptions && m_pOptions->bAutoArrange) {
        int iWidth  = m_pWorkspace->width();
    //  int iHeight = pChannel->height() + pChannel->parentWidget()->baseSize().height();
        int iHeight = pChannelStrip->parentWidget()->frameGeometry().height();        pChannelStrip->parentWidget()->setGeometry(0, y, iWidth, iHeight);
    }

	// This is pretty new, so we'll watch for it closely.
	channelStripChanged(pChannelStrip);

    // Return our successful reference...
    return pChannelStrip;
}


// Retrieve the active channel strip.
ChannelStrip* MainForm::activeChannelStrip (void)
{
    return (ChannelStrip*) m_pWorkspace->activeWindow();
}


// Retrieve a channel strip by index.
ChannelStrip* MainForm::channelStripAt ( int iChannel )
{
    QWidgetList wlist = m_pWorkspace->windowList();
    if (wlist.isEmpty())
        return NULL;

    return (ChannelStrip*) wlist.at(iChannel);
}


// Retrieve a channel strip by sampler channel id.
ChannelStrip* MainForm::channelStrip ( int iChannelID )
{
	QWidgetList wlist = m_pWorkspace->windowList();
	if (wlist.isEmpty())
		return NULL;

	for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
		ChannelStrip* pChannelStrip = (ChannelStrip*) wlist.at(iChannel);
		if (pChannelStrip) {
			qsamplerChannel *pChannel = pChannelStrip->channel();
			if (pChannel && pChannel->channelID() == iChannelID)
				return pChannelStrip;
		}
	}

	// Not found.
	return NULL;
}


// Construct the windows menu.
void MainForm::channelsMenuAboutToShow (void)
{
    ui.channelsMenu->clear();
    ui.channelsArrangeAction->addTo(ui.channelsMenu);
    ui.channelsAutoArrangeAction->addTo(ui.channelsMenu);

    QWidgetList wlist = m_pWorkspace->windowList();
    if (!wlist.isEmpty()) {
        ui.channelsMenu->insertSeparator();
        for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
            ChannelStrip* pChannelStrip = (ChannelStrip*) wlist.at(iChannel);
            if (pChannelStrip) {
                int iItemID = ui.channelsMenu->insertItem(pChannelStrip->caption(), this, SLOT(channelsMenuActivated(int)));
                ui.channelsMenu->setItemParameter(iItemID, iChannel);
                ui.channelsMenu->setItemChecked(iItemID, activeChannelStrip() == pChannelStrip);
            }
        }
    }
}


// Windows menu activation slot
void MainForm::channelsMenuActivated ( int iChannel )
{
    ChannelStrip* pChannelStrip = channelStripAt(iChannel);
    if (pChannelStrip)
        pChannelStrip->showNormal();
    pChannelStrip->setFocus();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Timer stuff.

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
    if (m_pOptions == NULL)
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
		if (m_changedStrips.count() > 0) {
			for (ChannelStrip* pChannelStrip = m_changedStrips.first();
					pChannelStrip; pChannelStrip = m_changedStrips.next()) {
				// If successfull, remove from pending list...
				if (pChannelStrip->updateChannelInfo())
					m_changedStrips.remove(pChannelStrip);
			}
		}
		// Refresh each channel usage, on each period...
		if (m_pOptions->bAutoRefresh) {
			m_iTimerSlot += QSAMPLER_TIMER_MSECS;
			if (m_iTimerSlot >= m_pOptions->iAutoRefreshTime)  {
				m_iTimerSlot = 0;
				// Update the channel stream usage for each strip...
				QWidgetList wlist = m_pWorkspace->windowList();
				for (int iChannel = 0;
						iChannel < (int) wlist.count(); iChannel++) {
					ChannelStrip* pChannelStrip
						= (ChannelStrip*) wlist.at(iChannel);
					if (pChannelStrip && pChannelStrip->isVisible())
						pChannelStrip->updateChannelUsage();
				}
			}
		}
	}

    // Register the next timer slot.
    QTimer::singleShot(QSAMPLER_TIMER_MSECS, this, SLOT(timerSlot()));
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Server stuff.

// Start linuxsampler server...
void MainForm::startServer (void)
{
    if (m_pOptions == NULL)
        return;

    // Aren't already a client, are we?
    if (!m_pOptions->bServerStart || m_pClient)
        return;

    // Is the server process instance still here?
    if (m_pServer) {
        switch (QMessageBox::warning(this,
			QSAMPLER_TITLE ": " + tr("Warning"),
            tr("Could not start the LinuxSampler server.\n\n"
               "Maybe it ss already started."),
            tr("Stop"), tr("Kill"), tr("Cancel"))) {
          case 0:
            m_pServer->terminate();
            break;
          case 1:
            m_pServer->kill();
            break;
        }
        return;
    }

    // Reset our timer counters...
    stopSchedule();

    // OK. Let's build the startup process...
    m_pServer = new QProcess(this);

    // Setup stdout/stderr capture...
	//	if (m_pOptions->bStdoutCapture) {
		//m_pServer->setProcessChannelMode(
		//	QProcess::StandardOutput);
		QObject::connect(m_pServer,
			SIGNAL(readyReadStdout()),
			SLOT(readServerStdout()));
		QObject::connect(m_pServer,
			SIGNAL(readyReadStderr()),
			SLOT(readServerStdout()));
	//	}
	// The unforgiveable signal communication...
	QObject::connect(m_pServer,
		SIGNAL(processExited()),
		SLOT(processServerExit()));

    // Build process arguments...
    QStringList serverCmdLine = QStringList::split(' ', m_pOptions->sServerCmdLine);

    appendMessages(tr("Server is starting..."));
    appendMessagesColor(m_pOptions->sServerCmdLine, "#990099");



    const QString prog = (serverCmdLine.size() > 0) ? serverCmdLine[0] : QString();
    const QStringList args = serverCmdLine.mid(1);

    // Go jack, go...
    m_pServer->start(prog, args);
    if (!m_pServer->waitForStarted()) {
        appendMessagesError(tr("Could not start server.\n\nSorry."));
        processServerExit();
        return;
    }

    // Show startup results...
    appendMessages(tr("Server was started with PID=%1.").arg((long) m_pServer->pid()));

    // Reset (yet again) the timer counters,
    // but this time is deferred as the user opted.
    startSchedule(m_pOptions->iStartDelay);
    stabilizeForm();
}


// Stop linuxsampler server...
void MainForm::stopServer (void)
{
    // Stop client code.
    stopClient();

    // And try to stop server.
    if (m_pServer) {
        appendMessages(tr("Server is stopping..."));
        if (m_pServer->state() == QProcess::Running)
            m_pServer->terminate();
     }

    // Give it some time to terminate gracefully and stabilize...
    QTime t;
    t.start();
    while (t.elapsed() < QSAMPLER_TIMER_MSECS)
        QApplication::processEvents(QEventLoop::ExcludeUserInput);

     // Do final processing anyway.
     processServerExit();
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

    if (m_pServer) {
        // Force final server shutdown...
        appendMessages(tr("Server was stopped with exit status %1.").arg(m_pServer->exitStatus()));
        m_pServer->terminate();
        if (!m_pServer->waitForFinished(2000))
            m_pServer->kill();
        // Destroy it.
        delete m_pServer;
        m_pServer = NULL;
    }

    // Again, make status visible stable.
    stabilizeForm();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Client stuff.

// The LSCP client callback procedure.
lscp_status_t qsampler_client_callback ( lscp_client_t */*pClient*/, lscp_event_t event, const char *pchData, int cchData, void *pvData )
{
    MainForm* pMainForm = (MainForm *) pvData;
    if (pMainForm == NULL)
        return LSCP_FAILED;

    // ATTN: DO NOT EVER call any GUI code here,
    // as this is run under some other thread context.
    // A custom event must be posted here...
    QApplication::postEvent(pMainForm, new qsamplerCustomEvent(event, pchData, cchData));

    return LSCP_OK;
}


// Start our almighty client...
bool MainForm::startClient (void)
{
    // Have it a setup?
    if (m_pOptions == NULL)
        return false;

    // Aren't we already started, are we?
    if (m_pClient)
        return true;

    // Log prepare here.
    appendMessages(tr("Client connecting..."));

    // Create the client handle...
    m_pClient = ::lscp_client_create(m_pOptions->sServerHost.latin1(), m_pOptions->iServerPort, qsampler_client_callback, this);
    if (m_pClient == NULL) {
        // Is this the first try?
        // maybe we need to start a local server...
        if ((m_pServer && m_pServer->state() == QProcess::Running) || !m_pOptions->bServerStart)
            appendMessagesError(tr("Could not connect to server as client.\n\nSorry."));
        else
            startServer();
        // This is always a failure.
        stabilizeForm();
        return false;
    }
    // Just set receive timeout value, blindly.
    ::lscp_client_set_timeout(m_pClient, m_pOptions->iServerTimeout);
    appendMessages(tr("Client receive timeout is set to %1 msec.").arg(::lscp_client_get_timeout(m_pClient)));

	// Subscribe to channel info change notifications...
	if (::lscp_client_subscribe(m_pClient, LSCP_EVENT_CHANNEL_INFO) != LSCP_OK)
		appendMessagesClient("lscp_client_subscribe");

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
            m_pOptions->sSessionFile = QString::null;
            return true;
        }
    }

    // Make a new session
    return newSession();
}


// Stop client...
void MainForm::stopClient (void)
{
    if (m_pClient == NULL)
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
	::lscp_client_unsubscribe(m_pClient, LSCP_EVENT_CHANNEL_INFO);
    ::lscp_client_destroy(m_pClient);
    m_pClient = NULL;

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

} // namespace QSampler


// end of qsamplerMainForm.cpp
