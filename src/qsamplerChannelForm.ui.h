// qsamplerChannelForm.ui.h
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
#include <qfileinfo.h>
#include <qlistbox.h>

#include "qsamplerOptions.h"
#include "qsamplerChannelStrip.h"

#include "config.h"


// Kind of constructor.
void qsamplerChannelForm::init (void)
{
    // Initialize locals.
    m_pChannel = NULL;
    m_iDirtySetup = 0;
    m_iDirtyCount = 0;

    // Try to restore normal window positioning.
    adjustSize();
}


// Kind of destructor.
void qsamplerChannelForm::destroy (void)
{
}


// Channel dialog setup formal initializer.
void qsamplerChannelForm::setup ( qsamplerChannelStrip *pChannel )
{
    m_pChannel = pChannel;
    m_iDirtySetup = 0;
    m_iDirtyCount = 0;

    setCaption(m_pChannel->caption());

    // Check if we're up and connected.
    if (m_pChannel->client() == NULL)
        return;

    qsamplerOptions *pOptions = m_pChannel->options();
    if (pOptions == NULL)
        return;

    // Avoid nested changes.
    m_iDirtySetup++;

    // Load combo box history...
    pOptions->loadComboBoxHistory(InstrumentFileComboBox);

    // Populate Engines list.
    const char **ppszEngines = ::lscp_get_available_engines(m_pChannel->client());
    if (ppszEngines) {
        EngineNameComboBox->clear();
        for (int iEngine = 0; ppszEngines[iEngine]; iEngine++)
            EngineNameComboBox->insertItem(ppszEngines[iEngine]);
    }
    else m_pChannel->appendMessagesClient("lscp_get_available_engines");

    // Populate Audio output type list.
    const char **ppszAudioDrivers = ::lscp_get_available_audio_drivers(m_pChannel->client());
    if (ppszAudioDrivers) {
        AudioDriverComboBox->clear();
        for (int iAudioDriver = 0; ppszAudioDrivers[iAudioDriver]; iAudioDriver++)
            AudioDriverComboBox->insertItem(ppszAudioDrivers[iAudioDriver]);
    }
    else m_pChannel->appendMessagesClient("lscp_get_available_audio_drivers");

    // Populate MIDI input type list.
    const char **ppszMidiDrivers = ::lscp_get_available_midi_drivers(m_pChannel->client());
    if (ppszMidiDrivers) {
        MidiDriverComboBox->clear();
        for (int iMidiDriver = 0; ppszMidiDrivers[iMidiDriver]; iMidiDriver++)
            MidiDriverComboBox->insertItem(ppszMidiDrivers[iMidiDriver]);
    }
    else m_pChannel->appendMessagesClient("lscp_get_available_midi_drivers");

    // Read proper channel information,
    // and populate the channel form fields.
    lscp_channel_info_t *pChannelInfo = ::lscp_get_channel_info(m_pChannel->client(), m_pChannel->channelID());
    if (pChannelInfo) {
        // Engine name...
        if (EngineNameComboBox->listBox()->findItem(pChannelInfo->engine_name, Qt::ExactMatch) == NULL)
            EngineNameComboBox->insertItem(pChannelInfo->engine_name);
        EngineNameComboBox->setCurrentText(pChannelInfo->engine_name);
        // Instrument filename and index...
        InstrumentFileComboBox->setCurrentText(pChannelInfo->instrument_file);
        InstrumentNrSpinBox->setValue(pChannelInfo->instrument_nr);
        // MIDI input...
    //  -- TODO: Get MIDI driver from MIDI device (?midi_driver <- pChannelInfo->midi_device).
    //  if (MidiDriverComboBox->listBox()->findItem(?midi_driver, Qt::ExactMatch) == NULL)
    //      MidiDriverComboBox->insertItem(?midi_driver);
    //  MidiDriverComboBox->setCurrentText(?midi_driver);
        MidiPortSpinBox->setValue(pChannelInfo->midi_port);
        MidiChannelSpinBox->setValue(pChannelInfo->midi_channel);
        // Audio output...
    //  -- TODO: Get Audio driver from Audio device (?audio_driver <- pChannelInfo->audio_device).
    //  if (AudioDriverComboBox->listBox()->findItem(?audio_driver, Qt::ExactMatch) == NULL)
    //      AudioDriverComboBox->insertItem(?audio_driver);
    //  AudioDriverComboBox->setCurrentText(?audio_driver);
    } else {
        m_pChannel->appendMessagesClient("lscp_get_channel_info");
        m_pChannel->appendMessagesError(tr("Could not get channel information.\n\nSorry."));
    }

    // Done.
    m_iDirtySetup--;
    stabilizeForm();
}


// Accept settings (OK button slot).
void qsamplerChannelForm::accept (void)
{
    // Check if we're up and connected.
    if (m_pChannel->client() == NULL)
        return;

    qsamplerOptions *pOptions = m_pChannel->options();
    if (pOptions == NULL)
        return;

    // We'll go for it!
    if (m_iDirtyCount > 0) {
        int iErrors = 0;
        // Engine name...
        if (::lscp_load_engine(m_pChannel->client(),
            EngineNameComboBox->currentText().latin1(),
            m_pChannel->channelID()) != LSCP_OK) {
            m_pChannel->appendMessagesClient("lscp_load_engine");
            iErrors++;
        }
        // MIDI input type...
        if (::lscp_set_channel_midi_type(m_pChannel->client(),
            m_pChannel->channelID(),
            MidiDriverComboBox->currentText().latin1()) != LSCP_OK) {
            m_pChannel->appendMessagesClient("lscp_set_channel_midi_type");
            iErrors++;
        }
        // MIDI input port number...
        if (::lscp_set_channel_midi_port(m_pChannel->client(),
            m_pChannel->channelID(),
            MidiPortSpinBox->value()) != LSCP_OK) {
            m_pChannel->appendMessagesClient("lscp_set_channel_midi_port");
            iErrors++;
        }
        // MIDI input channel (0=ALL)...
        if (::lscp_set_channel_midi_channel(m_pChannel->client(),
            m_pChannel->channelID(),
            MidiChannelSpinBox->value()) != LSCP_OK) {
            m_pChannel->appendMessagesClient("lscp_set_channel_midi_channel");
            iErrors++;
        }
        // Audio output type...
        if (::lscp_set_channel_audio_type(m_pChannel->client(),
            m_pChannel->channelID(),
            AudioDriverComboBox->currentText().latin1()) != LSCP_OK) {
            m_pChannel->appendMessagesClient("lscp_set_channel_audio_port");
            iErrors++;
        }
        // Instrument file and index...
        if (::lscp_load_instrument(m_pChannel->client(),
            InstrumentFileComboBox->currentText().latin1(),
            InstrumentNrSpinBox->value(),
            m_pChannel->channelID()) != LSCP_OK) {
            m_pChannel->appendMessagesClient("lscp_load_instrument");
            iErrors++;
        }
        // Show error messages?
        if (iErrors > 0)
            m_pChannel->appendMessagesError(tr("Some channel settings could not be set.\n\nSorry."));
    }

    // Save default instrument directory and history...
    pOptions->sInstrumentDir = QFileInfo(InstrumentFileComboBox->currentText()).dirPath(true);
    pOptions->saveComboBoxHistory(InstrumentFileComboBox);

    // Just go with dialog acceptance.
    QDialog::accept();
}


// Reject settings (Cancel button slot).
void qsamplerChannelForm::reject (void)
{
    bool bReject = true;

    // Check if there's any pending changes...
    if (m_iDirtyCount > 0) {
        switch (QMessageBox::warning(this, tr("Warning"),
            tr("Some settings have been changed.") + "\n\n" +
            tr("Do you want to apply the changes?"),
            tr("Apply"), tr("Discard"), tr("Cancel"))) {
        case 0:     // Apply...
            accept();
            return;
        case 1:     // Discard
            break;
        default:    // Cancel.
            bReject = false;
        }
    }

    if (bReject)
        QDialog::reject();
}


// Browse and open an instrument file.
void qsamplerChannelForm::openInstrumentFile (void)
{
    qsamplerOptions *pOptions = m_pChannel->options();
    if (pOptions == NULL)
        return;

    // FIXME: the instrument file filters should be restricted,
    // depending on the current engine.
    QString sInstrumentFile = QFileDialog::getOpenFileName(
            pOptions->sInstrumentDir,                   // Start here.
            tr("Instrument files") + " (*.gig *.dls)",  // Filter (GIG and DLS files)
            this, 0,                                    // Parent and name (none)
            tr("Instrument files")                      // Caption.
    );

    if (sInstrumentFile.isEmpty())
        return;

    InstrumentFileComboBox->setCurrentText(sInstrumentFile);
}


// Refresh the actual instrument name.
void qsamplerChannelForm::updateInstrumentName (void)
{
    // FIXME: A better idea would be to use libgig
    // to retrieve the REAL instrument name.
    InstrumentNameTextLabel->setText(QFileInfo(InstrumentFileComboBox->currentText()).fileName()
        + " [" + QString::number(InstrumentNrSpinBox->value()) + "]");

    optionsChanged();
}



// Dirty up settings.
void qsamplerChannelForm::optionsChanged (void)
{
    if (m_iDirtySetup > 0)
        return;

    m_iDirtyCount++;
    stabilizeForm();
}


// Stabilize current form state.
void qsamplerChannelForm::stabilizeForm (void)
{
    const QString sFilename = InstrumentFileComboBox->currentText();
    OkPushButton->setEnabled(m_iDirtyCount > 0 && !sFilename.isEmpty() && QFileInfo(sFilename).exists());
}


// end of qsamplerChannelForm.ui.h

