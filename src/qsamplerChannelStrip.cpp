// qsamplerChannelStrip.cpp
//
/****************************************************************************
   Copyright (C) 2004-2007, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007, 2008 Christian Schoenebeck

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
#include "qsamplerChannelStrip.h"

#include "qsamplerMainForm.h"

#include "qsamplerChannelFxForm.h"

#include <QMessageBox>
#include <QDragEnterEvent>
#include <QUrl>

// Channel status/usage usage limit control.
#define QSAMPLER_ERROR_LIMIT	3

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


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::ChannelStrip -- Channel strip form implementation.
//

// Channel strip activation/selection.
ChannelStrip *ChannelStrip::g_pSelectedStrip = NULL;

ChannelStrip::ChannelStrip ( QWidget* pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	m_ui.setupUi(this);

	// Initialize locals.
	m_pChannel     = NULL;
	m_iDirtyChange = 0;
	m_iErrorCount  = 0;

	// Try to restore normal window positioning.
	adjustSize();

	QObject::connect(m_ui.ChannelSetupPushButton,
		SIGNAL(clicked()),
		SLOT(channelSetup()));
	QObject::connect(m_ui.ChannelMutePushButton,
		SIGNAL(toggled(bool)),
		SLOT(channelMute(bool)));
	QObject::connect(m_ui.ChannelSoloPushButton,
		SIGNAL(toggled(bool)),
		SLOT(channelSolo(bool)));
	QObject::connect(m_ui.VolumeSlider,
		SIGNAL(valueChanged(int)),
		SLOT(volumeChanged(int)));
	QObject::connect(m_ui.VolumeSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(volumeChanged(int)));
	QObject::connect(m_ui.ChannelEditPushButton,
		SIGNAL(clicked()),
		SLOT(channelEdit()));
	QObject::connect(m_ui.FxPushButton,
		SIGNAL(clicked()),
		SLOT(channelFxEdit()));

	setSelected(false);
}


ChannelStrip::~ChannelStrip (void)
{
	setSelected(false);

	// Destroy existing channel descriptor.
	if (m_pChannel)
		delete m_pChannel;
	m_pChannel = NULL;
}


// Window drag-n-drop event handlers.
void ChannelStrip::dragEnterEvent ( QDragEnterEvent* pDragEnterEvent )
{
	if (m_pChannel == NULL)
		return;

	bool bAccept = false;

	if (pDragEnterEvent->source() == NULL) {
		const QMimeData *pMimeData = pDragEnterEvent->mimeData();
		if (pMimeData && pMimeData->hasUrls()) {
			QListIterator<QUrl> iter(pMimeData->urls());
			while (iter.hasNext()) {
				const QString& sFilename = iter.next().toLocalFile();
				if (!sFilename.isEmpty()) {
					bAccept = Channel::isInstrumentFile(sFilename);
					break;
				}
			}
		}
	}

	if (bAccept)
		pDragEnterEvent->accept();
	else
		pDragEnterEvent->ignore();
}


void ChannelStrip::dropEvent ( QDropEvent* pDropEvent )
{
	if (m_pChannel == NULL)
		return;

	if (pDropEvent->source())
		return;

	const QMimeData *pMimeData = pDropEvent->mimeData();
	if (pMimeData && pMimeData->hasUrls()) {
		QStringList files;
		QListIterator<QUrl> iter(pMimeData->urls());
		while (iter.hasNext()) {
			const QString& sFilename = iter.next().toLocalFile();
			if (!sFilename.isEmpty()) {
				// Go and set the dropped instrument filename...
				m_pChannel->setInstrument(sFilename, 0);
				// Open up the channel dialog.
				channelSetup();
				break;
			}
		}
	}
}


// Channel strip setup formal initializer.
void ChannelStrip::setup ( Channel *pChannel )
{
	// Destroy any previous channel descriptor;
	// (remember that once setup we own it!)
	if (m_pChannel)
		delete m_pChannel;

	// Set the new one...
	m_pChannel = pChannel;

	// Stabilize this around.
	updateChannelInfo();

	// We'll accept drops from now on...
	if (m_pChannel)
		setAcceptDrops(true);
}


// Channel secriptor accessor.
Channel *ChannelStrip::channel (void) const
{
	return m_pChannel;
}


// Messages view font accessors.
QFont ChannelStrip::displayFont (void) const
{
	return m_ui.EngineNameTextLabel->font();
}

void ChannelStrip::setDisplayFont ( const QFont & font )
{
	m_ui.EngineNameTextLabel->setFont(font);
	m_ui.MidiPortChannelTextLabel->setFont(font);
	m_ui.InstrumentNameTextLabel->setFont(font);
	m_ui.InstrumentStatusTextLabel->setFont(font);
}


// Channel display background effect.
void ChannelStrip::setDisplayEffect ( bool bDisplayEffect )
{
	QPalette pal;
	pal.setColor(QPalette::Foreground, Qt::yellow);
	m_ui.EngineNameTextLabel->setPalette(pal);
	m_ui.MidiPortChannelTextLabel->setPalette(pal);
	pal.setColor(QPalette::Foreground, Qt::green);
	if (bDisplayEffect) {
		QPixmap pm(":/icons/displaybg1.png");
		pal.setBrush(QPalette::Background, QBrush(pm));
	} else {
		pal.setColor(QPalette::Background, Qt::black);
	}
	m_ui.ChannelInfoFrame->setPalette(pal);
	m_ui.InstrumentNameTextLabel->setPalette(pal);
	m_ui.StreamVoiceCountTextLabel->setPalette(pal);
}


// Maximum volume slider accessors.
void ChannelStrip::setMaxVolume ( int iMaxVolume )
{
	m_iDirtyChange++;
	m_ui.VolumeSlider->setRange(0, iMaxVolume);
	m_ui.VolumeSpinBox->setRange(0, iMaxVolume);
	m_iDirtyChange--;
}


// Channel setup dialog slot.
bool ChannelStrip::channelSetup (void)
{
	if (m_pChannel == NULL)
		return false;

	// Invoke the channel setup dialog.
	bool bResult = m_pChannel->channelSetup(this);
	// Notify that this channel has changed.
	if (bResult)
		emit channelChanged(this);

	return bResult;
}


// Channel mute slot.
bool ChannelStrip::channelMute ( bool bMute )
{
	if (m_pChannel == NULL)
		return false;

	// Invoke the channel mute method.
	bool bResult = m_pChannel->setChannelMute(bMute);
	// Notify that this channel has changed.
	if (bResult)
		emit channelChanged(this);

	return bResult;
}


// Channel solo slot.
bool ChannelStrip::channelSolo ( bool bSolo )
{
	if (m_pChannel == NULL)
		return false;

	// Invoke the channel solo method.
	bool bResult = m_pChannel->setChannelSolo(bSolo);
	// Notify that this channel has changed.
	if (bResult)
		emit channelChanged(this);

	return bResult;
}


// Channel edit slot.
void ChannelStrip::channelEdit (void)
{
	if (m_pChannel == NULL)
		return;

	m_pChannel->editChannel();
}

bool ChannelStrip::channelFxEdit (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (!pMainForm || !channel())
		return false;

	pMainForm->appendMessages(QObject::tr("channel fx sends..."));

	bool bResult = false;

#if CONFIG_FXSEND
	ChannelFxForm *pChannelFxForm =
		new ChannelFxForm(channel()->channelID(), parentWidget());
	if (pChannelFxForm) {
		//pChannelForm->setup(this);
		bResult = pChannelFxForm->exec();
		delete pChannelFxForm;
	}
#else // CONFIG_FXSEND
	QMessageBox::critical(this,
		QSAMPLER_TITLE ": " + tr("Unavailable"),
			tr("Sorry, QSampler was built without FX send support!\n\n"
			   "(Make sure you have a recent liblscp when recompiling QSampler)"));
#endif // CONFIG_FXSEND

	return bResult;
}

// Channel reset slot.
bool ChannelStrip::channelReset (void)
{
	if (m_pChannel == NULL)
		return false;

	// Invoke the channel reset method.
	bool bResult = m_pChannel->channelReset();
	// Notify that this channel has changed.
	if (bResult)
		emit channelChanged(this);

	return bResult;
}


// Update the channel instrument name.
bool ChannelStrip::updateInstrumentName ( bool bForce )
{
	if (m_pChannel == NULL)
		return false;

	// Do we refresh the actual name?
	if (bForce)
		m_pChannel->updateInstrumentName();

	// Instrument name...
	if (m_pChannel->instrumentName().isEmpty()) {
		if (m_pChannel->instrumentStatus() >= 0) {
			m_ui.InstrumentNameTextLabel->setText(
				' ' + Channel::loadingInstrument());
		} else {
			m_ui.InstrumentNameTextLabel->setText(
				' ' + Channel::noInstrumentName());
		}
	} else {
		m_ui.InstrumentNameTextLabel->setText(
			' ' + m_pChannel->instrumentName());
	}

	return true;
}


// Do the dirty volume change.
bool ChannelStrip::updateChannelVolume (void)
{
	if (m_pChannel == NULL)
		return false;

	// Convert...
	int iVolume = ::lroundf(100.0f * m_pChannel->volume());
	// And clip...
	if (iVolume < 0)
		iVolume = 0;

	// Flag it here, to avoid infinite recursion.
	m_iDirtyChange++;
	m_ui.VolumeSlider->setValue(iVolume);
	m_ui.VolumeSpinBox->setValue(iVolume);
	m_iDirtyChange--;

	return true;
}


// Update whole channel info state.
bool ChannelStrip::updateChannelInfo (void)
{
	if (m_pChannel == NULL)
		return false;

	// Check for error limit/recycle...
	if (m_iErrorCount > QSAMPLER_ERROR_LIMIT)
		return true;

	// Update strip caption.
	QString sText = m_pChannel->channelName();
	setWindowTitle(sText);
	m_ui.ChannelSetupPushButton->setText('&' + sText);

	// Check if we're up and connected.
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm->client() == NULL)
		return false;

	// Read actual channel information.
	m_pChannel->updateChannelInfo();

	// Engine name...
	if (m_pChannel->engineName().isEmpty()) {
		m_ui.EngineNameTextLabel->setText(
			' ' + Channel::noEngineName());
	} else {
		m_ui.EngineNameTextLabel->setText(
			' ' + m_pChannel->engineName());
	}

	// Instrument name...
	updateInstrumentName(false);

	// MIDI Port/Channel...
	QString sMidiPortChannel = QString::number(m_pChannel->midiPort()) + " / ";
	if (m_pChannel->midiChannel() == LSCP_MIDI_CHANNEL_ALL)
		sMidiPortChannel += tr("All");
	else
		sMidiPortChannel += QString::number(m_pChannel->midiChannel() + 1);
	m_ui.MidiPortChannelTextLabel->setText(sMidiPortChannel);

	// Common palette...
	QPalette pal;
	const QColor& rgbFore = pal.color(QPalette::Foreground);

	// Instrument status...
	int iInstrumentStatus = m_pChannel->instrumentStatus();
	if (iInstrumentStatus < 0) {
		pal.setColor(QPalette::Foreground, Qt::red);
		m_ui.InstrumentStatusTextLabel->setPalette(pal);
		m_ui.InstrumentStatusTextLabel->setText(
			tr("ERR%1").arg(iInstrumentStatus));
		m_iErrorCount++;
		return false;
	}
	// All seems normal...
	pal.setColor(QPalette::Foreground,
		iInstrumentStatus < 100 ? Qt::yellow : Qt::green);
	m_ui.InstrumentStatusTextLabel->setPalette(pal);
	m_ui.InstrumentStatusTextLabel->setText(
		QString::number(iInstrumentStatus) + '%');
	m_iErrorCount = 0;

#ifdef CONFIG_MUTE_SOLO
	// Mute/Solo button state coloring...
	bool bMute = m_pChannel->channelMute();
	const QColor& rgbButton = pal.color(QPalette::Button);
	pal.setColor(QPalette::Foreground, rgbFore);
	pal.setColor(QPalette::Button, bMute ? Qt::yellow : rgbButton);
	m_ui.ChannelMutePushButton->setPalette(pal);
	m_ui.ChannelMutePushButton->setDown(bMute);
	bool bSolo = m_pChannel->channelSolo();
	pal.setColor(QPalette::Button, bSolo ? Qt::cyan : rgbButton);	
	m_ui.ChannelSoloPushButton->setPalette(pal);
	m_ui.ChannelSoloPushButton->setDown(bSolo);
#else
	m_ui.ChannelMutePushButton->setEnabled(false);
	m_ui.ChannelSoloPushButton->setEnabled(false);
#endif

	// And update the both GUI volume elements;
	// return success if, and only if, intrument is fully loaded...
	return updateChannelVolume() && (iInstrumentStatus == 100);
}


// Update whole channel usage state.
bool ChannelStrip::updateChannelUsage (void)
{
	if (m_pChannel == NULL)
		return false;

	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm->client() == NULL)
		return false;

	// This only makes sense on fully loaded channels...
	if (m_pChannel->instrumentStatus() < 100)
		return false;

	// Get current channel voice count.
	int iVoiceCount  = ::lscp_get_channel_voice_count(
		pMainForm->client(), m_pChannel->channelID());
// Get current stream count.
	int iStreamCount = ::lscp_get_channel_stream_count(
		pMainForm->client(), m_pChannel->channelID());
	// Get current channel buffer fill usage.
	// As benno has suggested this is the percentage usage
	// of the least filled buffer stream...
	int iStreamUsage = ::lscp_get_channel_stream_usage(
		pMainForm->client(), m_pChannel->channelID());;

	// Update the GUI elements...
	m_ui.StreamUsageProgressBar->setValue(iStreamUsage);
	m_ui.StreamVoiceCountTextLabel->setText(
		QString("%1 / %2").arg(iStreamCount).arg(iVoiceCount));

	// We're clean.
	return true;
}


// Volume change slot.
void ChannelStrip::volumeChanged ( int iVolume )
{
	if (m_pChannel == NULL)
		return;

	// Avoid recursion.
	if (m_iDirtyChange > 0)
		return;

	// Convert and clip.
	float fVolume = (float) iVolume / 100.0f;
	if (fVolume < 0.001f)
		fVolume = 0.0f;

	// Update the GUI elements.
	if (m_pChannel->setVolume(fVolume)) {
		updateChannelVolume();
		emit channelChanged(this);
	}
}


// Context menu event handler.
void ChannelStrip::contextMenuEvent( QContextMenuEvent *pEvent )
{
	if (m_pChannel == NULL)
		return;

	// We'll just show up the main form's edit menu (thru qsamplerChannel).
	m_pChannel->contextMenuEvent(pEvent);
}


// Error count hackish accessors.
void ChannelStrip::resetErrorCount (void)
{
	m_iErrorCount = 0;
}


// Channel strip activation/selection.
void ChannelStrip::setSelected ( bool bSelected )
{
	if (bSelected) {
		if (g_pSelectedStrip == this)
			return;
		if (g_pSelectedStrip)
			g_pSelectedStrip->setSelected(false);
		g_pSelectedStrip = this;
	} else {
		if (g_pSelectedStrip == this)
			g_pSelectedStrip = NULL;
	}

	QPalette pal;
	if (bSelected) {
		const QColor& color = pal.midlight().color();
		pal.setColor(QPalette::Background, color.dark(150));
		pal.setColor(QPalette::Foreground, color.light(150));
	}
	QWidget::setPalette(pal);
}


bool ChannelStrip::isSelected (void) const
{
	return (this == g_pSelectedStrip);
}


} // namespace QSampler


// end of qsamplerChannelStrip.cpp
