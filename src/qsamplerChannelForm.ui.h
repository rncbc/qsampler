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
    EngineNameComboBox->clear();
    const char **ppszEngines = ::lscp_get_available_engines(m_pChannel->client());
    if (ppszEngines) {
        for (int iEngine = 0; ppszEngines[iEngine]; iEngine++)
            EngineNameComboBox->insertItem(ppszEngines[iEngine]);
    }

    // Populate Audio output type list.
    AudioTypeComboBox->clear();
    const char **ppszAudioTypes = ::lscp_get_available_audio_types(m_pChannel->client());
    if (ppszAudioTypes) {
        for (int iAudioType = 0; ppszAudioTypes[iAudioType]; iAudioType++)
            AudioTypeComboBox->insertItem(ppszAudioTypes[iAudioType]);
    }

    // Populate MIDI input type list.
    MidiTypeComboBox->clear();
    const char **ppszMidiTypes = ::lscp_get_available_midi_types(m_pChannel->client());
    if (ppszMidiTypes) {
        for (int iMidiType = 0; ppszMidiTypes[iMidiType]; iMidiType++)
            MidiTypeComboBox->insertItem(ppszMidiTypes[iMidiType]);
    }
    
    // Read channel information,
    // and populate the channel form fields.
    lscp_channel_info_t *pChannelInfo = ::lscp_get_channel_info(m_pChannel->client(), m_pChannel->channelID());
    if (pChannelInfo) {
        EngineNameComboBox->setCurrentText(pChannelInfo->engine_name);
        InstrumentFileComboBox->setCurrentText(pChannelInfo->instrument_file);
        InstrumentNrSpinBox->setValue(pChannelInfo->instrument_nr);
        MidiTypeComboBox->setCurrentText(pChannelInfo->midi_type);
        MidiPortSpinBox->setValue(pChannelInfo->midi_port);
        MidiChannelSpinBox->setValue(pChannelInfo->midi_channel);
        AudioTypeComboBox->setCurrentText(pChannelInfo->audio_type);
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
        // Engine name...
        ::lscp_load_engine(m_pChannel->client(),
            EngineNameComboBox->currentText().latin1(),
            m_pChannel->channelID());
        // Instrument file and index...
        ::lscp_load_instrument(m_pChannel->client(),
            InstrumentFileComboBox->currentText().latin1(),
            InstrumentNrSpinBox->value(),
            m_pChannel->channelID());
        // MIDI input type...
        ::lscp_set_channel_midi_type(m_pChannel->client(),
            m_pChannel->channelID(),
            MidiTypeComboBox->currentText().latin1());
        // MIDI input port number...
        ::lscp_set_channel_midi_port(m_pChannel->client(),
            m_pChannel->channelID(),
            MidiPortSpinBox->value());
        // MIDI input channel (0=ALL)...
        ::lscp_set_channel_midi_channel(m_pChannel->client(),
            m_pChannel->channelID(),
            MidiChannelSpinBox->value());
        // Audio output type...
        ::lscp_set_channel_audio_type(m_pChannel->client(),
            m_pChannel->channelID(),
            AudioTypeComboBox->currentText().latin1());
    }
    
    // Save combobox history...
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
    if (m_pChannel->options() == NULL)
        return;

    // FIXME: the instrument file filters should be restricted,
    // depending on the current engine.
    QString sInstrumentFile = QFileDialog::getOpenFileName(
            InstrumentFileComboBox->currentText(),      // Start here.
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
    OkPushButton->setEnabled(m_iDirtyCount > 0);
}


// end of qsamplerChannelForm.ui.h

