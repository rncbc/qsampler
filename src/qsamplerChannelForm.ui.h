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
void qsamplerChannelForm::setup ( qsamplerChannel *pChannel )
{
    m_pChannel = pChannel;

    m_iDirtySetup = 0;
    m_iDirtyCount = 0;

    if (m_pChannel == NULL)
        return;

    // It can be a brand new channel, remember?
    bool bNew = (m_pChannel->channelID() < 0);
    setCaption(m_pChannel->channelName());

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

    // Engine name...
    QString sEngineName = pChannel->engineName();
    if (sEngineName.isEmpty() && bNew)
        sEngineName = pOptions->sEngineName;
    if (sEngineName.isEmpty())
        sEngineName = tr("(No engine)");
    if (EngineNameComboBox->listBox()->findItem(sEngineName, Qt::ExactMatch) == NULL)
        EngineNameComboBox->insertItem(sEngineName);
    EngineNameComboBox->setCurrentText(sEngineName);
    // Instrument filename and index...
    QString sInstrumentFile = pChannel->instrumentFile();
    if (sInstrumentFile.isEmpty())
        sInstrumentFile = tr("(No instrument)");
    InstrumentFileComboBox->setCurrentText(sInstrumentFile);
    InstrumentNrSpinBox->setValue(pChannel->instrumentNr());
    // MIDI input driver...
    QString sMidiDriver = pChannel->midiDriver();
    if (sMidiDriver.isEmpty() || bNew)
        sMidiDriver = pOptions->sMidiDriver;
    if (!sMidiDriver.isEmpty()) {
        if (MidiDriverComboBox->listBox()->findItem(sMidiDriver, Qt::ExactMatch) == NULL)
            MidiDriverComboBox->insertItem(sMidiDriver);
        MidiDriverComboBox->setCurrentText(sMidiDriver);
    }
    // MIDI input port...
    MidiPortSpinBox->setValue(pChannel->midiPort());
    // MIDI input channel...
    int iMidiChannel = pChannel->midiChannel();
    // When new, try to suggest a sensible MIDI channel...
    if (iMidiChannel < 0)
        iMidiChannel = (::lscp_get_channels(m_pChannel->client()) % 16);
    MidiChannelComboBox->setCurrentItem(iMidiChannel);
    // Audio output driver...
    QString sAudioDriver = pChannel->audioDriver();
    if (sAudioDriver.isEmpty() || bNew)
        sAudioDriver = pOptions->sAudioDriver;
    if (!sAudioDriver.isEmpty()) {
        if (AudioDriverComboBox->listBox()->findItem(sAudioDriver, Qt::ExactMatch) == NULL)
            AudioDriverComboBox->insertItem(sAudioDriver);
        AudioDriverComboBox->setCurrentText(sAudioDriver);
    }
    // Done.
    m_iDirtySetup--;
    stabilizeForm();
}


// Accept settings (OK button slot).
void qsamplerChannelForm::accept (void)
{
    if (m_pChannel == NULL)
        return;

    qsamplerOptions *pOptions = m_pChannel->options();
    if (pOptions == NULL)
        return;

    // We'll go for it!
    if (m_iDirtyCount > 0) {
        int iErrors = 0;
        // Are we a new channel?
        if (!m_pChannel->addChannel())
            iErrors++;
        // Audio output driver type...
        if (!m_pChannel->setAudioDriver(AudioDriverComboBox->currentText()))
            iErrors++;
        // MIDI input driver type...
        if (!m_pChannel->setMidiDriver(MidiDriverComboBox->currentText()))
            iErrors++;
        // MIDI input port number...
        if (!m_pChannel->setMidiPort(MidiPortSpinBox->value()))
            iErrors++;
        // MIDI input channel...
        if (!m_pChannel->setMidiChannel(MidiChannelComboBox->currentItem()))
            iErrors++;
        // Engine name...
        if (!m_pChannel->loadEngine(EngineNameComboBox->currentText()))
            iErrors++;
        // Instrument file and index...
        if (!m_pChannel->loadInstrument(InstrumentFileComboBox->currentText(), InstrumentNrSpinBox->value()))
            iErrors++;
        // Show error messages?
        if (iErrors > 0)
            m_pChannel->appendMessagesError(tr("Some channel settings could not be set.\n\nSorry."));
    }

    // Save default engine name, instrument directory and history...
    pOptions->sInstrumentDir = QFileInfo(InstrumentFileComboBox->currentText()).dirPath(true);
    pOptions->sEngineName  = EngineNameComboBox->currentText();
    pOptions->sAudioDriver = AudioDriverComboBox->currentText();
    pOptions->sMidiDriver  = MidiDriverComboBox->currentText();
    pOptions->saveComboBoxHistory(InstrumentFileComboBox);

    // Just go with dialog acceptance.
    QDialog::accept();
}


// Reject settings (Cancel button slot).
void qsamplerChannelForm::reject (void)
{
    bool bReject = true;

    // Check if there's any pending changes...
    if (m_iDirtyCount > 0 && OkPushButton->isEnabled()) {
        switch (QMessageBox::warning(this, tr("Warning"),
            tr("Some channel settings have been changed.\n\n"
               "Do you want to apply the changes?"),
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
    const QString& sFilename = InstrumentFileComboBox->currentText();
    OkPushButton->setEnabled(m_iDirtyCount > 0 && !sFilename.isEmpty() && QFileInfo(sFilename).exists());
}


// end of qsamplerChannelForm.ui.h

