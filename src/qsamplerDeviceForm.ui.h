// qsamplerDeviceForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2005, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qlistbox.h>
#include <qptrlist.h>
#include <qpopupmenu.h>

#include "qsamplerMainForm.h"

#include "config.h"


// Kind of constructor.
void qsamplerDeviceForm::init (void)
{
	// Initialize locals.
	m_pMainForm   = NULL;
	m_pClient     = NULL;
	m_iDirtySetup = 0;
	m_bNewDevice  = false;
	m_deviceType  = qsamplerDevice::None;
	m_pAudioItems = NULL;
	m_pMidiItems  = NULL;
	
	// This an outsider (from designer), but rather important.
	QObject::connect(DeviceParamTable, SIGNAL(valueChanged(int,int)),
		this, SLOT(changeDeviceParam(int,int)));
	QObject::connect(DevicePortParamTable, SIGNAL(valueChanged(int,int)),
		this, SLOT(changeDevicePortParam(int,int)));

	// Initial contents.
	refreshDevices();
	// Try to restore normal window positioning.
	adjustSize();
}


// Kind of destructor.
void qsamplerDeviceForm::destroy (void)
{
}


// Notify our parent that we're emerging.
void qsamplerDeviceForm::showEvent ( QShowEvent *pShowEvent )
{
	if (m_pMainForm)
		m_pMainForm->stabilizeForm();

	stabilizeForm();

	QWidget::showEvent(pShowEvent);
}


// Notify our parent that we're closing.
void qsamplerDeviceForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	if (m_pMainForm)
		m_pMainForm->stabilizeForm();
}


// Application main form settler (life depends on it).
void qsamplerDeviceForm::setMainForm ( qsamplerMainForm *pMainForm )
{
   m_pMainForm = pMainForm;
}


// Device configuration dialog setup formal initializer.
void qsamplerDeviceForm::setClient ( lscp_client_t *pClient )
{
	// If it has not changed, do nothing.
	if (m_pClient && m_pClient == pClient)
		return;

	// Set new reference.
	m_pClient = pClient;
	
	// OK. Do a whole refresh around.
	refreshDevices();
}


// Set current selected device by type and id.
void qsamplerDeviceForm::setDevice (
	qsamplerDevice::qsamplerDeviceType deviceType, int iDeviceID )
{
	// Get the device view root item...
	qsamplerDeviceItem *pRootItem = NULL;
	switch (deviceType) {
	case qsamplerDevice::Audio:
		pRootItem = m_pAudioItems;
		break;
	case qsamplerDevice::Midi:
		pRootItem = m_pMidiItems;
		break;
	case qsamplerDevice::None:
		break;
	}
	
	// Is the root present?
	if (pRootItem == NULL)
	    return;

	// For each child, test for identity...
	qsamplerDeviceItem *pDeviceItem =
		(qsamplerDeviceItem *) pRootItem->firstChild();
	while (pDeviceItem) {
		// If identities match, select as current device item.
		if (pDeviceItem->device().deviceID() == iDeviceID) {
			DeviceListView->setSelected(pDeviceItem, true);
			break;
		}
		pDeviceItem = (qsamplerDeviceItem *) pDeviceItem->nextSibling();
	}
}



// Create a new device from current table view.
void qsamplerDeviceForm::createDevice (void)
{
	if (m_pMainForm == NULL)
	    return;

	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM)
		return;

	// About a brand new device instance...
	qsamplerDevice device(((qsamplerDeviceItem *) pItem)->device());
	if (device.createDevice()) {
		// Now it depends on the device type...
		qsamplerDeviceItem *pRootItem = NULL;
		switch (device.deviceType()) {
		case qsamplerDevice::Audio:
			pRootItem = m_pAudioItems;
			break;
		case qsamplerDevice::Midi:
			pRootItem = m_pMidiItems;
			break;
		case qsamplerDevice::None:
			break;
		}
		// Append the new device item.
		qsamplerDeviceItem *pDeviceItem = new qsamplerDeviceItem(pRootItem,
			m_pMainForm, device.deviceType(), device.deviceID());
		// Just make it the new selection...
		DeviceListView->setSelected(pDeviceItem, true);
		// Main session should be marked dirty.
		m_pMainForm->sessionDirty();
	}
}


// Delete current device in table view.
void qsamplerDeviceForm::deleteDevice (void)
{
	if (m_pMainForm == NULL)
	    return;

	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	// Prompt user if this is for real...
	qsamplerOptions *pOptions = m_pMainForm->options();
	if (pOptions && pOptions->bConfirmRemove) {
		if (QMessageBox::warning(this, tr("Warning"),
			tr("Delete device:\n\n"
			"%1\n\n"
			"Are you sure?")
			.arg(device.deviceName()),
			tr("OK"), tr("Cancel")) > 0)
			return;
	}

	// Go and destroy...
	if (device.deleteDevice()) {
		// Remove it from the device view...
		delete pItem;
		// Main session should be marked dirty.
		m_pMainForm->sessionDirty();
	}
}


// Refresh all device list and views.
void qsamplerDeviceForm::refreshDevices (void)
{
	if (m_pMainForm == NULL)
	    return;

	// Avoid nested changes.
	m_iDirtySetup++;

	//
	// (Re)Load complete device configuration data ...
	//
	m_pAudioItems = NULL;
	m_pMidiItems = NULL;
	DeviceListView->clear();
	if (m_pClient) {
		int *piDeviceIDs;
		// Grab and pop Audio devices...
		m_pAudioItems = new qsamplerDeviceItem(DeviceListView,
			m_pMainForm, qsamplerDevice::Audio);
		if (m_pAudioItems) {
			piDeviceIDs = qsamplerDevice::getDevices(m_pClient, qsamplerDevice::Audio);
			for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
				new qsamplerDeviceItem(m_pAudioItems, m_pMainForm,
					qsamplerDevice::Audio, piDeviceIDs[i]);
			}
			m_pAudioItems->setOpen(true);
		}
		// Grab and pop MIDI devices...
		m_pMidiItems = new qsamplerDeviceItem(DeviceListView,
			m_pMainForm, qsamplerDevice::Midi);
		if (m_pMidiItems) {
			piDeviceIDs = qsamplerDevice::getDevices(m_pClient, qsamplerDevice::Midi);
			for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
				new qsamplerDeviceItem(m_pMidiItems, m_pMainForm,
					qsamplerDevice::Midi, piDeviceIDs[i]);
			}
			m_pMidiItems->setOpen(true);
		}
	}

	// Done.
	m_iDirtySetup--;

	// Show something.
	selectDevice();
}


// Driver selection slot.
void qsamplerDeviceForm::selectDriver ( const QString& sDriverName )
{
	if (m_iDirtySetup > 0)
		return;

	//
	//  Driver name has changed for a new device...
	//

	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	// Driver change is only valid for scratch devices...
	if (m_bNewDevice) {
		m_iDirtySetup++;
		device.setDriver(sDriverName);
		DeviceParamTable->refresh(device.params(), m_bNewDevice);
		m_iDirtySetup--;
		// Done.
		stabilizeForm();
	}
}


// Device selection slot.
void qsamplerDeviceForm::selectDevice (void)
{
	if (m_iDirtySetup > 0)
		return;

	//
	//  Device selection has changed...
	//

	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM) {
		m_deviceType = qsamplerDevice::None;
		DeviceNameTextLabel->setText(QString::null);
		DeviceParamTable->setNumRows(0);
		DevicePortComboBox->clear();
		DevicePortParamTable->setNumRows(0);
		DevicePortComboBox->setEnabled(false);
		DevicePortParamTable->setEnabled(false);
		stabilizeForm();
		return;
	}

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	m_iDirtySetup++;
	// Flag whether this is a new device.
	m_bNewDevice = (device.deviceID() < 0);

	// Fill the device/driver heading...
	DeviceNameTextLabel->setText(device.deviceName());
	// The driver combobox is only rebuilt if device type has changed...
	if (device.deviceType() != m_deviceType) {
		DriverNameComboBox->clear();
		DriverNameComboBox->insertStringList(
			qsamplerDevice::getDrivers(m_pClient, device.deviceType()));
		m_deviceType = device.deviceType();
	}
	// Do we need a driver name?
	if (m_bNewDevice || device.driverName().isEmpty())
		device.setDriver(DriverNameComboBox->currentText());
	const QString& sDriverName = device.driverName();
	if (DriverNameComboBox->listBox()->findItem(sDriverName, Qt::ExactMatch) == NULL)
		DriverNameComboBox->insertItem(sDriverName);
	DriverNameComboBox->setCurrentText(sDriverName);
	DriverNameTextLabel->setEnabled(m_bNewDevice);
	DriverNameComboBox->setEnabled(m_bNewDevice);
	// Fill the device parameter table...
	DeviceParamTable->refresh(device.params(), m_bNewDevice);
	// And now the device port/channel parameter table...
	DevicePortComboBox->clear();
	DevicePortParamTable->setNumRows(0);
	if (m_bNewDevice) {
		DevicePortComboBox->setEnabled(false);
		DevicePortParamTable->setEnabled(false);
	} else {
		QPixmap pixmap;
		switch (device.deviceType()) {
		case qsamplerDevice::Audio:
		    pixmap = QPixmap::fromMimeSource("audio2.png");
		    break;
		case qsamplerDevice::Midi:
		    pixmap = QPixmap::fromMimeSource("midi2.png");
		    break;
		case qsamplerDevice::None:
		    break;
		}
		qsamplerDevicePortList& ports = device.ports();
		qsamplerDevicePort *pPort;
		for (pPort = ports.first(); pPort; pPort = ports.next()) {
            DevicePortComboBox->insertItem(pixmap, device.deviceTypeName()
				+ ' ' + device.driverName()
				+ ' ' + pPort->portName());
		}
		bool bEnabled = (ports.count() > 0);
		DevicePortComboBox->setEnabled(bEnabled);
		DevicePortParamTable->setEnabled(bEnabled);
	}
	// Done.
	m_iDirtySetup--;
	
	// Make the device port/channel selection effective.
	selectDevicePort(DevicePortComboBox->currentItem());
}


// Device port/channel selection slot.
void qsamplerDeviceForm::selectDevicePort ( int iPort )
{
	if (m_iDirtySetup > 0)
		return;

	//
	//  Device port/channel selection has changed...
	//

	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();
	qsamplerDevicePort *pPort = device.ports().at(iPort);
	if (pPort) {
		m_iDirtySetup++;
		DevicePortParamTable->refresh(pPort->params(), false);
		m_iDirtySetup--;
	}
	// Done.
	stabilizeForm();
}


// Device parameter value change slot.
void qsamplerDeviceForm::changeDeviceParam ( int iRow, int iCol )
{
	if (m_pMainForm == NULL)
	    return;
	if (m_iDirtySetup > 0)
		return;
	if (iRow < 0 || iCol < 0)
		return;
		
	//
	//  Device parameter change...
	//

	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	// Table 1st column has the parameter name;
	const QString sParam = DeviceParamTable->text(iRow, 0);
	const QString sValue = DeviceParamTable->text(iRow, iCol);
	// Set the local device parameter value.
	if (device.setParam(sParam, sValue)) {
		selectDevice();
	} else {
		stabilizeForm();
	}
	// Main session should be dirtier...
	m_pMainForm->sessionDirty();
}


// Device port/channel parameter value change slot.
void qsamplerDeviceForm::changeDevicePortParam ( int iRow, int iCol )
{
	if (m_pMainForm == NULL)
	    return;
	if (m_iDirtySetup > 0)
		return;
	if (iRow < 0 || iCol < 0)
		return;

	//
	//  Device port/channel parameter change...
	//

	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	int iPort = DevicePortComboBox->currentItem();
	qsamplerDevicePort *pPort = device.ports().at(iPort);
	if (pPort == NULL)
	    return;

	// Table 1st column has the parameter name;
	const QString sParam = DevicePortParamTable->text(iRow, 0);
	const QString sValue = DevicePortParamTable->text(iRow, iCol);
	// Set the local device port/channel parameter value.
	pPort->setParam(sParam, sValue);
	// Done.
	stabilizeForm();
	// Main session should be dirtier...
	m_pMainForm->sessionDirty();
}


// Device list view context menu handler.
void qsamplerDeviceForm::contextMenu ( QListViewItem *pItem, const QPoint& pos, int )
{
	int iItemID;
	
	// Build the device context menu...
	QPopupMenu* pContextMenu = new QPopupMenu(this);
	
	bool bClient = (m_pClient != NULL);
	bool bEnabled = (pItem != NULL);
	iItemID = pContextMenu->insertItem(
		QIconSet(QPixmap::fromMimeSource("deviceCreate.png")),
		tr("&Create device"), this, SLOT(createDevice()));
	pContextMenu->setItemEnabled(iItemID, bEnabled || (bClient && m_bNewDevice));
	iItemID = pContextMenu->insertItem(
		QIconSet(QPixmap::fromMimeSource("deviceDelete.png")),
		tr("&Delete device"), this, SLOT(deleteDevice()));
	pContextMenu->setItemEnabled(iItemID, bEnabled && !m_bNewDevice);
	pContextMenu->insertSeparator();
	iItemID = pContextMenu->insertItem(
		QIconSet(QPixmap::fromMimeSource("formRefresh.png")),
		tr("&Refresh"), this, SLOT(refreshDevices()));
	pContextMenu->setItemEnabled(iItemID, bClient);
	
	pContextMenu->exec(pos);
	
	delete pContextMenu;
}


// Stabilize current form state.
void qsamplerDeviceForm::stabilizeForm (void)
{
	QListViewItem *pItem = DeviceListView->selectedItem();
	bool bClient = (m_pClient != NULL);
	bool bEnabled = (pItem != NULL);
	DeviceNameTextLabel->setEnabled(bEnabled && !m_bNewDevice);
	DriverNameTextLabel->setEnabled(bEnabled &&  m_bNewDevice);
	DriverNameComboBox->setEnabled(bEnabled && m_bNewDevice);
	DeviceParamTable->setEnabled(bEnabled);
	RefreshDevicesPushButton->setEnabled(bClient);
	CreateDevicePushButton->setEnabled(bEnabled || (bClient && m_bNewDevice));
	DeleteDevicePushButton->setEnabled(bEnabled && !m_bNewDevice);
}


// end of qsamplerDeviceForm.ui.h



