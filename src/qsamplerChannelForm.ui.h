// qsamplerChannelForm.ui.h
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
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qlistbox.h>

#include "qsamplerDeviceForm.h"

#include "config.h"


// Kind of constructor.
void qsamplerChannelForm::init (void)
{
	// Initialize locals.
	m_pChannel = NULL;

	m_iDirtySetup = 0;
	m_iDirtyCount = 0;

	m_midiDevices.setAutoDelete(true);
	m_audioDevices.setAutoDelete(true);

	m_pDeviceForm = NULL;

	// Try to restore normal window positioning.
	adjustSize();
}


// Kind of destructor.
void qsamplerChannelForm::destroy (void)
{
	if (m_pDeviceForm)
		delete m_pDeviceForm;
	m_pDeviceForm = NULL;
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
	AudioDriverComboBox->clear();
	AudioDriverComboBox->insertStringList(
		qsamplerDevice::getDrivers(m_pChannel->client(), qsamplerDevice::Audio));

	// Populate MIDI input type list.
	MidiDriverComboBox->clear();
	MidiDriverComboBox->insertStringList(
		qsamplerDevice::getDrivers(m_pChannel->client(), qsamplerDevice::Midi));

	// Read proper channel information,
	// and populate the channel form fields.

	// Engine name...
	QString sEngineName = pChannel->engineName();
	if (sEngineName.isEmpty() || bNew)
		sEngineName = pOptions->sEngineName;
	if (sEngineName.isEmpty())
		sEngineName = qsamplerChannel::noEngineName();
	if (EngineNameComboBox->listBox()->findItem(sEngineName,
			Qt::ExactMatch | Qt::CaseSensitive) == NULL) {
		EngineNameComboBox->insertItem(sEngineName);
	}
	EngineNameComboBox->setCurrentText(sEngineName);
	// Instrument filename and index...
	QString sInstrumentFile = pChannel->instrumentFile();
	if (sInstrumentFile.isEmpty())
		sInstrumentFile = qsamplerChannel::noInstrumentName();
	InstrumentFileComboBox->setCurrentText(sInstrumentFile);
	InstrumentNrComboBox->clear();
	InstrumentNrComboBox->insertStringList(
		qsamplerChannel::getInstrumentList(sInstrumentFile,
		pOptions->bInstrumentNames));
	InstrumentNrComboBox->setCurrentItem(pChannel->instrumentNr());

	// MIDI input device...
	qsamplerDevice midiDevice(m_pChannel->mainForm(),
		qsamplerDevice::Midi, m_pChannel->midiDevice());
	// MIDI input driver...
	QString sMidiDriver = midiDevice.driverName();
	if (sMidiDriver.isEmpty() || bNew)
		sMidiDriver = pOptions->sMidiDriver.upper();
	if (!sMidiDriver.isEmpty()) {
		if (MidiDriverComboBox->listBox()->findItem(sMidiDriver,
				Qt::ExactMatch | Qt::CaseSensitive) == NULL) {
			MidiDriverComboBox->insertItem(sMidiDriver);
		}
		MidiDriverComboBox->setCurrentText(sMidiDriver);
	}
	selectMidiDriverItem(sMidiDriver);
	if (!bNew)
		MidiDeviceComboBox->setCurrentText(midiDevice.deviceName());
	selectMidiDeviceItem(MidiDeviceComboBox->currentItem());
	// MIDI input port...
	MidiPortSpinBox->setValue(pChannel->midiPort());
	// MIDI input channel...
	int iMidiChannel = pChannel->midiChannel();
	// When new, try to suggest a sensible MIDI channel...
	if (iMidiChannel < 0)
		iMidiChannel = (::lscp_get_channels(m_pChannel->client()) % 16);
	MidiChannelComboBox->setCurrentItem(iMidiChannel);

	// Audio output device...
	qsamplerDevice audioDevice(m_pChannel->mainForm(),
		qsamplerDevice::Audio, m_pChannel->audioDevice());
	// Audio output driver...
	QString sAudioDriver = audioDevice.driverName();
	if (sAudioDriver.isEmpty() || bNew)
		sAudioDriver = pOptions->sAudioDriver.upper();
	if (!sAudioDriver.isEmpty()) {
		if (AudioDriverComboBox->listBox()->findItem(sAudioDriver,
				Qt::ExactMatch | Qt::CaseSensitive) == NULL) {
			AudioDriverComboBox->insertItem(sAudioDriver);
		}
		AudioDriverComboBox->setCurrentText(sAudioDriver);
	}
	selectAudioDriverItem(sAudioDriver);
	if (!bNew)
		AudioDeviceComboBox->setCurrentText(audioDevice.deviceName());
	selectAudioDeviceItem(AudioDeviceComboBox->currentItem());

	// As convenient, make it ready on stabilizeForm() for
	// prompt acceptance, if we got the minimum required...
	if (sEngineName != qsamplerChannel::noEngineName() &&
		sInstrumentFile != qsamplerChannel::noInstrumentName())
		m_iDirtyCount++;
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
		// Accept Audio driver or device selection...
		if (m_audioDevices.isEmpty()) {
			if (!m_pChannel->setAudioDriver(AudioDriverComboBox->currentText()))
				iErrors++;
		} else {
			qsamplerDevice *pDevice = m_audioDevices.at(AudioDeviceComboBox->currentItem());
			if (pDevice == NULL)
				iErrors++;
			else if (!m_pChannel->setAudioDevice(pDevice->deviceID()))
				iErrors++;
		}
		// Accept MIDI driver or device selection...
		if (m_midiDevices.isEmpty()) {
			if (!m_pChannel->setMidiDriver(MidiDriverComboBox->currentText()))
				iErrors++;
		} else {
			qsamplerDevice *pDevice = m_midiDevices.at(MidiDeviceComboBox->currentItem());
			if (pDevice == NULL)
				iErrors++;
			else if (!m_pChannel->setMidiDevice(pDevice->deviceID()))
				iErrors++;
		}
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
		if (!m_pChannel->loadInstrument(InstrumentFileComboBox->currentText(), InstrumentNrComboBox->currentItem()))
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
	qsamplerOptions *pOptions = m_pChannel->options();
	if (pOptions == NULL)
		return;

	// Finally this better idea would be to use libgig
	// to retrieve the REAL instrument names.
	InstrumentNrComboBox->clear();
	InstrumentNrComboBox->insertStringList(
		qsamplerChannel::getInstrumentList(
			InstrumentFileComboBox->currentText(),
			pOptions->bInstrumentNames)
	);

	optionsChanged();
}

// Show device options dialog.
void qsamplerChannelForm::setupDevice ( qsamplerDevice *pDevice,
	qsamplerDevice::qsamplerDeviceType deviceTypeMode,
	const QString& sDriverName )
{
	// Create the device form if not already...
	if (m_pDeviceForm == NULL) {
		m_pDeviceForm = new qsamplerDeviceForm(this, 0,
			WType_Dialog | WShowModal);
		m_pDeviceForm->setMainForm(m_pChannel->mainForm());
		QObject::connect(m_pDeviceForm, SIGNAL(devicesChanged()),
			this, SLOT(updateDevices()));
	}

	// Refresh the device form with selected data.
	if (m_pDeviceForm) {
		m_pDeviceForm->setDeviceTypeMode(deviceTypeMode);
		m_pDeviceForm->setClient(m_pChannel->client()); // -> refreshDevices();
		m_pDeviceForm->setDevice(pDevice);
		m_pDeviceForm->setDriverName(sDriverName);
		m_pDeviceForm->show();
	}
}


// Refresh MIDI driver item devices.
void qsamplerChannelForm::selectMidiDriverItem ( const QString& sMidiDriver )
{
	const QString sDriverName = sMidiDriver.upper();

	// Save current device id.
	int iDeviceID = -1;
	qsamplerDevice *pDevice = m_midiDevices.at(MidiDeviceComboBox->currentItem());
	if (pDevice)
		iDeviceID = pDevice->deviceID();

	// Clean maplist.
	MidiDeviceComboBox->clear();
	m_midiDevices.clear();

	// Populate with the current ones...
	const QPixmap& midiPixmap = QPixmap::fromMimeSource("midi2.png");
	int *piDeviceIDs = qsamplerDevice::getDevices(m_pChannel->client(),
		qsamplerDevice::Midi);
	for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
		pDevice = new qsamplerDevice(m_pChannel->mainForm(),
			qsamplerDevice::Midi, piDeviceIDs[i]);
		if (pDevice->driverName().upper() == sDriverName) {
			MidiDeviceComboBox->insertItem(midiPixmap, pDevice->deviceName());
			m_midiDevices.append(pDevice);
		} else {
			delete pDevice;
		}
	}

	// Do proper enabling...
	bool bEnabled = !m_midiDevices.isEmpty();
	if (!bEnabled) {
		MidiDeviceComboBox->insertItem(
			tr("(New MIDI %1 device)").arg(sMidiDriver));
	} else if (iDeviceID >= 0) {
		// Select the previous current device...
		int iMidiItem = 0;
		for (pDevice = m_midiDevices.first();
				pDevice;
					pDevice = m_midiDevices.next()) {
			if (pDevice->deviceID() == iDeviceID) {
				MidiDeviceComboBox->setCurrentItem(iMidiItem);
			//	selectMidiDeviceItem(iMidiItem);
				break;
			}
			iMidiItem++;
		}
	}
	MidiDeviceTextLabel->setEnabled(bEnabled);
	MidiDeviceComboBox->setEnabled(bEnabled);
}


// Refresh MIDI device options slot.
void qsamplerChannelForm::selectMidiDriver ( const QString& sMidiDriver )
{
	if (m_iDirtySetup > 0)
		return;

	selectMidiDriverItem(sMidiDriver);
	optionsChanged();
}


// Select MIDI device item.
void qsamplerChannelForm::selectMidiDeviceItem ( int iMidiItem )
{
	qsamplerDevice *pDevice = m_midiDevices.at(iMidiItem);
	if (pDevice) {
		const qsamplerDeviceParamMap& params = pDevice->params();
		int iPorts = params["PORTS"].value.toInt();
		MidiPortTextLabel->setEnabled(iPorts > 0);
		MidiPortSpinBox->setEnabled(iPorts > 0);
		if (iPorts > 0)
			MidiPortSpinBox->setMaxValue(iPorts - 1);
	}
}


// Select MIDI device options slot.
void qsamplerChannelForm::selectMidiDevice ( int iMidiItem )
{
	if (m_iDirtySetup > 0)
		return;

	selectMidiDeviceItem(iMidiItem);
	optionsChanged();
}


// MIDI device options.
void qsamplerChannelForm::setupMidiDevice (void)
{
	setupDevice(m_midiDevices.at(MidiDeviceComboBox->currentItem()),
		qsamplerDevice::Midi, MidiDriverComboBox->currentText());
}


// Refresh Audio driver item devices.
void qsamplerChannelForm::selectAudioDriverItem ( const QString& sAudioDriver )
{
	const QString sDriverName = sAudioDriver.upper();

	// Save current device id.
	int iDeviceID = -1;
	qsamplerDevice *pDevice = m_audioDevices.at(AudioDeviceComboBox->currentItem());
	if (pDevice)
		iDeviceID = pDevice->deviceID();

	// Clean maplist.
	AudioDeviceComboBox->clear();
	m_audioDevices.clear();

	// Populate with the current ones...
	const QPixmap& audioPixmap = QPixmap::fromMimeSource("audio2.png");
	int *piDeviceIDs = qsamplerDevice::getDevices(m_pChannel->client(),
		qsamplerDevice::Audio);
	for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
		pDevice = new qsamplerDevice(m_pChannel->mainForm(),
			qsamplerDevice::Audio, piDeviceIDs[i]);
		if (pDevice->driverName().upper() == sDriverName) {
			AudioDeviceComboBox->insertItem(audioPixmap, pDevice->deviceName());
			m_audioDevices.append(pDevice);
		} else {
			delete pDevice;
		}
	}

	// Do proper enabling...
	bool bEnabled = !m_audioDevices.isEmpty();
	if (!bEnabled) {
		AudioDeviceComboBox->insertItem(
			tr("(New Audio %1 device)").arg(sAudioDriver));
	} else if (iDeviceID >= 0) {
		// Select the previous current device...
		int iAudioItem = 0;
		for (pDevice = m_audioDevices.first();
				pDevice;
					pDevice = m_audioDevices.next()) {
			if (pDevice->deviceID() == iDeviceID) {
				AudioDeviceComboBox->setCurrentItem(iAudioItem);
			//	selectAudioDeviceItem(iAudioItem);
				break;
			}
			iAudioItem++;
		}
	}
	AudioDeviceTextLabel->setEnabled(bEnabled);
	AudioDeviceComboBox->setEnabled(bEnabled);
}


// Refresh Audio device options slot.
void qsamplerChannelForm::selectAudioDriver ( const QString& sAudioDriver )
{
	if (m_iDirtySetup > 0)
		return;

	selectAudioDriverItem(sAudioDriver);
	optionsChanged();
}


// Select Audio device item.
void qsamplerChannelForm::selectAudioDeviceItem ( int iAudioItem )
{
	qsamplerDevice *pDevice = m_audioDevices.at(iAudioItem);
	if (pDevice) {
		// Is there anything to do here?
	}
}


// Select Audio device options slot.
void qsamplerChannelForm::selectAudioDevice ( int iAudioItem )
{
	if (m_iDirtySetup > 0)
		return;

	selectAudioDeviceItem(iAudioItem);
	optionsChanged();
}


// Audio device options.
void qsamplerChannelForm::setupAudioDevice (void)
{
	setupDevice(m_audioDevices.at(AudioDeviceComboBox->currentItem()),
		qsamplerDevice::Audio, AudioDriverComboBox->currentText());
}


// UPdate all device lists slot.
void qsamplerChannelForm::updateDevices (void)
{
	if (m_iDirtySetup > 0)
		return;

	selectMidiDriverItem(MidiDriverComboBox->currentText());
	selectAudioDriverItem(AudioDriverComboBox->currentText());
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
