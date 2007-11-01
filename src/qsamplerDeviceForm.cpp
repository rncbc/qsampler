// qsamplerDeviceForm.cpp
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

#include "qsamplerDeviceForm.h"

#include "qsamplerAbout.h"

namespace QSampler {

DeviceForm::DeviceForm(QWidget* parent, Qt::WFlags f) : QDialog(parent, f) {
    ui.setupUi(this);

	// Initialize locals.
	m_iDirtySetup = 0;
	m_iDirtyCount = 0;
	m_bNewDevice  = false;
	m_deviceType  = qsamplerDevice::None;
	m_pAudioItems = NULL;
	m_pMidiItems  = NULL;
	// No exclusive mode as default.
	m_deviceTypeMode = qsamplerDevice::None;

	ui.DeviceParamTable->setModel(&deviceParamModel);
	ui.DeviceParamTable->setItemDelegate(&deviceParamDelegate);

	ui.DevicePortParamTable->setModel(&devicePortParamModel);
	ui.DevicePortParamTable->setItemDelegate(&devicePortParamDelegate);

	// This an outsider (from designer), but rather important.
	//QObject::connect(DeviceParamTable, SIGNAL(valueChanged(int,int)),
	//	this, SLOT(changeDeviceParam(int,int)));
	//QObject::connect(DevicePortParamTable, SIGNAL(valueChanged(int,int)),
	//	this, SLOT(changeDevicePortParam(int,int)));

	// Initial contents.
	refreshDevices();
	// Try to restore normal window positioning.
	adjustSize();

	QObject::connect(ui.DeviceListView,
		SIGNAL(selectionChanged()),
		SLOT(selectDevice()));
	QObject::connect(ui.DeviceListView,
		SIGNAL(contextMenuRequested(QListViewItem*,const QPoint&amp;,int)),
		SLOT(contextMenu(QListViewItem*,const QPoint&amp;,int)));
	QObject::connect(ui.RefreshDevicesPushButton,
		SIGNAL(clicked()),
		SLOT(refreshDevices()));
	QObject::connect(ui.DriverNameComboBox,
		SIGNAL(activated(const QString&amp;)),
		SLOT(selectDriver(const QString&amp;)));
	QObject::connect(ui.DevicePortComboBox,
		SIGNAL(activated(int)),
		SLOT(selectDevicePort(int)));
	QObject::connect(ui.CreateDevicePushButton,
		SIGNAL(clicked()),
		SLOT(createDevice()));
	QObject::connect(ui.DeleteDevicePushButton,
		SIGNAL(clicked()),
		SLOT(deleteDevice()));
	QObject::connect(ui.ClosePushButton,
		SIGNAL(clicked()),
		SLOT(close()));
}

DeviceForm::~DeviceForm() {
}


// Notify our parent that we're emerging.
void DeviceForm::showEvent ( QShowEvent *pShowEvent )
{
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	stabilizeForm();

	QWidget::showEvent(pShowEvent);
}


// Notify our parent that we're closing.
void DeviceForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	// Signal special whether we changed the device set.
	if (m_iDirtyCount > 0) {
		m_iDirtyCount = 0;
		emit devicesChanged();
	}
}


// Set device type spacial exclusive mode.
void DeviceForm::setDeviceTypeMode (
	qsamplerDevice::qsamplerDeviceType deviceTypeMode )
{
	// If it has not changed, do nothing.
	if (m_deviceTypeMode == deviceTypeMode)
		return;

	m_deviceTypeMode = deviceTypeMode;

	// OK. Do a whole refresh around.
	refreshDevices();
}


// Device driver name setup formal initializer.
void DeviceForm::setDriverName ( const QString& sDriverName )
{
	if (ui.DriverNameComboBox->findText(sDriverName) == 0) {
		ui.DriverNameComboBox->insertItem(sDriverName);
	}
	ui.DriverNameComboBox->setCurrentText(sDriverName);
}


// Set current selected device by type and id.
void DeviceForm::setDevice ( qsamplerDevice *pDevice )
{
	// In case no device is given...
	qsamplerDevice::qsamplerDeviceType deviceType = m_deviceTypeMode;
	if (pDevice)
		deviceType = pDevice->deviceType();

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

	// So there's no device huh?
	if (pDevice == NULL) {
		ui.DeviceListView->setCurrentItem(pRootItem);
		return;
	}

	// For each child, test for identity...
	for (int i = 0; i < pRootItem->childCount(); i++) {
		qsamplerDeviceItem* pDeviceItem =
			(qsamplerDeviceItem*) pRootItem->child(i);

		// If identities match, select as current device item.
		if (pDeviceItem->device().deviceID() == pDevice->deviceID()) {
			pDeviceItem->setSelected(true);
			break;
		}
	}
}



// Create a new device from current table view.
void DeviceForm::createDevice (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	QTreeWidgetItem *pItem = ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
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
			device.deviceType(), device.deviceID());
		// Just make it the new selection...
		pDeviceItem->setSelected(true);
		// Main session should be marked dirty.
		pMainForm->sessionDirty();
		m_iDirtyCount++;
	}
}


// Delete current device in table view.
void DeviceForm::deleteDevice (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	QTreeWidgetItem* pItem = ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	// Prompt user if this is for real...
	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions && pOptions->bConfirmRemove) {
		if (QMessageBox::warning(this,
			QSAMPLER_TITLE ": " + tr("Warning"),
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
		pMainForm->sessionDirty();
		m_iDirtyCount++;
	}
}


// Refresh all device list and views.
void DeviceForm::refreshDevices (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	// Avoid nested changes.
	m_iDirtySetup++;

	//
	// (Re)Load complete device configuration data ...
	//
	m_pAudioItems = NULL;
	m_pMidiItems = NULL;
	ui.DeviceListView->clear();
	if (pMainForm->client()) {
		int *piDeviceIDs;
		// Grab and pop Audio devices...
		if (m_deviceTypeMode == qsamplerDevice::None ||
			m_deviceTypeMode == qsamplerDevice::Audio) {
			m_pAudioItems = new qsamplerDeviceItem(ui.DeviceListView,
				qsamplerDevice::Audio);
		}
		if (m_pAudioItems) {
			piDeviceIDs = qsamplerDevice::getDevices(pMainForm->client(),
				qsamplerDevice::Audio);
			for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
				new qsamplerDeviceItem(m_pAudioItems,
					qsamplerDevice::Audio, piDeviceIDs[i]);
			}
			m_pAudioItems->setExpanded(true);
		}
		// Grab and pop MIDI devices...
		if (m_deviceTypeMode == qsamplerDevice::None ||
			m_deviceTypeMode == qsamplerDevice::Midi) {
			m_pMidiItems = new qsamplerDeviceItem(ui.DeviceListView,
				qsamplerDevice::Midi);
		}
		if (m_pMidiItems) {
			piDeviceIDs = qsamplerDevice::getDevices(pMainForm->client(),
				qsamplerDevice::Midi);
			for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
				new qsamplerDeviceItem(m_pMidiItems,
					qsamplerDevice::Midi, piDeviceIDs[i]);
			}
			m_pMidiItems->setExpanded(true);
		}
	}

	// Done.
	m_iDirtySetup--;

	// Show something.
	selectDevice();
}


// Driver selection slot.
void DeviceForm::selectDriver ( const QString& sDriverName )
{
	if (m_iDirtySetup > 0)
		return;

	//
	//  Driver name has changed for a new device...
	//

	QTreeWidgetItem* pItem = ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	// Driver change is only valid for scratch devices...
	if (m_bNewDevice) {
		m_iDirtySetup++;
		device.setDriver(sDriverName);
		deviceParamModel.refresh(device.params(), m_bNewDevice);
		m_iDirtySetup--;
		// Done.
		stabilizeForm();
	}
}


// Device selection slot.
void DeviceForm::selectDevice (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	if (m_iDirtySetup > 0)
		return;

	//
	//  Device selection has changed...
	//

	QTreeWidgetItem* pItem = ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM) {
		m_deviceType = qsamplerDevice::None;
		ui.DeviceNameTextLabel->setText(QString::null);
		deviceParamModel.clear();
		ui.DevicePortComboBox->clear();
		devicePortParamModel.clear();
		ui.DevicePortTextLabel->setEnabled(false);
		ui.DevicePortComboBox->setEnabled(false);
		ui.DevicePortParamTable->setEnabled(false);
		stabilizeForm();
		return;
	}

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	m_iDirtySetup++;
	// Flag whether this is a new device.
	m_bNewDevice = (device.deviceID() < 0);

	// Fill the device/driver heading...
	ui.DeviceNameTextLabel->setText(device.deviceName());
	// The driver combobox is only rebuilt if device type has changed...
	if (device.deviceType() != m_deviceType) {
		ui.DriverNameComboBox->clear();
		ui.DriverNameComboBox->insertStringList(
			qsamplerDevice::getDrivers(pMainForm->client(), device.deviceType()));
		m_deviceType = device.deviceType();
	}
	// Do we need a driver name?
	if (m_bNewDevice || device.driverName().isEmpty())
		device.setDriver(ui.DriverNameComboBox->currentText());
	setDriverName(device.driverName());
	ui.DriverNameTextLabel->setEnabled(m_bNewDevice);
	ui.DriverNameComboBox->setEnabled(m_bNewDevice);
	// Fill the device parameter table...
	deviceParamModel.refresh(device.params(), m_bNewDevice);
	// And now the device port/channel parameter table...
	switch (device.deviceType()) {
	case qsamplerDevice::Audio:
		ui.DevicePortTextLabel->setText(tr("Ch&annel:"));
		break;
	case qsamplerDevice::Midi:
		ui.DevicePortTextLabel->setText(tr("P&ort:"));
		break;
	case qsamplerDevice::None:
		break;
	}
	ui.DevicePortComboBox->clear();
	devicePortParamModel.clear();
	if (m_bNewDevice) {
		ui.DevicePortTextLabel->setEnabled(false);
		ui.DevicePortComboBox->setEnabled(false);
		ui.DevicePortParamTable->setEnabled(false);
	} else {
		QPixmap pixmap;
		switch (device.deviceType()) {
		case qsamplerDevice::Audio:
			pixmap = QPixmap(":/qsampler/pixmaps/audio2.png");
			break;
		case qsamplerDevice::Midi:
			pixmap = QPixmap(":/qsampler/pixmaps/midi2.png");
			break;
		case qsamplerDevice::None:
			break;
		}
		qsamplerDevicePortList& ports = device.ports();
		qsamplerDevicePort *pPort;
		for (pPort = ports.first(); pPort; pPort = ports.next()) {
			ui.DevicePortComboBox->insertItem(pixmap, device.deviceTypeName()
				+ ' ' + device.driverName()
				+ ' ' + pPort->portName());
		}
		bool bEnabled = (ports.count() > 0);
		ui.DevicePortTextLabel->setEnabled(bEnabled);
		ui.DevicePortComboBox->setEnabled(bEnabled);
		ui.DevicePortParamTable->setEnabled(bEnabled);
	}
	// Done.
	m_iDirtySetup--;

	// Make the device port/channel selection effective.
	selectDevicePort(ui.DevicePortComboBox->currentItem());
}


// Device port/channel selection slot.
void DeviceForm::selectDevicePort ( int iPort )
{
	if (m_iDirtySetup > 0)
		return;

	//
	//  Device port/channel selection has changed...
	//

	QTreeWidgetItem* pItem = ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();
	qsamplerDevicePort *pPort = device.ports().at(iPort);
	if (pPort) {
		m_iDirtySetup++;
		devicePortParamModel.refresh(pPort->params(), false);
		m_iDirtySetup--;
	}
	// Done.
	stabilizeForm();
}


// Device parameter value change slot.
void DeviceForm::changeDeviceParam ( int iRow, int iCol )
{
	if (m_iDirtySetup > 0)
		return;
	if (iRow < 0 || iCol < 0)
		return;

	//
	//  Device parameter change...
	//

	QTreeWidgetItem* pItem = ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	// Table 1st column has the parameter name;
	//const QString sParam = ui.DeviceParamTable->text(iRow, 0);
	//const QString sValue = ui.DeviceParamTable->text(iRow, iCol);
	const QString sParam = deviceParamModel.data(deviceParamModel.index(iRow, 0), Qt::DisplayRole).value<DeviceParameterRow>().name;
	const QString sValue = deviceParamModel.data(deviceParamModel.index(iRow, iCol), Qt::DisplayRole).value<DeviceParameterRow>().param.value;
	// Set the local device parameter value.
	if (device.setParam(sParam, sValue)) {
		selectDevice();
	} else {
		stabilizeForm();
	}

	// Main session should be dirtier...
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->sessionDirty();
}


// Device port/channel parameter value change slot.
void DeviceForm::changeDevicePortParam ( int iRow, int iCol )
{
	if (m_iDirtySetup > 0)
		return;
	if (iRow < 0 || iCol < 0)
		return;

	//
	//  Device port/channel parameter change...
	//

	QTreeWidgetItem* pItem = ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	int iPort = ui.DevicePortComboBox->currentItem();
	qsamplerDevicePort *pPort = device.ports().at(iPort);
	if (pPort == NULL)
		return;

	// Table 1st column has the parameter name;
	//const QString sParam = ui.DevicePortParamTable->text(iRow, 0);
	//const QString sValue = ui.DevicePortParamTable->text(iRow, iCol);
	const QString sParam = devicePortParamModel.data(devicePortParamModel.index(iRow, 0), Qt::DisplayRole).value<DeviceParameterRow>().name;
	const QString sValue = devicePortParamModel.data(devicePortParamModel.index(iRow, iCol), Qt::DisplayRole).value<DeviceParameterRow>().param.value;

	// Set the local device port/channel parameter value.
	pPort->setParam(sParam, sValue);
	// Done.
	stabilizeForm();

	// Main session should be dirtier...
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->sessionDirty();
}


// Device list view context menu handler.
void DeviceForm::contextMenu ( QTreeWidgetItem* pItem, const QPoint& pos, int )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	int iItemID;

	// Build the device context menu...
	QMenu* pContextMenu = new QMenu(this);

	bool bClient = (pMainForm->client() != NULL);
	bool bEnabled = (pItem != NULL);
	iItemID = pContextMenu->insertItem(
		QIconSet(QPixmap(":/qsampler/pixmaps/deviceCreate.png")),
		tr("&Create device"), this, SLOT(createDevice()));
	pContextMenu->setItemEnabled(iItemID, bEnabled || (bClient && m_bNewDevice));
	iItemID = pContextMenu->insertItem(
		QIconSet(QPixmap(":/qsampler/pixmaps/deviceDelete.png")),
		tr("&Delete device"), this, SLOT(deleteDevice()));
	pContextMenu->setItemEnabled(iItemID, bEnabled && !m_bNewDevice);
	pContextMenu->insertSeparator();
	iItemID = pContextMenu->insertItem(
		QIconSet(QPixmap(":/qsampler/pixmaps/formRefresh.png")),
		tr("&Refresh"), this, SLOT(refreshDevices()));
	pContextMenu->setItemEnabled(iItemID, bClient);

	pContextMenu->exec(pos);

	delete pContextMenu;
}


// Stabilize current form state.
void DeviceForm::stabilizeForm (void)
{
	MainForm* pMainForm = MainForm::getInstance();
	QTreeWidgetItem* pItem = ui.DeviceListView->currentItem();
	bool bClient = (pMainForm && pMainForm->client() != NULL);
	bool bEnabled = (pItem != NULL);
	ui.DeviceNameTextLabel->setEnabled(bEnabled && !m_bNewDevice);
	ui.DriverNameTextLabel->setEnabled(bEnabled &&  m_bNewDevice);
	ui.DriverNameComboBox->setEnabled(bEnabled && m_bNewDevice);
	ui.DeviceParamTable->setEnabled(bEnabled);
	ui.RefreshDevicesPushButton->setEnabled(bClient);
	ui.CreateDevicePushButton->setEnabled(bEnabled || (bClient && m_bNewDevice));
	ui.DeleteDevicePushButton->setEnabled(bEnabled && !m_bNewDevice);
}

} // namespace QSampler


// end of qsamplerDeviceForm.cpp
