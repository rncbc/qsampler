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
#include <qfiledialog.h>

#include "qsamplerMainForm.h"
#include "qsamplerChannelForm.h"

#include "config.h"


// Kind of constructor.
void qsamplerChannelStrip::init (void)
{
    // Initialize locals.
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
    m_pMainForm = pMainForm;

    setChannelID(iChannelID);
    
    // Read channel information.
    lscp_channel_info_t *pChannelInfo = ::lscp_get_channel_info(m_pMainForm->client(), m_iChannelID);
    if (pChannelInfo == NULL) {
        m_pMainForm->appendMessagesClient("lscp_get_channel_info");
        m_pMainForm->appendMessagesError(tr("Could not get channel information. Sorry."));
        return;
    }
    // Set some proper values.
    EngineNameTextLabel->setText(pChannelInfo->engine_name);
    InstrumentNameTextLabel->setText(pChannelInfo->instrument_file);
    int iVolume = (int) (100.0 * pChannelInfo->volume);
    if (iVolume > 100)
        iVolume = 100;
    VolumeSlider->setValue(iVolume);
    VolumeSpinBox->setValue(iVolume);
}


// Channel-ID (aka Sammpler-Channel) accessors.
int qsamplerChannelStrip::channelID (void)
{
    return m_iChannelID;
}

void qsamplerChannelStrip::setChannelID ( int iChannelID )
{
    m_iChannelID = iChannelID;

    QString sText = tr("Channel %1").arg(m_iChannelID);
    setCaption(sText);
    ChannelSetupPushButton->setText(sText);
}


// Channel setup dialog.
void qsamplerChannelStrip::channelSetup (void)
{
    if (m_pMainForm == NULL)
        return;
    if (m_pMainForm->options() == NULL || m_pMainForm->client() == NULL)
        return;
    
    qsamplerChannelForm *pChannelForm = new qsamplerChannelForm(this);
    if (pChannelForm) {
        pChannelForm->setup(this);
        if (pChannelForm->exec())
            emit channelChanged(this);
        delete pChannelForm;
    }
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


// end of qsamplerChannelStrip.ui.h

