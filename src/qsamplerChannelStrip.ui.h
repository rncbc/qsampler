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

#include <math.h>

#include "qsamplerMainForm.h"
#include "qsamplerChannelForm.h"

#include "config.h"


// Kind of constructor.
void qsamplerChannelStrip::init (void)
{
    // Initialize locals.
    m_iDirtyChange = 0;
    m_pMainForm = NULL;
    m_iChannelID = 0;

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
    return m_pMainForm->options();
}


// The client descriptor delegated property.
lscp_client_t *qsamplerChannelStrip::client (void)
{
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

    updateChannel();
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
}

// Channel setup dialog.
void qsamplerChannelStrip::channelSetup (void)
{
    qsamplerChannelForm *pChannelForm = new qsamplerChannelForm(this);
    if (pChannelForm) {
        pChannelForm->setup(this);
        if (pChannelForm->exec()) {
            updateChannel();
            emit channelChanged(this);
        }
        delete pChannelForm;
    }
}



// Do the dirty volume change.
void qsamplerChannelStrip::setChannelVolume ( float fVolume )
{
    // Convert...
#ifdef CONFIG_ROUND
    int iVolume = (int) ::round(100.0 * fVolume);
#else
    double fIPart = 0.0;
    double fFPart = ::modf(100.0 * fVolume, &fIPart);
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


// Volume change slot.
void qsamplerChannelStrip::volumeChanged ( int iVolume )
{
    if (m_pMainForm->client() == NULL)
        return;
    // Avoid recursion.
    if (m_iDirtyChange > 0)
        return;
        
    // Convert and clip.
    float fVolume = (float) iVolume / 100.0;
    if (fVolume > 1.0)
        fVolume = 1.0;
    else if (fVolume < 0.0)
        fVolume = 0.0;

    // Do it for real.
    if (::lscp_set_channel_volume(m_pMainForm->client(), m_iChannelID, fVolume) != LSCP_OK) {
        appendMessagesClient("lscp_set_channel_volume");
        appendMessagesError(tr("Could not set channel volume. Sorry."));
    }
    
    // And update the GUI elements.
    setChannelVolume(fVolume);
}


// Update whole channel info state.
void qsamplerChannelStrip::updateChannel (void)
{
    // Update strip caption.
    QString sText = tr("Channel %1").arg(m_iChannelID);
    setCaption(sText);
    ChannelSetupPushButton->setText(sText);

    // Check if we're up and connected.
    if (m_pMainForm->client() == NULL)
        return;

    // Read channel information.
    lscp_channel_info_t *pChannelInfo = ::lscp_get_channel_info(m_pMainForm->client(), m_iChannelID);
    if (pChannelInfo == NULL) {
        appendMessagesClient("lscp_get_channel_info");
        appendMessagesError(tr("Could not get channel information. Sorry."));
        return;
    }
    // Set some proper values.
    EngineNameTextLabel->setText(pChannelInfo->engine_name);
    InstrumentNameTextLabel->setText(QFileInfo(pChannelInfo->instrument_file).fileName()
         + " [" + QString::number(pChannelInfo->instrument_nr) + "]");
    // And update the both volume elements.
    setChannelVolume(pChannelInfo->volume);
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


// end of qsamplerChannelStrip.ui.h

