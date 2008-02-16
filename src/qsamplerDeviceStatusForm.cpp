// qsamplerDeviceStatusForm.cpp
//
/****************************************************************************
   Copyright (C) 2008, Christian Schoenebeck

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

#include "qsamplerAbout.h"
#include "qsamplerDeviceStatusForm.h"
#include "qsamplerMainForm.h"

#include <QGridLayout>

#define MIDI_OFF_COLOR			Qt::darkGreen
#define MIDI_ON_COLOR			Qt::green

namespace QSampler {

MidiActivityLED::MidiActivityLED(QString text, QWidget* parent) : QLabel(text, parent) {
#if CONFIG_LSCP_DEVICE_MIDI
	setPalette(MIDI_OFF_COLOR);
	setAutoFillBackground(true);
#else
	setText("X");
	setTooltip("MIDI Activity Disabled");
#endif
	timer.setSingleShot(true);
	QObject::connect(
		&timer, SIGNAL(timeout()),
		this, SLOT(midiDataCeased())
	);
}

void MidiActivityLED::midiDataArrived() {
	setPalette(MIDI_ON_COLOR);
	timer.start(50);
}

void MidiActivityLED::midiDataCeased() {
	setPalette(MIDI_OFF_COLOR);
}


std::map<int, DeviceStatusForm*> DeviceStatusForm::instances;

DeviceStatusForm::DeviceStatusForm (
	int DeviceID, QWidget* pParent, Qt::WindowFlags wflags )
	: QMainWindow(pParent, wflags)
{
	m_pDevice = new Device(Device::Midi, DeviceID);
	DevicePortList ports = m_pDevice->ports();

	if (!centralWidget()) setCentralWidget(new QWidget(this));

	QGridLayout* pLayout = new QGridLayout(centralWidget());
	for (int i = 0; i < ports.size(); ++i) {
		QLabel* pLabel =
			new QLabel(QString("MIDI port \"") + ports[i]->portName() + "\": ");
		pLabel->setToolTip(QString("Device ID ") + QString::number(ports[i]->portID()));
		pLayout->addWidget(pLabel, i, 0, Qt::AlignLeft);
		MidiActivityLED* pLED = new MidiActivityLED();
		midiActivityLEDs.push_back(pLED);
		pLayout->addWidget(pLED, i, 1);
	}
	centralWidget()->setLayout(pLayout);

	m_pVisibleAction = new QAction(this);
	m_pVisibleAction->setCheckable(true);
	m_pVisibleAction->setChecked(false);
	m_pVisibleAction->setText(m_pDevice->deviceName());
	m_pVisibleAction->setToolTip(
		QString("MIDI Device ID: ") +
		QString::number(m_pDevice->deviceID())
	);
	QObject::connect(
		m_pVisibleAction, SIGNAL(toggled(bool)),
		this, SLOT(setVisible(bool))
	);

	setWindowTitle(m_pDevice->deviceName() + " Status");
}

DeviceStatusForm::~DeviceStatusForm() {
	if (m_pDevice) delete m_pDevice;
}

QAction* DeviceStatusForm::visibleAction() {
	return m_pVisibleAction;
}

void DeviceStatusForm::closeEvent(QCloseEvent* event) {
	m_pVisibleAction->setChecked(false);
	event->accept();
}

void DeviceStatusForm::midiArrived(int iPort) {
	if (uint(iPort) >= midiActivityLEDs.size()) return;
	midiActivityLEDs[iPort]->midiDataArrived();
}

DeviceStatusForm* DeviceStatusForm::getInstance(int iDeviceID) {
	std::map<int, DeviceStatusForm*>::iterator iter =
		instances.find(iDeviceID);
	return (iter != instances.end()) ? iter->second : NULL;
}

const std::map<int, DeviceStatusForm*>& DeviceStatusForm::getInstances() {
	return instances;
}

void DeviceStatusForm::onDevicesChanged() {
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm && pMainForm->client()) {
		std::set<int> deviceIDs =
			Device::getDeviceIDs(pMainForm->client(), Device::Midi);
		// hide and delete status forms whose device has been destroyed
		for (
			std::map<int, DeviceStatusForm*>::iterator iter = instances.begin();
			iter != instances.end(); ++iter
		) {
			if (deviceIDs.find(iter->first) == deviceIDs.end()) {
				iter->second->hide();
				delete iter->second;
				instances.erase(iter);
			}
		}
		// create status forms for new devices
		for (
			std::set<int>::iterator iter = deviceIDs.begin();
			iter != deviceIDs.end(); ++iter
		) {
			if (instances.find(*iter) == instances.end()) {
				DeviceStatusForm* pStatusForm =
					new DeviceStatusForm(*iter, pMainForm);
				instances[*iter] = pStatusForm;
			}
		}
	}
}

} // namespace QSampler

// end of qsamplerDeviceStatusForm.cpp
