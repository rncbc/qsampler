// qsamplerDeviceStatusForm.cpp
//
/****************************************************************************
   Copyright (C) 2008, Christian Schoenebeck
   Copyright (C) 2010, rncbc aka Rui Nuno Capela. All rights reserved.

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


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::MidiActivityLED -- Graphical indicator for MIDI activity.
//

// MIDI activity pixmap common resources.
int      MidiActivityLED::g_iMidiActivityRefCount = 0;
QPixmap *MidiActivityLED::g_pMidiActivityLedOn    = NULL;
QPixmap *MidiActivityLED::g_pMidiActivityLedOff   = NULL;


MidiActivityLED::MidiActivityLED ( QString sText, QWidget *pParent )
	: QLabel(sText, pParent)
{
	if (++g_iMidiActivityRefCount == 1) {
		g_pMidiActivityLedOn  = new QPixmap(":/icons/ledon1.png");
		g_pMidiActivityLedOff = new QPixmap(":/icons/ledoff1.png");
	}

	setPixmap(*g_pMidiActivityLedOff);
#ifndef CONFIG_EVENT_DEVICE_MIDI
	setToolTip("MIDI Activity disabled");
#endif
	m_timer.setSingleShot(true);

	QObject::connect(&m_timer,
		SIGNAL(timeout()),
		SLOT(midiActivityLedOff())
	);
}

MidiActivityLED::~MidiActivityLED (void)
{
	if (--g_iMidiActivityRefCount == 0) {
		if (g_pMidiActivityLedOn)
			delete g_pMidiActivityLedOn;
		g_pMidiActivityLedOn = NULL;
		if (g_pMidiActivityLedOff)
			delete g_pMidiActivityLedOff;
		g_pMidiActivityLedOff = NULL;
	}
}


void MidiActivityLED::midiActivityLedOn (void)
{
	setPixmap(*g_pMidiActivityLedOn);
	m_timer.start(100);
}


void MidiActivityLED::midiActivityLedOff (void)
{
	setPixmap(*g_pMidiActivityLedOff);
}


//-------------------------------------------------------------------------
// QSampler::DeviceStatusForm -- Device status informations window.
//

std::map<int, DeviceStatusForm *> DeviceStatusForm::g_instances;


DeviceStatusForm::DeviceStatusForm (
	int DeviceID, QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	m_pDevice = new Device(Device::Midi, DeviceID);

	setLayout(new QGridLayout(/*this*/));
	updateGUIPorts(); // build the GUI

	m_pVisibleAction = new QAction(this);
	m_pVisibleAction->setCheckable(true);
	m_pVisibleAction->setChecked(false);
	m_pVisibleAction->setText(m_pDevice->deviceName());
	m_pVisibleAction->setToolTip(
		QString("MIDI Device ID: ") +
		QString::number(m_pDevice->deviceID())
	);

	QObject::connect(m_pVisibleAction,
		SIGNAL(toggled(bool)),
		SLOT(setVisible(bool))
	);

	setWindowTitle(tr("%1 Status").arg(m_pDevice->deviceName()));
}


void DeviceStatusForm::updateGUIPorts (void)
{
	// refresh device informations
	m_pDevice->setDevice(m_pDevice->deviceType(), m_pDevice->deviceID());
	DevicePortList ports = m_pDevice->ports();

	// clear the GUI
	QGridLayout *pLayout = static_cast<QGridLayout *> (layout());
	for (int i = pLayout->count() - 1; i >= 0; --i) {
		QLayoutItem *pItem = pLayout->itemAt(i);
		if (pItem) {
			pLayout->removeItem(pItem);
			if (pItem->widget())
				delete pItem->widget();
			delete pItem;
		}
	}

	m_midiActivityLEDs.clear();

	// rebuild the GUI
	for (int i = 0; i < ports.size(); ++i) {
		MidiActivityLED *pLED = new MidiActivityLED();
		m_midiActivityLEDs.push_back(pLED);
		pLayout->addWidget(pLED, i, 0);
		QLabel *pLabel = new QLabel(
			m_pDevice->deviceTypeName()
			+ ' ' + m_pDevice->driverName()
			+ ' ' + ports[i]->portName());
		pLayout->addWidget(pLabel, i, 1, Qt::AlignLeft);
	}
}


DeviceStatusForm::~DeviceStatusForm (void)
{
	if (m_pDevice) delete m_pDevice;
}


QAction* DeviceStatusForm::visibleAction (void)
{
	return m_pVisibleAction;
}

void DeviceStatusForm::closeEvent ( QCloseEvent *pCloseEvent )
{
	m_pVisibleAction->setChecked(false);

	pCloseEvent->accept();
}


void DeviceStatusForm::midiArrived ( int iPort )
{
	if (uint(iPort) >= m_midiActivityLEDs.size())
		return;

	m_midiActivityLEDs[iPort]->midiActivityLedOn();
}


DeviceStatusForm *DeviceStatusForm::getInstance ( int iDeviceID )
{
	std::map<int, DeviceStatusForm *>::iterator iter
		= g_instances.find(iDeviceID);
	return ((iter != g_instances.end()) ? iter->second : NULL);
}


const std::map<int, DeviceStatusForm *>& DeviceStatusForm::getInstances (void)
{
	return g_instances;
}

void DeviceStatusForm::deleteAllInstances (void)
{
	std::map<int, DeviceStatusForm *>::iterator iter = g_instances.begin();
	for ( ; iter != g_instances.end(); ++iter) {
		iter->second->hide();
		delete iter->second;
	}

	g_instances.clear();
}


void DeviceStatusForm::onDevicesChanged (void)
{
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm && pMainForm->client()) {
		std::set<int> deviceIDs
			= Device::getDeviceIDs(pMainForm->client(), Device::Midi);
		// hide and delete status forms whose device has been destroyed
		std::map<int, DeviceStatusForm *>::iterator iter = g_instances.begin();
		for ( ; iter != g_instances.end(); ++iter) {
			if (deviceIDs.find(iter->first) == deviceIDs.end()) {
				iter->second->hide();
				delete iter->second;
				g_instances.erase(iter);
			}
		}
		// create status forms for new devices
		std::set<int>::iterator it = deviceIDs.begin();
		for ( ; it != deviceIDs.end(); ++it) {
			if (g_instances.find(*it) == g_instances.end()) {
				// What style do we create these forms?
				Qt::WindowFlags wflags = Qt::Window
					| Qt::CustomizeWindowHint
					| Qt::WindowTitleHint
					| Qt::WindowSystemMenuHint
					| Qt::WindowMinMaxButtonsHint
					| Qt::WindowCloseButtonHint;
				Options *pOptions = pMainForm->options();
				if (pOptions && pOptions->bKeepOnTop)
					wflags |= Qt::Tool;
				// Create the form, giving it the device id.
				DeviceStatusForm *pStatusForm
					= new DeviceStatusForm(*it, NULL, wflags);
				g_instances[*it] = pStatusForm;
			}
		}
	}
}


void DeviceStatusForm::onDeviceChanged ( int iDeviceID )
{
	DeviceStatusForm *pStatusForm
		= DeviceStatusForm::getInstance(iDeviceID);
	if (pStatusForm)
		pStatusForm->updateGUIPorts();
}


} // namespace QSampler

// end of qsamplerDeviceStatusForm.cpp
