// qsamplerChannelStrip.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2004-2005, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#include <qvalidator.h>
#include <qmessagebox.h>
#include <qdragobject.h>
#include <qfileinfo.h>
#include <qtooltip.h>
#include <qpopupmenu.h>
#include <qobjectlist.h>

#include <math.h>

#include "qsamplerMainForm.h"
#include "qsamplerChannelForm.h"

#include "config.h"

// Channel status/usage usage limit control.
#define QSAMPLER_ERROR_LIMIT	3


// Kind of constructor.
void qsamplerChannelStrip::init (void)
{
    // Initialize locals.
    m_pChannel     = NULL;
    m_iDirtyChange = 0;
	m_iErrorCount  = 0;

    // Try to restore normal window positioning.
    adjustSize();
}


// Kind of destructor.
void qsamplerChannelStrip::destroy (void)
{
    // Destroy existing channel descriptor.
    if (m_pChannel)
        delete m_pChannel;
    m_pChannel = NULL;
}


// Drag'n'drop file handler.
bool qsamplerChannelStrip::decodeDragFile ( const QMimeSource *pEvent, QString& sInstrumentFile )
{
	if (m_pChannel == NULL)
		return false;

	if (QTextDrag::canDecode(pEvent)) {
		QString sText;
		if (QTextDrag::decode(pEvent, sText)) {
			QStringList files = QStringList::split('\n', sText);
			for (QStringList::Iterator iter = files.begin(); iter != files.end(); iter++) {
				*iter = (*iter).stripWhiteSpace().replace(QRegExp("^file:"), QString::null);
				if (qsamplerChannel::isInstrumentFile(*iter)) {
					sInstrumentFile = *iter;
					return true;
				}
			}
		}
	}
	// Fail.
	return false;
}


// Window drag-n-drop event handlers.
void qsamplerChannelStrip::dragEnterEvent ( QDragEnterEvent* pDragEnterEvent )
{
	QString sInstrumentFile;
	pDragEnterEvent->accept(decodeDragFile(pDragEnterEvent, sInstrumentFile));
}


void qsamplerChannelStrip::dropEvent ( QDropEvent* pDropEvent )
{
	QString sInstrumentFile;

	if (decodeDragFile(pDropEvent, sInstrumentFile)) {
		// Go and set the dropped instrument filename...
		m_pChannel->setInstrument(sInstrumentFile, 0);
		// Open up the channel dialog.
		channelSetup();
	}
}


// Channel strip setup formal initializer.
void qsamplerChannelStrip::setup ( qsamplerChannel *pChannel )
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
qsamplerChannel *qsamplerChannelStrip::channel (void)
{
    return m_pChannel;
}


// Messages view font accessors.
QFont qsamplerChannelStrip::displayFont (void)
{
    return EngineNameTextLabel->font();
}

void qsamplerChannelStrip::setDisplayFont ( const QFont & font )
{
    EngineNameTextLabel->setFont(font);
    MidiPortChannelTextLabel->setFont(font);
    InstrumentNameTextLabel->setFont(font);
    InstrumentStatusTextLabel->setFont(font);
}


// Channel display background effect.
void qsamplerChannelStrip::setDisplayEffect ( bool bDisplayEffect )
{
    QPixmap pm;
    if (bDisplayEffect)
        pm = QPixmap::fromMimeSource("displaybg1.png");
    setDisplayBackground(pm);
}


// Update main display background pixmap.
void qsamplerChannelStrip::setDisplayBackground ( const QPixmap& pm )
{
    // Set the main origin...
    ChannelInfoFrame->setPaletteBackgroundPixmap(pm);

    // Iterate for every child text label...
    QObjectList *pList = ChannelInfoFrame->queryList("QLabel");
    if (pList) {
        for (QLabel *pLabel = (QLabel *) pList->first(); pLabel; pLabel = (QLabel *) pList->next())
            pLabel->setPaletteBackgroundPixmap(pm);
        delete pList;
    }
    
    // And this standalone too.
    StreamVoiceCountTextLabel->setPaletteBackgroundPixmap(pm);
}


// Maximum volume slider accessors.
void qsamplerChannelStrip::setMaxVolume ( int iMaxVolume )
{
    m_iDirtyChange++;
    VolumeSlider->setRange(0, iMaxVolume);
    VolumeSpinBox->setRange(0, iMaxVolume);
    m_iDirtyChange--;
}


// Channel setup dialog slot.
bool qsamplerChannelStrip::channelSetup (void)
{
	if (m_pChannel == NULL)
		return false;
		
	// Invoke the channel setup dialog.
	bool bResult = m_pChannel->channelSetup(this);
	// Notify that thie channel has changed.
	if (bResult)
		emit channelChanged(this);

	return bResult;
}


// Channel reset slot.
bool qsamplerChannelStrip::channelReset (void)
{
	if (m_pChannel == NULL)
		return false;

	// Invoke the channel reset method.
	bool bResult = m_pChannel->channelReset();
	// Notify that thie channel has changed.
	if (bResult)
		emit channelChanged(this);

	return bResult;
}


// Update the channel instrument name.
bool qsamplerChannelStrip::updateInstrumentName ( bool bForce )
{
	if (m_pChannel == NULL)
		return false;

	// Do we refersh the actual name?
	if (bForce)
		m_pChannel->updateInstrumentName();

	// Instrument name...
	if (m_pChannel->instrumentName().isEmpty())
		InstrumentNameTextLabel->setText(' ' + qsamplerChannel::noInstrumentName());
	else
		InstrumentNameTextLabel->setText(' ' + m_pChannel->instrumentName());

	return true;    
}


// Do the dirty volume change.
bool qsamplerChannelStrip::updateChannelVolume (void)
{
    if (m_pChannel == NULL)
        return false;

    // Convert...
#ifdef CONFIG_ROUND
    int iVolume = (int) ::round(100.0 * m_pChannel->volume());
#else
    double fIPart = 0.0;
    double fFPart = ::modf(100.0 * m_pChannel->volume(), &fIPart);
    int iVolume = (int) fIPart;
    if (fFPart >= +0.5)
        iVolume++;
    else
    if (fFPart <= -0.5)
        iVolume--;
#endif

    // And clip...
    if (iVolume < 0)
        iVolume = 0;

    // Flag it here, to avoid infinite recursion.
    m_iDirtyChange++;
    VolumeSlider->setValue(iVolume);
    VolumeSpinBox->setValue(iVolume);
    m_iDirtyChange--;

    return true;
}


// Update whole channel info state.
bool qsamplerChannelStrip::updateChannelInfo (void)
{
    if (m_pChannel == NULL)
        return false;
        
	// Check for error limit/recycle...
	if (m_iErrorCount > QSAMPLER_ERROR_LIMIT)
		return true;

    // Update strip caption.
    QString sText = m_pChannel->channelName();
    setCaption(sText);
    ChannelSetupPushButton->setText(sText);

    // Check if we're up and connected.
    if (m_pChannel->client() == NULL)
        return false;

    // Read actual channel information.
    m_pChannel->updateChannelInfo();

    // Engine name...
    if (m_pChannel->engineName().isEmpty())
        EngineNameTextLabel->setText(' ' + qsamplerChannel::noEngineName());
    else
        EngineNameTextLabel->setText(' ' + m_pChannel->engineName());

	// Instrument name...
	updateInstrumentName(false);

    // MIDI Port/Channel...
    if (m_pChannel->midiChannel() == LSCP_MIDI_CHANNEL_ALL)
        MidiPortChannelTextLabel->setText(QString("%1 / *").arg(m_pChannel->midiPort()));
    else
        MidiPortChannelTextLabel->setText(QString("%1 / %2").arg(m_pChannel->midiPort()).arg(m_pChannel->midiChannel() + 1));

    // Instrument status...
    int iInstrumentStatus = m_pChannel->instrumentStatus();
    if (iInstrumentStatus < 0) {
        InstrumentStatusTextLabel->setPaletteForegroundColor(Qt::red);
        InstrumentStatusTextLabel->setText(tr("ERR%1").arg(iInstrumentStatus));
        m_iErrorCount++;
        return false;
    }
    // All seems normal...
    InstrumentStatusTextLabel->setPaletteForegroundColor(iInstrumentStatus < 100 ? Qt::yellow : Qt::green);
    InstrumentStatusTextLabel->setText(QString::number(iInstrumentStatus) + '%');
    m_iErrorCount = 0;

    // And update the both GUI volume elements;
    // return success if, and only if, intrument is fully loaded...
    return updateChannelVolume() && (iInstrumentStatus == 100);
}


// Update whole channel usage state.
bool qsamplerChannelStrip::updateChannelUsage (void)
{
    if (m_pChannel == NULL)
        return false;
    if (m_pChannel->client() == NULL)
        return false;

	// This only makes sense on fully loaded channels...
	if (m_pChannel->instrumentStatus() < 100)
	    return false;
	    
    // Get current channel voice count.
    int iVoiceCount  = ::lscp_get_channel_voice_count(m_pChannel->client(), m_pChannel->channelID());
    // Get current stream count.
    int iStreamCount = ::lscp_get_channel_stream_count(m_pChannel->client(), m_pChannel->channelID());
    // Get current channel buffer fill usage.
    // As benno has suggested this is the percentage usage
    // of the least filled buffer stream...
    int iStreamUsage = ::lscp_get_channel_stream_usage(m_pChannel->client(), m_pChannel->channelID());;

    // Update the GUI elements...
    StreamUsageProgressBar->setProgress(iStreamUsage);
    StreamVoiceCountTextLabel->setText(QString("%1 / %2").arg(iStreamCount).arg(iVoiceCount));
    
    // We're clean.
    return true;
}


// Volume change slot.
void qsamplerChannelStrip::volumeChanged ( int iVolume )
{
    if (m_pChannel == NULL)
        return;

    // Avoid recursion.
    if (m_iDirtyChange > 0)
        return;

    // Convert and clip.
    float fVolume = (float) iVolume / 100.0;
    if (fVolume < 0.001)
        fVolume = 0.0;

    // Update the GUI elements.
    if (m_pChannel->setVolume(fVolume)) {
        updateChannelVolume();
        emit channelChanged(this);
    }
}


// Context menu event handler.
void qsamplerChannelStrip::contextMenuEvent( QContextMenuEvent *pEvent )
{
    if (m_pChannel == NULL)
        return;
        
    // We'll just show up the main form's edit menu (thru qsamplerChannel).
    m_pChannel->contextMenuEvent(pEvent);
}


// end of qsamplerChannelStrip.ui.h
