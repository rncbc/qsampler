// qsamplerChannelStrip.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2004, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include <qfileinfo.h>
#include <qtooltip.h>
#include <qpopupmenu.h>

#include <math.h>

#include "qsamplerMainForm.h"
#include "qsamplerChannelForm.h"

#include "config.h"


// Kind of constructor.
void qsamplerChannelStrip::init (void)
{
    // Initialize locals.
    m_pMainForm  = NULL;
    m_iChannelID = 0;
    
//  m_sEngineName     = "(No engine)";
//  m_sInstrumentFile = "(No instrument)";
    m_iInstrumentNr   = 0;
    m_sMidiDriver     = "ALSA"; // DEPRECATED.
    m_iMidiDevice     = 0;
    m_iMidiPort       = 0;
    m_iMidiChannel    = 0;
    m_sAudioDriver    = "ALSA"; // DEPRECATED.
    m_iAudioDevice    = 0;
    m_fVolume         = 0.8;
    
    m_iDirtyChange = 0;

    // Try to restore normal window positioning.
    adjustSize();
}


// Kind of destructor.
void qsamplerChannelStrip::destroy (void)
{
}


// Channel strip setup formal initializer.
void qsamplerChannelStrip::setup ( qsamplerMainForm *pMainForm, int iChannelID )
{
    m_iDirtyChange = 0;
    m_pMainForm = pMainForm;

    setChannelID(iChannelID);
}


// The global options settings delegated property.
qsamplerOptions *qsamplerChannelStrip::options (void)
{
    if (m_pMainForm == NULL)
        return NULL;
        
    return m_pMainForm->options();
}


// The client descriptor delegated property.
lscp_client_t *qsamplerChannelStrip::client (void)
{
    if (m_pMainForm == NULL)
        return NULL;

    return m_pMainForm->client();
}


// Channel-ID (aka Sammpler-Channel) accessors.
int qsamplerChannelStrip::channelID (void)
{
    return m_iChannelID;
}

void qsamplerChannelStrip::setChannelID ( int iChannelID )
{
    m_iChannelID = iChannelID;

    updateChannelInfo();
}


// Engine name accessors.
QString& qsamplerChannelStrip::engineName (void)
{
    return m_sEngineName;
}

bool qsamplerChannelStrip::loadEngine ( const QString& sEngineName )
{
    if (client() == NULL)
        return false;

    if (::lscp_load_engine(client(), sEngineName.latin1(), m_iChannelID) != LSCP_OK) {
        appendMessagesClient("lscp_load_engine");
        return false;
    }

    m_sEngineName = sEngineName;
    return true;
}


// Instrument filename accessors.
QString& qsamplerChannelStrip::instrumentFile (void)
{
    return m_sInstrumentFile;
}

// Instrument index accessors.
int qsamplerChannelStrip::instrumentNr (void)
{
    return m_iInstrumentNr;
}

bool qsamplerChannelStrip::loadInstrument ( const QString& sInstrumentFile, int iInstrumentNr )
{
    if (client() == NULL)
        return false;

    if (::lscp_load_instrument(client(), sInstrumentFile.latin1(), iInstrumentNr, m_iChannelID) != LSCP_OK) {
        appendMessagesClient("lscp_load_instrument");
        return false;
    }

    m_sInstrumentFile = sInstrumentFile;
    m_iInstrumentNr = iInstrumentNr;
    return true;
}


// MIDI driver type accessors (DEPRECATED).
QString& qsamplerChannelStrip::midiDriver (void)
{
    return m_sMidiDriver;
}

bool qsamplerChannelStrip::setMidiDriver ( const QString& sMidiDriver )
{
    if (client() == NULL)
        return false;

    if (::lscp_set_channel_midi_type(client(), m_iChannelID, sMidiDriver.latin1()) != LSCP_OK) {
        appendMessagesClient("lscp_set_channel_midi_type");
        return false;
    }

    m_sMidiDriver = sMidiDriver;
    return true;
}


// MIDI device accessors.
int qsamplerChannelStrip::midiDevice (void)
{
    return m_iMidiDevice;
}

bool qsamplerChannelStrip::setMidiDevice ( int iMidiDevice )
{
    if (client() == NULL)
        return false;

//  FIXME: call this when LSCP becomes stable.
//  if (::lscp_set_channel_midi_device(client(), m_iChannelID, iMidiDevice) != LSCP_OK) {
//      appendMessagesClient("lscp_set_channel_midi_device");
//      return false;
//  }

    m_iMidiDevice = iMidiDevice;
    return true;
}


// MIDI port number accessor.
int qsamplerChannelStrip::midiPort (void)
{
    return m_iMidiPort;
}

bool qsamplerChannelStrip::setMidiPort ( int iMidiPort )
{
    if (client() == NULL)
        return false;

//  FIXME: call this when LSCP becomes stable.
//  if (::lscp_set_channel_midi_port(client(), m_iChannelID, iMidiPort) != LSCP_OK) {
//      appendMessagesClient("lscp_set_channel_midi_port");
//      return false;
//  }

    m_iMidiPort = iMidiPort;
    return true;
}


// MIDI channel accessor.
int qsamplerChannelStrip::midiChannel (void)
{
    return m_iMidiChannel;
}

bool qsamplerChannelStrip::setMidiChannel ( int iMidiChannel )
{
    if (client() == NULL)
        return false;

//  FIXME: call this when LSCP becomes stable.
//  if (::lscp_set_channel_midi_channel(client(), m_iChannelID, iMidiChannel) != LSCP_OK) {
//      appendMessagesClient("lscp_set_channel_midi_channel");
//      return false;
//  }

    m_iMidiChannel = iMidiChannel;
    return true;
}


// Audio device accessor.
int qsamplerChannelStrip::audioDevice (void)
{
    return m_iAudioDevice;
}

bool qsamplerChannelStrip::setAudioDevice ( int iAudioDevice )
{
    if (client() == NULL)
        return false;

//  FIXME: call this when LSCP becomes stable.
//  if (::lscp_set_channel_audio_device(client(), m_iChannelID, iAudioDevice) != LSCP_OK) {
//      appendMessagesClient("lscp_set_channel_audio_device");
//      return false;
//  }

    m_iAudioDevice = iAudioDevice;
    return true;
}


// Audio driver type accessors (DEPRECATED).
QString& qsamplerChannelStrip::audioDriver (void)
{
    return m_sAudioDriver;
}

bool qsamplerChannelStrip::setAudioDriver ( const QString& sAudioDriver )
{
    if (client() == NULL)
        return false;

    if (::lscp_set_channel_audio_type(client(), m_iChannelID, sAudioDriver.latin1()) != LSCP_OK) {
        appendMessagesClient("lscp_set_channel_audio_type");
        return false;
    }

    m_sAudioDriver = sAudioDriver;
    return true;
}


// Channel volume accessors.
float qsamplerChannelStrip::volume (void)
{
    return m_fVolume;
}

bool qsamplerChannelStrip::setVolume ( float fVolume )
{
    if (client() == NULL)
        return false;

    if (::lscp_set_channel_volume(client(), m_iChannelID, fVolume) != LSCP_OK) {
        appendMessagesClient("lscp_set_channel_volume");
        return false;
    }

    m_fVolume = fVolume;
    return true;
}


// Messages view font accessors.
QFont qsamplerChannelStrip::displayFont (void)
{
    return EngineNameTextLabel->font();
}

void qsamplerChannelStrip::setDisplayFont ( const QFont & font )
{
    EngineNameTextLabel->setFont(font);
    InstrumentNameTextLabel->setFont(font);
    MidiPortChannelTextLabel->setFont(font);
}

// Channel setup dialog.
void qsamplerChannelStrip::channelSetup (void)
{
    qsamplerChannelForm *pChannelForm = new qsamplerChannelForm(this);
    if (pChannelForm) {
        pChannelForm->setup(this);
        if (pChannelForm->exec()) {
            updateChannelInfo();
            emit channelChanged(this);
        }
        delete pChannelForm;
    }
}


// Update whole channel info state.
void qsamplerChannelStrip::updateChannelInfo (void)
{
    // Update strip caption.
    QString sText = tr("Channel %1").arg(m_iChannelID);
    setCaption(sText);
    ChannelSetupPushButton->setText(sText);

    // Check if we're up and connected.
    if (client() == NULL)
        return;

    // Read channel information.
    lscp_channel_info_t *pChannelInfo = ::lscp_get_channel_info(client(), m_iChannelID);
    if (pChannelInfo == NULL) {
        appendMessagesClient("lscp_get_channel_info");     
    //  appendMessagesError(tr("Could not get channel information.\n\nSorry."));
    } else {
        // Cache in channel information.
        m_sEngineName     = pChannelInfo->engine_name;
        m_sInstrumentFile = pChannelInfo->instrument_file;
        m_iInstrumentNr   = pChannelInfo->instrument_nr;
        m_iMidiDevice     = pChannelInfo->midi_device;
        m_iMidiPort       = pChannelInfo->midi_port;
        m_iMidiChannel    = pChannelInfo->midi_channel;
        m_iAudioDevice    = pChannelInfo->audio_device;
        m_fVolume         = pChannelInfo->volume;
    }

    // Set some proper display values.

    // Engine name...
    if (m_sEngineName.isEmpty())
        EngineNameTextLabel->setText(tr("(No engine)"));
    else
        EngineNameTextLabel->setText(m_sEngineName);

    // Instrument name...
    if (m_sInstrumentFile.isEmpty())
        InstrumentNameTextLabel->setText(tr("(No instrument)"));
    else
        InstrumentNameTextLabel->setText(QString("%1 [%2]")
            .arg(QFileInfo(m_sInstrumentFile).fileName()).arg(m_iInstrumentNr));

    // MIDI Port/Channel...
    MidiPortChannelTextLabel->setText(QString("%1 / %2")
        .arg(m_iMidiPort).arg(m_iMidiChannel));
        
    // And update the both GUI volume elements.
    updateChannelVolume();
}


// Do the dirty volume change.
void qsamplerChannelStrip::updateChannelVolume (void)
{
    // Convert...
#ifdef CONFIG_ROUND
    int iVolume = (int) ::round(100.0 * m_fVolume);
#else
    double fIPart = 0.0;
    double fFPart = ::modf(100.0 * m_fVolume, &fIPart);
    int iVolume = (int) fIPart;
    if (fFPart >= +0.5)
        iVolume++;
    else
    if (fFPart <= -0.5)
        iVolume--;
#endif

    // And clip...
    if (iVolume > 100)
        iVolume = 100;
    else if (iVolume < 0)
        iVolume = 0;

    // Flag it here, to avoid infinite recursion.
    m_iDirtyChange++;
    VolumeSlider->setValue(iVolume);
    VolumeSpinBox->setValue(iVolume);
    m_iDirtyChange--;
}


// Update whole channel usage state.
void qsamplerChannelStrip::updateChannelUsage (void)
{
    if (client() == NULL)
        return;

    // Get current channel voice count.
    int iVoiceCount  = ::lscp_get_channel_voice_count(client(), m_iChannelID);
    // Get current stream count.
    int iStreamCount = ::lscp_get_channel_stream_count(client(), m_iChannelID);
    // Get current channel buffer fill usage.
    // As benno has suggested this is the percentage usage
    // of the least filled buffer stream...
    int iStreamUsage = 0;
    if (iStreamCount > 0) {
        lscp_buffer_fill_t *pBufferFill = ::lscp_get_channel_buffer_fill(client(), LSCP_USAGE_PERCENTAGE, m_iChannelID);
        if (pBufferFill) {
            for (int iStream = 0; iStream < iStreamCount; iStream++) {
                if (iStreamUsage > (int) pBufferFill[iStream].stream_usage || iStream == 0)
                    iStreamUsage = pBufferFill[iStream].stream_usage;
            }
        }
    }    
    // Update the GUI elements...
    StreamUsageProgressBar->setProgress(iStreamUsage);
    StreamVoiceCountTextLabel->setText(QString("%1 / %2").arg(iStreamCount).arg(iVoiceCount));
}


// Volume change slot.
void qsamplerChannelStrip::volumeChanged ( int iVolume )
{
    // Avoid recursion.
    if (m_iDirtyChange > 0)
        return;

    // Convert and clip.
    float fVolume = (float) iVolume / 100.0;
    if (fVolume > 1.0)
        fVolume = 1.0;
    else if (fVolume < 0.0)
        fVolume = 0.0;

    // Update the GUI elements.
    if (setVolume(fVolume)) {
        updateChannelVolume();
        emit channelChanged(this);
    }
}


// Redirected messages output methods.
void qsamplerChannelStrip::appendMessages( const QString& s )
{
    m_pMainForm->appendMessages(s);
}

void qsamplerChannelStrip::appendMessagesColor( const QString& s, const QString& c )
{
    m_pMainForm->appendMessagesColor(s, c);
}

void qsamplerChannelStrip::appendMessagesText( const QString& s )
{
    m_pMainForm->appendMessagesText(s);
}

void qsamplerChannelStrip::appendMessagesError( const QString& s )
{
    m_pMainForm->appendMessagesError(s);
}

void qsamplerChannelStrip::appendMessagesClient( const QString& s )
{
    m_pMainForm->appendMessagesClient(s);
}


// Context menu event handler.
void qsamplerChannelStrip::contextMenuEvent( QContextMenuEvent *pEvent )
{
    // We'll just show up the main form's edit menu.
    m_pMainForm->stabilizeForm();
    m_pMainForm->editMenu->exec(pEvent->globalPos());
}


// end of qsamplerChannelStrip.ui.h

