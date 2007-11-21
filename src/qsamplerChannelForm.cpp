// qsamplerChannelForm.cpp
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

#include "qsamplerChannelForm.h"

#include "qsamplerAbout.h"
#include "qsamplerDeviceForm.h"

#include "qsamplerMainForm.h"
#include "qsamplerInstrument.h"

#include <QValidator>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>

#include <QHeaderView>


namespace QSampler {

ChannelForm::ChannelForm(QWidget* parent) : QDialog(parent) {
    ui.setupUi(this);

	// Initialize locals.
	m_pChannel = NULL;

	m_iDirtySetup = 0;
	m_iDirtyCount = 0;

//	m_midiDevices.setAutoDelete(true);
//	m_audioDevices.setAutoDelete(true);

	m_pDeviceForm = NULL;

	// Try to restore normal window positioning.
	adjustSize();

	ui.AudioRoutingTable->setModel(&routingModel);
	ui.AudioRoutingTable->setItemDelegate(&routingDelegate);
	ui.AudioRoutingTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);

	QObject::connect(ui.EngineNameComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(ui.InstrumentFileComboBox,
		SIGNAL(activated(const QString&)),
		SLOT(updateInstrumentName()));
	QObject::connect(ui.InstrumentFileToolButton,
		SIGNAL(clicked()),
		SLOT(openInstrumentFile()));
	QObject::connect(ui.InstrumentNrComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(ui.MidiDriverComboBox,
		SIGNAL(activated(const QString&)),
		SLOT(selectMidiDriver(const QString&)));
	QObject::connect(ui.MidiDeviceComboBox,
		SIGNAL(activated(int)),
		SLOT(selectMidiDevice(int)));
	QObject::connect(ui.MidiPortSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(ui.MidiChannelComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(ui.MidiMapComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(ui.AudioDriverComboBox,
		SIGNAL(activated(const QString&)),
		SLOT(selectAudioDriver(const QString&)));
	QObject::connect(ui.AudioDeviceComboBox,
		SIGNAL(activated(int)),
		SLOT(selectAudioDevice(int)));
	QObject::connect(ui.OkPushButton,
		SIGNAL(clicked()),
		SLOT(accept()));
	QObject::connect(ui.CancelPushButton,
		SIGNAL(clicked()),
		SLOT(reject()));
	QObject::connect(ui.MidiDeviceToolButton,
		SIGNAL(clicked()),
		SLOT(setupMidiDevice()));
	QObject::connect(ui.AudioDeviceToolButton,
		SIGNAL(clicked()),
		SLOT(setupAudioDevice()));
	QObject::connect(&routingModel,
		SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
		SLOT(optionsChanged()));
	QObject::connect(&routingModel,
		SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
		SLOT(updateTableCellRenderers(const QModelIndex&, const QModelIndex&)));
	QObject::connect(&routingModel,
		SIGNAL(modelReset()),
		SLOT(updateTableCellRenderers()));
}

ChannelForm::~ChannelForm()
{
	if (m_pDeviceForm)
		delete m_pDeviceForm;
	m_pDeviceForm = NULL;

	qDeleteAll(m_midiDevices);
	m_midiDevices.clear();

	qDeleteAll(m_audioDevices);
	m_audioDevices.clear();
}


// Channel dialog setup formal initializer.
void ChannelForm::setup ( qsamplerChannel *pChannel )
{
	m_pChannel = pChannel;

	m_iDirtySetup = 0;
	m_iDirtyCount = 0;

	if (m_pChannel == NULL)
		return;

	// It can be a brand new channel, remember?
	bool bNew = (m_pChannel->channelID() < 0);
	setWindowTitle(QSAMPLER_TITLE ": " + m_pChannel->channelName());

	// Check if we're up and connected.
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// Avoid nested changes.
	m_iDirtySetup++;

	// Load combo box history...
	pOptions->loadComboBoxHistory(ui.InstrumentFileComboBox);

	// Populate Engines list.
	const char **ppszEngines = ::lscp_list_available_engines(pMainForm->client());
	if (ppszEngines) {
		ui.EngineNameComboBox->clear();
		for (int iEngine = 0; ppszEngines[iEngine]; iEngine++)
			ui.EngineNameComboBox->addItem(QString(ppszEngines[iEngine]));
	}
	else m_pChannel->appendMessagesClient("lscp_list_available_engines");

	// Populate Audio output type list.
	ui.AudioDriverComboBox->clear();
	ui.AudioDriverComboBox->insertItems(0,
		qsamplerDevice::getDrivers(pMainForm->client(), qsamplerDevice::Audio));

	// Populate MIDI input type list.
	ui.MidiDriverComboBox->clear();
	ui.MidiDriverComboBox->insertItems(0,
		qsamplerDevice::getDrivers(pMainForm->client(), qsamplerDevice::Midi));

	// Populate Maps list.
	ui.MidiMapComboBox->clear();
	ui.MidiMapComboBox->insertItems(0,
		qsamplerInstrument::getMapNames());

	// Read proper channel information,
	// and populate the channel form fields.

	// Engine name...
	QString sEngineName = pChannel->engineName();
	if (sEngineName.isEmpty() || bNew)
		sEngineName = pOptions->sEngineName;
	if (sEngineName.isEmpty())
		sEngineName = qsamplerChannel::noEngineName();
	if (ui.EngineNameComboBox->findText(sEngineName,
			Qt::MatchExactly | Qt::MatchCaseSensitive) < 0) {
		ui.EngineNameComboBox->addItem(sEngineName);
	}
	ui.EngineNameComboBox->setCurrentIndex(
		ui.EngineNameComboBox->findText(sEngineName,
			Qt::MatchExactly | Qt::MatchCaseSensitive));

	// Instrument filename and index...
	QString sInstrumentFile = pChannel->instrumentFile();
	if (sInstrumentFile.isEmpty())
		sInstrumentFile = qsamplerChannel::noInstrumentName();
	ui.InstrumentFileComboBox->setEditText(sInstrumentFile);
	ui.InstrumentNrComboBox->clear();
	ui.InstrumentNrComboBox->insertItems(0,
		qsamplerChannel::getInstrumentList(sInstrumentFile,
		pOptions->bInstrumentNames));
	ui.InstrumentNrComboBox->setCurrentIndex(pChannel->instrumentNr());

	// MIDI input device...
	qsamplerDevice midiDevice(qsamplerDevice::Midi, m_pChannel->midiDevice());
	// MIDI input driver...
	QString sMidiDriver = midiDevice.driverName();
	if (sMidiDriver.isEmpty() || bNew)
		sMidiDriver = pOptions->sMidiDriver.toUpper();
	if (!sMidiDriver.isEmpty()) {
		if (ui.MidiDriverComboBox->findText(sMidiDriver,
				Qt::MatchExactly | Qt::MatchCaseSensitive) < 0) {
			ui.MidiDriverComboBox->insertItem(0, sMidiDriver);
		}
		ui.MidiDriverComboBox->setItemText(
			ui.MidiDriverComboBox->currentIndex(),
			sMidiDriver);
	}
	selectMidiDriverItem(sMidiDriver);
	if (!bNew) {
		ui.MidiDeviceComboBox->setItemText(
			ui.MidiDeviceComboBox->currentIndex(),
			midiDevice.deviceName());
	}
	selectMidiDeviceItem(ui.MidiDeviceComboBox->currentIndex());
	// MIDI input port...
	ui.MidiPortSpinBox->setValue(pChannel->midiPort());
	// MIDI input channel...
	int iMidiChannel = pChannel->midiChannel();
	// When new, try to suggest a sensible MIDI channel...
	if (iMidiChannel < 0)
		iMidiChannel = (::lscp_get_channels(pMainForm->client()) % 16);
	ui.MidiChannelComboBox->setCurrentIndex(iMidiChannel);
	// MIDI instrument map...
	int iMidiMap = (bNew ? pOptions->iMidiMap : pChannel->midiMap());
	// When new, try to suggest a sensible MIDI map...
	if (iMidiMap < 0)
		iMidiMap = 0;
	const QString& sMapName = qsamplerInstrument::getMapName(iMidiMap);
	if (!sMapName.isEmpty()) {
		ui.MidiMapComboBox->setItemText(
			ui.MidiMapComboBox->currentIndex(),
			sMapName);
	}
	// It might be no maps around...
	bool bMidiMapEnabled = (ui.MidiMapComboBox->count() > 0);
	ui.MidiMapTextLabel->setEnabled(bMidiMapEnabled);
	ui.MidiMapComboBox->setEnabled(bMidiMapEnabled);

	// Audio output device...
	qsamplerDevice audioDevice(qsamplerDevice::Audio, m_pChannel->audioDevice());
	// Audio output driver...
	QString sAudioDriver = audioDevice.driverName();
	if (sAudioDriver.isEmpty() || bNew)
		sAudioDriver = pOptions->sAudioDriver.toUpper();
	if (!sAudioDriver.isEmpty()) {
		if (ui.AudioDriverComboBox->findText(sAudioDriver,
				Qt::MatchExactly | Qt::MatchCaseSensitive) < 0) {
			ui.AudioDriverComboBox->insertItem(0, sAudioDriver);
		}
		ui.AudioDriverComboBox->setItemText(
			ui.AudioDriverComboBox->currentIndex(),
			sAudioDriver);
	}
	selectAudioDriverItem(sAudioDriver);
	if (!bNew) {
		ui.AudioDeviceComboBox->setItemText(
			ui.AudioDeviceComboBox->currentIndex(),
			audioDevice.deviceName());
	}
	selectAudioDeviceItem(ui.AudioDeviceComboBox->currentIndex());

	// As convenient, make it ready on stabilizeForm() for
	// prompt acceptance, if we got the minimum required...
/*	if (sEngineName != qsamplerChannel::noEngineName() &&
		sInstrumentFile != qsamplerChannel::noInstrumentName())
		m_iDirtyCount++; */
	// Done.
	m_iDirtySetup--;
	stabilizeForm();
}


// Accept settings (OK button slot).
void ChannelForm::accept (void)
{
	if (m_pChannel == NULL)
		return;

	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// Flush any pending editing...
	//ui.AudioRoutingTable->flush();

	// We'll go for it!
	if (m_iDirtyCount > 0) {
		int iErrors = 0;
		// Are we a new channel?
		if (!m_pChannel->addChannel())
			iErrors++;
		// Accept Audio driver or device selection...
		if (m_audioDevices.isEmpty()) {
			if (!m_pChannel->setAudioDriver(ui.AudioDriverComboBox->currentText()))
				iErrors++;
		} else {
			qsamplerDevice *pDevice = NULL;
			int iAudioItem = ui.AudioDeviceComboBox->currentIndex();
			if (iAudioItem >= 0 && iAudioItem < m_audioDevices.count())
				pDevice = m_audioDevices.at(iAudioItem);
			qsamplerChannelRoutingMap routingMap = routingModel.routingMap();
			if (pDevice == NULL)
				iErrors++;
			else if (!m_pChannel->setAudioDevice(pDevice->deviceID()))
				iErrors++;
			else if (!routingMap.isEmpty()) {
				// Set the audio route changes...
				qsamplerChannelRoutingMap::ConstIterator iter;
				for (iter = routingMap.begin();
						iter != routingMap.end(); ++iter) {
					if (!m_pChannel->setAudioChannel(iter.key(), iter.value()))
						iErrors++;
				}
			}
		}
		// Accept MIDI driver or device selection...
		if (m_midiDevices.isEmpty()) {
			if (!m_pChannel->setMidiDriver(ui.MidiDriverComboBox->currentText()))
				iErrors++;
		} else {
			qsamplerDevice *pDevice = NULL;
			int iMidiItem = ui.MidiDeviceComboBox->currentIndex();
			if (iMidiItem >= 0 && iMidiItem < m_midiDevices.count())
				pDevice = m_midiDevices.at(iMidiItem);
			if (pDevice == NULL)
				iErrors++;
			else if (!m_pChannel->setMidiDevice(pDevice->deviceID()))
				iErrors++;
		}
		// MIDI input port number...
		if (!m_pChannel->setMidiPort(ui.MidiPortSpinBox->value()))
			iErrors++;
		// MIDI input channel...
		if (!m_pChannel->setMidiChannel(ui.MidiChannelComboBox->currentIndex()))
			iErrors++;
		// Engine name...
		if (!m_pChannel->loadEngine(ui.EngineNameComboBox->currentText()))
			iErrors++;
		// Instrument file and index...
		const QString& sPath = ui.InstrumentFileComboBox->currentText();
		if (!sPath.isEmpty() && QFileInfo(sPath).exists()) {
			if (!m_pChannel->loadInstrument(sPath, ui.InstrumentNrComboBox->currentIndex()))
				iErrors++;
		}
		// MIDI intrument map...
		if (!m_pChannel->setMidiMap(ui.MidiMapComboBox->currentIndex()))
			iErrors++;
		// Show error messages?
		if (iErrors > 0)
			m_pChannel->appendMessagesError(tr("Some channel settings could not be set.\n\nSorry."));
	}

	// Save default engine name, instrument directory and history...
	pOptions->sInstrumentDir = QFileInfo(
		ui.InstrumentFileComboBox->currentText()).dir().absolutePath();
	pOptions->sEngineName  = ui.EngineNameComboBox->currentText();
	pOptions->sAudioDriver = ui.AudioDriverComboBox->currentText();
	pOptions->sMidiDriver  = ui.MidiDriverComboBox->currentText();
	pOptions->iMidiMap     = ui.MidiMapComboBox->currentIndex();
	pOptions->saveComboBoxHistory(ui.InstrumentFileComboBox);

	// Just go with dialog acceptance.
	QDialog::accept();
}


// Reject settings (Cancel button slot).
void ChannelForm::reject (void)
{
	bool bReject = true;

	// Check if there's any pending changes...
	if (m_iDirtyCount > 0 && ui.OkPushButton->isEnabled()) {
		switch (QMessageBox::warning(this,
			QSAMPLER_TITLE ": " + tr("Warning"),
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
			break;
		}
	}

	if (bReject)
		QDialog::reject();
}


// Browse and open an instrument file.
void ChannelForm::openInstrumentFile (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// FIXME: the instrument file filters should be restricted,
	// depending on the current engine.
	QString sInstrumentFile = QFileDialog::getOpenFileName(this,
		QSAMPLER_TITLE ": " + tr("Instrument files"), // Caption.
		pOptions->sInstrumentDir,                 // Start here.
		tr("Instrument files") + " (*.gig *.dls)" // Filter (GIG and DLS files)
	);

	if (sInstrumentFile.isEmpty())
		return;

	ui.InstrumentFileComboBox->setEditText(sInstrumentFile);
	updateInstrumentName();
}


// Refresh the actual instrument name.
void ChannelForm::updateInstrumentName (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// Finally this better idea would be to use libgig
	// to retrieve the REAL instrument names.
	ui.InstrumentNrComboBox->clear();
	ui.InstrumentNrComboBox->insertItems(0,
		qsamplerChannel::getInstrumentList(
			ui.InstrumentFileComboBox->currentText(),
			pOptions->bInstrumentNames)
	);

	optionsChanged();
}


// Show device options dialog.
void ChannelForm::setupDevice ( qsamplerDevice *pDevice,
	qsamplerDevice::qsamplerDeviceType deviceTypeMode,
	const QString& sDriverName )
{
	if (pDevice == NULL)
		return;

	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	// Create the device form if not already...
	if (m_pDeviceForm == NULL) {
		m_pDeviceForm = new DeviceForm(this, Qt::Dialog);
        m_pDeviceForm->setAttribute(Qt::WA_ShowModal);
		QObject::connect(m_pDeviceForm, SIGNAL(devicesChanged()),
			this, SLOT(updateDevices()));
	}

	// Refresh the device form with selected data.
	if (m_pDeviceForm) {
		m_pDeviceForm->setDeviceTypeMode(deviceTypeMode);
		m_pDeviceForm->refreshDevices();
		m_pDeviceForm->setDevice(pDevice);
		m_pDeviceForm->setDriverName(sDriverName);
		m_pDeviceForm->show();
	}
}


// Refresh MIDI driver item devices.
void ChannelForm::selectMidiDriverItem ( const QString& sMidiDriver )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	const QString sDriverName = sMidiDriver.toUpper();

	// Save current device id.
	// Save current device id.
	int iDeviceID = 0;
	qsamplerDevice *pDevice = NULL;
	int iMidiItem = ui.MidiDeviceComboBox->currentIndex();
	if (iMidiItem >= 0 && iMidiItem < m_midiDevices.count())
		pDevice = m_midiDevices.at(iMidiItem);
	if (pDevice)
		iDeviceID = pDevice->deviceID();

	// Clean maplist.
	ui.MidiDeviceComboBox->clear();
	qDeleteAll(m_midiDevices);
	m_midiDevices.clear();

	// Populate with the current ones...
	const QPixmap midiPixmap(":/icons/midi2.png");
	int *piDeviceIDs = qsamplerDevice::getDevices(pMainForm->client(),
		qsamplerDevice::Midi);
	for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
		pDevice = new qsamplerDevice(qsamplerDevice::Midi, piDeviceIDs[i]);
		if (pDevice->driverName().toUpper() == sDriverName) {
			ui.MidiDeviceComboBox->insertItem(0, midiPixmap, pDevice->deviceName());
			m_midiDevices.append(pDevice);
		} else {
			delete pDevice;
		}
	}

	// Do proper enabling...
	bool bEnabled = !m_midiDevices.isEmpty();
	if (bEnabled) {
		// Select the previous current device...
		iMidiItem = 0;
		QListIterator<qsamplerDevice *> iter(m_midiDevices);
		while (iter.hasNext()) {
			pDevice = iter.next();
			if (pDevice->deviceID() == iDeviceID) {
				ui.MidiDeviceComboBox->setCurrentIndex(iMidiItem);
				selectMidiDeviceItem(iMidiItem);
				break;
			}
			iMidiItem++;
		}
	} else {
		ui.MidiDeviceComboBox->insertItem(0,
			tr("(New MIDI %1 device)").arg(sMidiDriver));
	}
	ui.MidiDeviceTextLabel->setEnabled(bEnabled);
	ui.MidiDeviceComboBox->setEnabled(bEnabled);
}


// Refresh MIDI device options slot.
void ChannelForm::selectMidiDriver ( const QString& sMidiDriver )
{
	if (m_iDirtySetup > 0)
		return;

	selectMidiDriverItem(sMidiDriver);
	optionsChanged();
}


// Select MIDI device item.
void ChannelForm::selectMidiDeviceItem ( int iMidiItem )
{
	qsamplerDevice *pDevice = NULL;
	if (iMidiItem >= 0 && iMidiItem < m_midiDevices.count())
		pDevice = m_midiDevices.at(iMidiItem);
	if (pDevice) {
		const qsamplerDeviceParamMap& params = pDevice->params();
		int iPorts = params["PORTS"].value.toInt();
		ui.MidiPortTextLabel->setEnabled(iPorts > 0);
		ui.MidiPortSpinBox->setEnabled(iPorts > 0);
		if (iPorts > 0)
			ui.MidiPortSpinBox->setMaximum(iPorts - 1);
	}
}


// Select MIDI device options slot.
void ChannelForm::selectMidiDevice ( int iMidiItem )
{
	if (m_iDirtySetup > 0)
		return;

	selectMidiDeviceItem(iMidiItem);
	optionsChanged();
}


// MIDI device options.
void ChannelForm::setupMidiDevice (void)
{
	qsamplerDevice *pDevice = NULL;
	int iMidiItem = ui.MidiDeviceComboBox->currentIndex();
	if (iMidiItem >= 0 && iMidiItem < m_midiDevices.count())
		pDevice = m_midiDevices.at(iMidiItem);
	setupDevice(pDevice,
		qsamplerDevice::Midi, ui.MidiDriverComboBox->currentText());
}


// Refresh Audio driver item devices.
void ChannelForm::selectAudioDriverItem ( const QString& sAudioDriver )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	const QString sDriverName = sAudioDriver.toUpper();

	// Save current device id.
	int iDeviceID = 0;
	qsamplerDevice *pDevice = NULL;
	int iAudioItem = ui.AudioDeviceComboBox->currentIndex();
	if (iAudioItem >= 0 && iAudioItem < m_audioDevices.count())
		pDevice = m_audioDevices.at(iAudioItem);
	if (pDevice)
		iDeviceID = pDevice->deviceID();

	// Clean maplist.
	ui.AudioDeviceComboBox->clear();
	qDeleteAll(m_audioDevices);
	m_audioDevices.clear();

	// Populate with the current ones...
	const QPixmap audioPixmap(":/icons/audio2.png");
	int *piDeviceIDs = qsamplerDevice::getDevices(pMainForm->client(),
		qsamplerDevice::Audio);
	for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
		pDevice = new qsamplerDevice(qsamplerDevice::Audio, piDeviceIDs[i]);
		if (pDevice->driverName().toUpper() == sDriverName) {
			ui.AudioDeviceComboBox->insertItem(0, audioPixmap, pDevice->deviceName());
			m_audioDevices.append(pDevice);
		} else {
			delete pDevice;
		}
	}

	// Do proper enabling...
	bool bEnabled = !m_audioDevices.isEmpty();
	if (bEnabled) {
		// Select the previous current device...
		iAudioItem = 0;
		QListIterator<qsamplerDevice *> iter(m_audioDevices);
		while (iter.hasNext()) {
			pDevice = iter.next();
			if (pDevice->deviceID() == iDeviceID) {
				ui.AudioDeviceComboBox->setCurrentIndex(iAudioItem);
				selectAudioDeviceItem(iAudioItem);
				break;
			}
			iAudioItem++;
		}
	} else {
		ui.AudioDeviceComboBox->insertItem(0,
			tr("(New Audio %1 device)").arg(sAudioDriver));
		//ui.AudioRoutingTable->setNumRows(0);
	}
	ui.AudioDeviceTextLabel->setEnabled(bEnabled);
	ui.AudioDeviceComboBox->setEnabled(bEnabled);
	ui.AudioRoutingTable->setEnabled(bEnabled);
}


// Refresh Audio device options slot.
void ChannelForm::selectAudioDriver ( const QString& sAudioDriver )
{
	if (m_iDirtySetup > 0)
		return;

	selectAudioDriverItem(sAudioDriver);
	optionsChanged();
}


// Select Audio device item.
void ChannelForm::selectAudioDeviceItem ( int iAudioItem )
{
	qsamplerDevice *pDevice = NULL;
	if (iAudioItem >= 0 && iAudioItem < m_audioDevices.count())
		pDevice = m_audioDevices.at(iAudioItem);
	if (pDevice) {
		// Refresh the audio routing table.
		routingModel.refresh(pDevice, m_pChannel->audioRouting());
		// Reset routing change map.
		routingModel.clear();
	}
}


// Select Audio device options slot.
void ChannelForm::selectAudioDevice ( int iAudioItem )
{
	if (m_iDirtySetup > 0)
		return;

	selectAudioDeviceItem(iAudioItem);
	optionsChanged();
}


// Audio device options.
void ChannelForm::setupAudioDevice (void)
{
	qsamplerDevice *pDevice = NULL;
	int iAudioItem = ui.AudioDeviceComboBox->currentIndex();
	if (iAudioItem >= 0 && iAudioItem < m_audioDevices.count())
		pDevice = m_audioDevices.at(iAudioItem);
	setupDevice(pDevice,
		qsamplerDevice::Audio, ui.AudioDriverComboBox->currentText());
}

// UPdate all device lists slot.
void ChannelForm::updateDevices (void)
{
	if (m_iDirtySetup > 0)
		return;

	selectMidiDriverItem(ui.MidiDriverComboBox->currentText());
	selectAudioDriverItem(ui.AudioDriverComboBox->currentText());
	optionsChanged();
}


// Dirty up settings.
void ChannelForm::optionsChanged (void)
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtyCount++;
	stabilizeForm();
}


// Stabilize current form state.
void ChannelForm::stabilizeForm (void)
{
	bool bValid = true;
#if 0
	const QString& sPath = InstrumentFileComboBox->currentText();
	bValid = bValid && !sPath.isEmpty() && QFileInfo(sPath).exists();
#endif
	ui.OkPushButton->setEnabled(m_iDirtyCount > 0 && bValid);
}

void ChannelForm::updateTableCellRenderers() {
    const int rows = routingModel.rowCount();
    const int cols = routingModel.columnCount();
    updateTableCellRenderers(routingModel.index(0,0),routingModel.index(rows-1,cols-1));
}

void ChannelForm::updateTableCellRenderers(const QModelIndex& topLeft, const QModelIndex& bottomRight) {
    for (int r = topLeft.row(); r <= bottomRight.row(); r++) {
        for (int c = topLeft.column(); c <= bottomRight.column(); c++) {
            const QModelIndex index = routingModel.index(r,c);
            ui.AudioRoutingTable->openPersistentEditor(index);
        }
    }
}

} // namespace QSampler


// end of qsamplerChannelForm.cpp
