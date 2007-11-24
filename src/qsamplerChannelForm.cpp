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

ChannelForm::ChannelForm ( QWidget* pParent )
	: QDialog(pParent)
{
	m_ui.setupUi(this);

	// Initialize locals.
	m_pChannel = NULL;

	m_iDirtySetup = 0;
	m_iDirtyCount = 0;

//	m_midiDevices.setAutoDelete(true);
//	m_audioDevices.setAutoDelete(true);

	m_pDeviceForm = NULL;

	int iRowHeight = m_ui.AudioRoutingTable->fontMetrics().height() + 4;
	m_ui.AudioRoutingTable->verticalHeader()->setDefaultSectionSize(iRowHeight);
	m_ui.AudioRoutingTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

	m_ui.AudioRoutingTable->setModel(&m_routingModel);
	m_ui.AudioRoutingTable->setItemDelegate(&m_routingDelegate);
	m_ui.AudioRoutingTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
//	m_ui.AudioRoutingTable->verticalHeader()->hide();

	// This goes initially hidden, and will be shown
	// on setup() for currently existing channels...
	m_ui.AudioRoutingTable->hide();

	// Try to restore normal window positioning.
	adjustSize();

	QObject::connect(m_ui.EngineNameComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.InstrumentFileComboBox,
		SIGNAL(activated(const QString&)),
		SLOT(updateInstrumentName()));
	QObject::connect(m_ui.InstrumentFileToolButton,
		SIGNAL(clicked()),
		SLOT(openInstrumentFile()));
	QObject::connect(m_ui.InstrumentNrComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.MidiDriverComboBox,
		SIGNAL(activated(const QString&)),
		SLOT(selectMidiDriver(const QString&)));
	QObject::connect(m_ui.MidiDeviceComboBox,
		SIGNAL(activated(int)),
		SLOT(selectMidiDevice(int)));
	QObject::connect(m_ui.MidiPortSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.MidiChannelComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.MidiMapComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.AudioDriverComboBox,
		SIGNAL(activated(const QString&)),
		SLOT(selectAudioDriver(const QString&)));
	QObject::connect(m_ui.AudioDeviceComboBox,
		SIGNAL(activated(int)),
		SLOT(selectAudioDevice(int)));
	QObject::connect(m_ui.OkPushButton,
		SIGNAL(clicked()),
		SLOT(accept()));
	QObject::connect(m_ui.CancelPushButton,
		SIGNAL(clicked()),
		SLOT(reject()));
	QObject::connect(m_ui.MidiDeviceToolButton,
		SIGNAL(clicked()),
		SLOT(setupMidiDevice()));
	QObject::connect(m_ui.AudioDeviceToolButton,
		SIGNAL(clicked()),
		SLOT(setupAudioDevice()));
	QObject::connect(&m_routingModel,
		SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
		SLOT(optionsChanged()));
	QObject::connect(&m_routingModel,
		SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
		SLOT(updateTableCellRenderers(const QModelIndex&, const QModelIndex&)));
	QObject::connect(&m_routingModel,
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
	pOptions->loadComboBoxHistory(m_ui.InstrumentFileComboBox);

	// Populate Engines list.
	const char **ppszEngines = ::lscp_list_available_engines(pMainForm->client());
	if (ppszEngines) {
		m_ui.EngineNameComboBox->clear();
		for (int iEngine = 0; ppszEngines[iEngine]; iEngine++)
			m_ui.EngineNameComboBox->addItem(QString(ppszEngines[iEngine]));
	}
	else m_pChannel->appendMessagesClient("lscp_list_available_engines");

	// Populate Audio output type list.
	m_ui.AudioDriverComboBox->clear();
	m_ui.AudioDriverComboBox->insertItems(0,
		qsamplerDevice::getDrivers(pMainForm->client(), qsamplerDevice::Audio));

	// Populate MIDI input type list.
	m_ui.MidiDriverComboBox->clear();
	m_ui.MidiDriverComboBox->insertItems(0,
		qsamplerDevice::getDrivers(pMainForm->client(), qsamplerDevice::Midi));

	// Populate Maps list.
	m_ui.MidiMapComboBox->clear();
	m_ui.MidiMapComboBox->insertItems(0,
		qsamplerInstrument::getMapNames());

	// Read proper channel information,
	// and populate the channel form fields.

	// Engine name...
	QString sEngineName = pChannel->engineName();
	if (sEngineName.isEmpty() || bNew)
		sEngineName = pOptions->sEngineName;
	if (sEngineName.isEmpty())
		sEngineName = qsamplerChannel::noEngineName();
	if (m_ui.EngineNameComboBox->findText(sEngineName,
			Qt::MatchExactly | Qt::MatchCaseSensitive) < 0) {
		m_ui.EngineNameComboBox->addItem(sEngineName);
	}
	m_ui.EngineNameComboBox->setCurrentIndex(
		m_ui.EngineNameComboBox->findText(sEngineName,
			Qt::MatchExactly | Qt::MatchCaseSensitive));

	// Instrument filename and index...
	QString sInstrumentFile = pChannel->instrumentFile();
	if (sInstrumentFile.isEmpty())
		sInstrumentFile = qsamplerChannel::noInstrumentName();
	m_ui.InstrumentFileComboBox->setEditText(sInstrumentFile);
	m_ui.InstrumentNrComboBox->clear();
	m_ui.InstrumentNrComboBox->insertItems(0,
		qsamplerChannel::getInstrumentList(sInstrumentFile,
		pOptions->bInstrumentNames));
	int iInstrumentNr = pChannel->instrumentNr();
	if (iInstrumentNr < 0)
		iInstrumentNr = 0;
	m_ui.InstrumentNrComboBox->setCurrentIndex(iInstrumentNr);

	// MIDI input device...
	qsamplerDevice midiDevice(qsamplerDevice::Midi, m_pChannel->midiDevice());
	// MIDI input driver...
	QString sMidiDriver = midiDevice.driverName();
	if (sMidiDriver.isEmpty() || bNew)
		sMidiDriver = pOptions->sMidiDriver.toUpper();
	if (!sMidiDriver.isEmpty()) {
		if (m_ui.MidiDriverComboBox->findText(sMidiDriver,
				Qt::MatchExactly | Qt::MatchCaseSensitive) < 0) {
			m_ui.MidiDriverComboBox->insertItem(0, sMidiDriver);
		}
		m_ui.MidiDriverComboBox->setItemText(
			m_ui.MidiDriverComboBox->currentIndex(),
			sMidiDriver);
	}
	selectMidiDriverItem(sMidiDriver);
	if (!bNew) {
		m_ui.MidiDeviceComboBox->setItemText(
			m_ui.MidiDeviceComboBox->currentIndex(),
			midiDevice.deviceName());
	}
	selectMidiDeviceItem(m_ui.MidiDeviceComboBox->currentIndex());
	// MIDI input port...
	m_ui.MidiPortSpinBox->setValue(pChannel->midiPort());
	// MIDI input channel...
	int iMidiChannel = pChannel->midiChannel();
	// When new, try to suggest a sensible MIDI channel...
	if (iMidiChannel < 0)
		iMidiChannel = (::lscp_get_channels(pMainForm->client()) % 16);
	m_ui.MidiChannelComboBox->setCurrentIndex(iMidiChannel);
	// MIDI instrument map...
	int iMidiMap = (bNew ? pOptions->iMidiMap : pChannel->midiMap());
	// When new, try to suggest a sensible MIDI map...
	if (iMidiMap < 0)
		iMidiMap = 0;
	const QString& sMapName = qsamplerInstrument::getMapName(iMidiMap);
	if (!sMapName.isEmpty()) {
		m_ui.MidiMapComboBox->setItemText(
			m_ui.MidiMapComboBox->currentIndex(),
			sMapName);
	}
	// It might be no maps around...
	bool bMidiMapEnabled = (m_ui.MidiMapComboBox->count() > 0);
	m_ui.MidiMapTextLabel->setEnabled(bMidiMapEnabled);
	m_ui.MidiMapComboBox->setEnabled(bMidiMapEnabled);

	// Audio output device...
	qsamplerDevice audioDevice(qsamplerDevice::Audio, m_pChannel->audioDevice());
	// Audio output driver...
	QString sAudioDriver = audioDevice.driverName();
	if (sAudioDriver.isEmpty() || bNew)
		sAudioDriver = pOptions->sAudioDriver.toUpper();
	if (!sAudioDriver.isEmpty()) {
		if (m_ui.AudioDriverComboBox->findText(sAudioDriver,
				Qt::MatchExactly | Qt::MatchCaseSensitive) < 0) {
			m_ui.AudioDriverComboBox->insertItem(0, sAudioDriver);
		}
		m_ui.AudioDriverComboBox->setItemText(
			m_ui.AudioDriverComboBox->currentIndex(),
			sAudioDriver);
	}
	selectAudioDriverItem(sAudioDriver);
	if (!bNew) {
		m_ui.AudioDeviceComboBox->setItemText(
			m_ui.AudioDeviceComboBox->currentIndex(),
			audioDevice.deviceName());
	}
	selectAudioDeviceItem(m_ui.AudioDeviceComboBox->currentIndex());

	// Let the audio routing table see the light,
	// if we're editing an existing sampler channel...
	m_ui.AudioRoutingTable->setVisible(!bNew);

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
	//m_ui.AudioRoutingTable->flush();

	// We'll go for it!
	if (m_iDirtyCount > 0) {
		int iErrors = 0;
		// Are we a new channel?
		if (!m_pChannel->addChannel())
			iErrors++;
		// Accept Audio driver or device selection...
		if (m_audioDevices.isEmpty()) {
			if (!m_pChannel->setAudioDriver(m_ui.AudioDriverComboBox->currentText()))
				iErrors++;
		} else {
			qsamplerDevice *pDevice = NULL;
			int iAudioItem = m_ui.AudioDeviceComboBox->currentIndex();
			if (iAudioItem >= 0 && iAudioItem < m_audioDevices.count())
				pDevice = m_audioDevices.at(iAudioItem);
			qsamplerChannelRoutingMap routingMap = m_routingModel.routingMap();
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
			if (!m_pChannel->setMidiDriver(m_ui.MidiDriverComboBox->currentText()))
				iErrors++;
		} else {
			qsamplerDevice *pDevice = NULL;
			int iMidiItem = m_ui.MidiDeviceComboBox->currentIndex();
			if (iMidiItem >= 0 && iMidiItem < m_midiDevices.count())
				pDevice = m_midiDevices.at(iMidiItem);
			if (pDevice == NULL)
				iErrors++;
			else if (!m_pChannel->setMidiDevice(pDevice->deviceID()))
				iErrors++;
		}
		// MIDI input port number...
		if (!m_pChannel->setMidiPort(m_ui.MidiPortSpinBox->value()))
			iErrors++;
		// MIDI input channel...
		if (!m_pChannel->setMidiChannel(m_ui.MidiChannelComboBox->currentIndex()))
			iErrors++;
		// Engine name...
		if (!m_pChannel->loadEngine(m_ui.EngineNameComboBox->currentText()))
			iErrors++;
		// Instrument file and index...
		const QString& sPath = m_ui.InstrumentFileComboBox->currentText();
		if (!sPath.isEmpty() && QFileInfo(sPath).exists()) {
			if (!m_pChannel->loadInstrument(sPath, m_ui.InstrumentNrComboBox->currentIndex()))
				iErrors++;
		}
		// MIDI intrument map...
		if (!m_pChannel->setMidiMap(m_ui.MidiMapComboBox->currentIndex()))
			iErrors++;
		// Show error messages?
		if (iErrors > 0) {
			m_pChannel->appendMessagesError(
				tr("Some channel settings could not be set.\n\nSorry."));
		}
	}

	// Save default engine name, instrument directory and history...
	pOptions->sInstrumentDir = QFileInfo(
		m_ui.InstrumentFileComboBox->currentText()).dir().absolutePath();
	pOptions->sEngineName  = m_ui.EngineNameComboBox->currentText();
	pOptions->sAudioDriver = m_ui.AudioDriverComboBox->currentText();
	pOptions->sMidiDriver  = m_ui.MidiDriverComboBox->currentText();
	pOptions->iMidiMap     = m_ui.MidiMapComboBox->currentIndex();
	pOptions->saveComboBoxHistory(m_ui.InstrumentFileComboBox);

	// Just go with dialog acceptance.
	QDialog::accept();
}


// Reject settings (Cancel button slot).
void ChannelForm::reject (void)
{
	bool bReject = true;

	// Check if there's any pending changes...
	if (m_iDirtyCount > 0 && m_ui.OkPushButton->isEnabled()) {
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

	m_ui.InstrumentFileComboBox->setEditText(sInstrumentFile);
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
	m_ui.InstrumentNrComboBox->clear();
	m_ui.InstrumentNrComboBox->insertItems(0,
		qsamplerChannel::getInstrumentList(
			m_ui.InstrumentFileComboBox->currentText(),
			pOptions->bInstrumentNames)
	);

	optionsChanged();
}


// Show device options dialog.
void ChannelForm::setupDevice ( qsamplerDevice *pDevice,
	qsamplerDevice::DeviceType deviceTypeMode,
	const QString& sDriverName )
{
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
	int iMidiItem = m_ui.MidiDeviceComboBox->currentIndex();
	if (iMidiItem >= 0 && iMidiItem < m_midiDevices.count())
		pDevice = m_midiDevices.at(iMidiItem);
	if (pDevice)
		iDeviceID = pDevice->deviceID();

	// Clean maplist.
	m_ui.MidiDeviceComboBox->clear();
	qDeleteAll(m_midiDevices);
	m_midiDevices.clear();

	// Populate with the current ones...
	const QPixmap midiPixmap(":/icons/midi2.png");
	int *piDeviceIDs = qsamplerDevice::getDevices(pMainForm->client(),
		qsamplerDevice::Midi);
	for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
		pDevice = new qsamplerDevice(qsamplerDevice::Midi, piDeviceIDs[i]);
		if (pDevice->driverName().toUpper() == sDriverName) {
			m_ui.MidiDeviceComboBox->insertItem(0,
				midiPixmap, pDevice->deviceName());
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
				m_ui.MidiDeviceComboBox->setCurrentIndex(iMidiItem);
				selectMidiDeviceItem(iMidiItem);
				break;
			}
			iMidiItem++;
		}
	} else {
		m_ui.MidiDeviceComboBox->insertItem(0,
			tr("(New MIDI %1 device)").arg(sMidiDriver));
	}
	m_ui.MidiDeviceTextLabel->setEnabled(bEnabled);
	m_ui.MidiDeviceComboBox->setEnabled(bEnabled);
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
		m_ui.MidiPortTextLabel->setEnabled(iPorts > 0);
		m_ui.MidiPortSpinBox->setEnabled(iPorts > 0);
		if (iPorts > 0)
			m_ui.MidiPortSpinBox->setMaximum(iPorts - 1);
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
	int iMidiItem = m_ui.MidiDeviceComboBox->currentIndex();
	if (iMidiItem >= 0 && iMidiItem < m_midiDevices.count())
		pDevice = m_midiDevices.at(iMidiItem);
	setupDevice(pDevice,
		qsamplerDevice::Midi, m_ui.MidiDriverComboBox->currentText());
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
	int iAudioItem = m_ui.AudioDeviceComboBox->currentIndex();
	if (iAudioItem >= 0 && iAudioItem < m_audioDevices.count())
		pDevice = m_audioDevices.at(iAudioItem);
	if (pDevice)
		iDeviceID = pDevice->deviceID();

	// Clean maplist.
	m_ui.AudioDeviceComboBox->clear();
	qDeleteAll(m_audioDevices);
	m_audioDevices.clear();

	// Populate with the current ones...
	const QPixmap audioPixmap(":/icons/audio2.png");
	int *piDeviceIDs = qsamplerDevice::getDevices(pMainForm->client(),
		qsamplerDevice::Audio);
	for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
		pDevice = new qsamplerDevice(qsamplerDevice::Audio, piDeviceIDs[i]);
		if (pDevice->driverName().toUpper() == sDriverName) {
			m_ui.AudioDeviceComboBox->insertItem(0,
				audioPixmap, pDevice->deviceName());
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
				m_ui.AudioDeviceComboBox->setCurrentIndex(iAudioItem);
				selectAudioDeviceItem(iAudioItem);
				break;
			}
			iAudioItem++;
		}
	} else {
		m_ui.AudioDeviceComboBox->insertItem(0,
			tr("(New Audio %1 device)").arg(sAudioDriver));
		//m_ui.AudioRoutingTable->setNumRows(0);
	}
	m_ui.AudioDeviceTextLabel->setEnabled(bEnabled);
	m_ui.AudioDeviceComboBox->setEnabled(bEnabled);
	m_ui.AudioRoutingTable->setEnabled(bEnabled);
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
		m_routingModel.refresh(pDevice, m_pChannel->audioRouting());
		// Reset routing change map.
		m_routingModel.clear();
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
	int iAudioItem = m_ui.AudioDeviceComboBox->currentIndex();
	if (iAudioItem >= 0 && iAudioItem < m_audioDevices.count())
		pDevice = m_audioDevices.at(iAudioItem);
	setupDevice(pDevice,
		qsamplerDevice::Audio, m_ui.AudioDriverComboBox->currentText());
}

// UPdate all device lists slot.
void ChannelForm::updateDevices (void)
{
	if (m_iDirtySetup > 0)
		return;

	selectMidiDriverItem(m_ui.MidiDriverComboBox->currentText());
	selectAudioDriverItem(m_ui.AudioDriverComboBox->currentText());
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
	const bool bValid =
		m_ui.EngineNameComboBox->currentIndex() >= 0 &&
		m_ui.EngineNameComboBox->currentText()
			!= qsamplerChannel::noEngineName();
#if 0
	const QString& sPath = InstrumentFileComboBox->currentText();
	bValid = bValid && !sPath.isEmpty() && QFileInfo(sPath).exists();
#endif
	m_ui.OkPushButton->setEnabled(m_iDirtyCount > 0 && bValid);
}


void ChannelForm::updateTableCellRenderers (void)
{
	const int rows = m_routingModel.rowCount();
	const int cols = m_routingModel.columnCount();
	updateTableCellRenderers(
		m_routingModel.index(0, 0),
		m_routingModel.index(rows - 1, cols - 1));
}


void ChannelForm::updateTableCellRenderers (
	const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
	for (int r = topLeft.row(); r <= bottomRight.row(); r++) {
		for (int c = topLeft.column(); c <= bottomRight.column(); c++) {
			const QModelIndex index = m_routingModel.index(r, c);
			m_ui.AudioRoutingTable->openPersistentEditor(index);
		}
	}
}

} // namespace QSampler


// end of qsamplerChannelForm.cpp
