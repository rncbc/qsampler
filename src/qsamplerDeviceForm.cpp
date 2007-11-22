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
#include "qsamplerMainForm.h"

#include <QHeaderView>
#include <QMessageBox>


namespace QSampler {

DeviceForm::DeviceForm ( QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags)
{
	m_ui.setupUi(this);

	// Initialize locals.
	m_iDirtySetup = 0;
	m_iDirtyCount = 0;
	m_bNewDevice  = false;
	m_deviceType  = qsamplerDevice::None;
	m_pAudioItems = NULL;
	m_pMidiItems  = NULL;
	// No exclusive mode as default.
	m_deviceTypeMode = qsamplerDevice::None;

	m_ui.DeviceListView->header()->hide();

	m_ui.DeviceParamTable->setModel(&m_deviceParamModel);
	m_ui.DeviceParamTable->setItemDelegate(&m_deviceParamDelegate);
	m_ui.DeviceParamTable->horizontalHeader()->setResizeMode(2, QHeaderView::Stretch);
	m_ui.DeviceParamTable->verticalHeader()->hide();

	m_ui.DevicePortParamTable->setModel(&m_devicePortParamModel);
	m_ui.DevicePortParamTable->setItemDelegate(&m_devicePortParamDelegate);
	m_ui.DevicePortParamTable->horizontalHeader()->setResizeMode(2, QHeaderView::Stretch);
	m_ui.DevicePortParamTable->verticalHeader()->hide();

	// Initial contents.
	refreshDevices();
	// Try to restore normal window positioning.
	adjustSize();

	QObject::connect(m_ui.DeviceListView,
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(selectDevice()));
	QObject::connect(m_ui.DeviceListView,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(deviceListViewContextMenu(const QPoint&)));
	QObject::connect(m_ui.RefreshDevicesPushButton,
		SIGNAL(clicked()),
		SLOT(refreshDevices()));
	QObject::connect(m_ui.DriverNameComboBox,
		SIGNAL(activated(const QString&)),
		SLOT(selectDriver(const QString&)));
	QObject::connect(m_ui.DevicePortComboBox,
		SIGNAL(activated(int)),
		SLOT(selectDevicePort(int)));
	QObject::connect(m_ui.CreateDevicePushButton,
		SIGNAL(clicked()),
		SLOT(createDevice()));
	QObject::connect(m_ui.DeleteDevicePushButton,
		SIGNAL(clicked()),
		SLOT(deleteDevice()));
	QObject::connect(m_ui.ClosePushButton,
		SIGNAL(clicked()),
		SLOT(close()));
	QObject::connect(&m_deviceParamModel,
		SIGNAL(modelReset()),
		SLOT(updateCellRenderers()));
	QObject::connect(&m_deviceParamModel,
		SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
		SLOT(updateCellRenderers(const QModelIndex&, const QModelIndex&)));
	QObject::connect(&m_devicePortParamModel,
		SIGNAL(modelReset()),
		SLOT(updatePortCellRenderers()));
	QObject::connect(&m_devicePortParamModel,
		SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
		SLOT(updatePortCellRenderers(const QModelIndex&, const QModelIndex&)));
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
	qsamplerDevice::DeviceType deviceTypeMode )
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
	if (m_ui.DriverNameComboBox->findText(sDriverName) < 0)
		m_ui.DriverNameComboBox->insertItem(0, sDriverName);
	m_ui.DriverNameComboBox->setItemText(
		m_ui.DriverNameComboBox->currentIndex(),
		sDriverName);
}


// Set current selected device by type and id.
void DeviceForm::setDevice ( qsamplerDevice *pDevice )
{
	// In case no device is given...
	qsamplerDevice::DeviceType deviceType = m_deviceTypeMode;
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
		m_ui.DeviceListView->setCurrentItem(pRootItem);
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

	QTreeWidgetItem *pItem = m_ui.DeviceListView->currentItem();
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

	QTreeWidgetItem* pItem = m_ui.DeviceListView->currentItem();
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
	m_ui.DeviceListView->clear();
	if (pMainForm->client()) {
		int *piDeviceIDs;
		// Grab and pop Audio devices...
		if (m_deviceTypeMode == qsamplerDevice::None ||
			m_deviceTypeMode == qsamplerDevice::Audio) {
			m_pAudioItems = new qsamplerDeviceItem(m_ui.DeviceListView,
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
			m_pMidiItems = new qsamplerDeviceItem(m_ui.DeviceListView,
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

	QTreeWidgetItem* pItem = m_ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	// Driver change is only valid for scratch devices...
	if (m_bNewDevice) {
		m_iDirtySetup++;
		device.setDriver(sDriverName);
		m_deviceParamModel.refresh(&device, m_bNewDevice);
		m_iDirtySetup--;
		// Done.
		stabilizeForm();
	}
}


// Device selection slot.
void DeviceForm::selectDevice ()
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	if (m_iDirtySetup > 0)
		return;

	//
	//  Device selection has changed...
	//

	QTreeWidgetItem* pItem = m_ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM) {
		m_deviceType = qsamplerDevice::None;
		m_ui.DeviceNameTextLabel->setText(QString::null);
		m_deviceParamModel.clear();
		m_ui.DevicePortComboBox->clear();
		m_devicePortParamModel.clear();
		m_ui.DevicePortTextLabel->setEnabled(false);
		m_ui.DevicePortComboBox->setEnabled(false);
		m_ui.DevicePortParamTable->setEnabled(false);
		stabilizeForm();
		return;
	}

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	m_iDirtySetup++;
	// Flag whether this is a new device.
	m_bNewDevice = (device.deviceID() < 0);

	// Fill the device/driver heading...
	m_ui.DeviceNameTextLabel->setText(device.deviceName());
	// The driver combobox is only rebuilt if device type has changed...
	if (device.deviceType() != m_deviceType) {
		m_ui.DriverNameComboBox->clear();
		m_ui.DriverNameComboBox->insertItems(0,
			qsamplerDevice::getDrivers(pMainForm->client(), device.deviceType()));
		m_deviceType = device.deviceType();
	}
	// Do we need a driver name?
	if (m_bNewDevice || device.driverName().isEmpty())
		device.setDriver(m_ui.DriverNameComboBox->currentText());
	setDriverName(device.driverName());
	m_ui.DriverNameTextLabel->setEnabled(m_bNewDevice);
	m_ui.DriverNameComboBox->setEnabled(m_bNewDevice);
	// Fill the device parameter table...
	m_deviceParamModel.refresh(&device, m_bNewDevice);
	// And now the device port/channel parameter table...
	switch (device.deviceType()) {
	case qsamplerDevice::Audio:
		m_ui.DevicePortTextLabel->setText(tr("Ch&annel:"));
		break;
	case qsamplerDevice::Midi:
		m_ui.DevicePortTextLabel->setText(tr("P&ort:"));
		break;
	case qsamplerDevice::None:
		break;
	}
	m_ui.DevicePortComboBox->clear();
	m_devicePortParamModel.clear();
	if (m_bNewDevice) {
		m_ui.DevicePortTextLabel->setEnabled(false);
		m_ui.DevicePortComboBox->setEnabled(false);
		m_ui.DevicePortParamTable->setEnabled(false);
	} else {
		QPixmap pixmap;
		switch (device.deviceType()) {
		case qsamplerDevice::Audio:
			pixmap = QPixmap(":/icons/audio2.png");
			break;
		case qsamplerDevice::Midi:
			pixmap = QPixmap(":/icons/midi2.png");
			break;
		case qsamplerDevice::None:
			break;
		}
		qsamplerDevicePortList& ports = device.ports();
		QListIterator<qsamplerDevicePort *> iter(ports);
		while (iter.hasNext()) {
			qsamplerDevicePort *pPort = iter.next();
			m_ui.DevicePortComboBox->addItem(pixmap,
				device.deviceTypeName()
				+ ' ' + device.driverName()
				+ ' ' + pPort->portName());
		}
		bool bEnabled = (ports.count() > 0);
		m_ui.DevicePortTextLabel->setEnabled(bEnabled);
		m_ui.DevicePortComboBox->setEnabled(bEnabled);
		m_ui.DevicePortParamTable->setEnabled(bEnabled);
	}
	// Done.
	m_iDirtySetup--;

	// Make the device port/channel selection effective.
	selectDevicePort(m_ui.DevicePortComboBox->currentIndex());
}


// Device port/channel selection slot.
void DeviceForm::selectDevicePort ( int iPort )
{
	if (m_iDirtySetup > 0)
		return;

	//
	//  Device port/channel selection has changed...
	//

	QTreeWidgetItem* pItem = m_ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();
	qsamplerDevicePort *pPort = NULL;
	if (iPort >= 0 && iPort < device.ports().count())
		pPort = device.ports().at(iPort);
	if (pPort) {
		m_iDirtySetup++;
		m_devicePortParamModel.refresh(pPort, false);
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

/* we do that in the model class now ...
	QTreeWidgetItem* pItem = m_ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	// Table 1st column has the parameter name;
	//const QString sParam = m_ui.DeviceParamTable->text(iRow, 0);
	//const QString sValue = m_ui.DeviceParamTable->text(iRow, iCol);
	const QString sParam = m_deviceParamModel.data(m_deviceParamModel.index(iRow, 0), Qt::DisplayRole).value<DeviceParameterRow>().name;
	const QString sValue = m_deviceParamModel.data(m_deviceParamModel.index(iRow, iCol), Qt::DisplayRole).value<DeviceParameterRow>().param.value;
	// Set the local device parameter value.
	if (device.setParam(sParam, sValue)) {
		selectDevice();
	} else {
		stabilizeForm();
	}
*/

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

/* we do that in the model class now ...
	QTreeWidgetItem* pItem = m_ui.DeviceListView->currentItem();
	if (pItem == NULL || pItem->type() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	int iPort = m_ui.DevicePortComboBox->currentIndex();
	qsamplerDevicePort *pPort = NULL;
	if (iPort >= 0 && iPort < device.ports().count())
		pPort = device.ports().at(iPort);
	if (pPort == NULL)
		return;

	// Table 1st column has the parameter name;
	//const QString sParam = m_ui.DevicePortParamTable->text(iRow, 0);
	//const QString sValue = m_ui.DevicePortParamTable->text(iRow, iCol);
	const QString sParam = m_devicePortParamModel.data(m_devicePortParamModel.index(iRow, 0), Qt::DisplayRole).value<DeviceParameterRow>().name;
	const QString sValue = m_devicePortParamModel.data(m_devicePortParamModel.index(iRow, iCol), Qt::DisplayRole).value<DeviceParameterRow>().param.value;

	// Set the local device port/channel parameter value.
	pPort->setParam(sParam, sValue);
*/

	// Done.
	stabilizeForm();

	// Main session should be dirtier...
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->sessionDirty();
}


// Device list view context menu handler.
void DeviceForm::deviceListViewContextMenu ( const QPoint& pos )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	QTreeWidgetItem* pItem = m_ui.DeviceListView->itemAt(pos);
	if (pItem == NULL)
		return;

	// Build the device context menu...
	QMenu menu(this);
	QAction *pAction;

	bool bClient = (pMainForm->client() != NULL);
	bool bEnabled = (pItem != NULL);
	pAction = menu.addAction(
		QIcon(":/qsampler/pixmaps/deviceCreate.png"),
		tr("&Create device"), this, SLOT(createDevice()));
	pAction->setEnabled(bEnabled || (bClient && m_bNewDevice));
	pAction = menu.addAction(
		QIcon(":/qsampler/pixmaps/deviceDelete.png"),
		tr("&Delete device"), this, SLOT(deleteDevice()));
	pAction->setEnabled(bEnabled && !m_bNewDevice);
	menu.addSeparator();
	pAction = menu.addAction(
		QIcon(":/qsampler/pixmaps/formRefresh.png"),
		tr("&Refresh"), this, SLOT(refreshDevices()));
	pAction->setEnabled(bClient);

	menu.exec(pos);
}


// Stabilize current form state.
void DeviceForm::stabilizeForm (void)
{
	MainForm* pMainForm = MainForm::getInstance();
	QTreeWidgetItem* pItem = m_ui.DeviceListView->currentItem();
	bool bClient = (pMainForm && pMainForm->client() != NULL);
	bool bEnabled = (pItem != NULL);
	m_ui.DeviceNameTextLabel->setEnabled(bEnabled && !m_bNewDevice);
	m_ui.DriverNameTextLabel->setEnabled(bEnabled &&  m_bNewDevice);
	m_ui.DriverNameComboBox->setEnabled(bEnabled && m_bNewDevice);
	m_ui.DeviceParamTable->setEnabled(bEnabled);
	m_ui.RefreshDevicesPushButton->setEnabled(bClient);
	m_ui.CreateDevicePushButton->setEnabled(bEnabled || (bClient && m_bNewDevice));
	m_ui.DeleteDevicePushButton->setEnabled(bEnabled && !m_bNewDevice);
}


void DeviceForm::updateCellRenderers (void)
{
	const int rows = m_deviceParamModel.rowCount();
	const int cols = m_deviceParamModel.columnCount();
	updateCellRenderers(
		m_deviceParamModel.index(0, 0),
		m_deviceParamModel.index(rows - 1, cols - 1));
}


void DeviceForm::updateCellRenderers (
	const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
	for (int r = topLeft.row(); r <= bottomRight.row(); r++) {
		for (int c = topLeft.column(); c <= bottomRight.column(); c++) {
			const QModelIndex index = m_deviceParamModel.index(r, c);
			m_ui.DeviceParamTable->openPersistentEditor(index);
		}
	}
}


void DeviceForm::updatePortCellRenderers (void)
{
	const int rows = m_devicePortParamModel.rowCount();
	const int cols = m_devicePortParamModel.columnCount();
	updatePortCellRenderers(
		m_devicePortParamModel.index(0, 0),
		m_devicePortParamModel.index(rows - 1, cols - 1));
}


void DeviceForm::updatePortCellRenderers ( 
	const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
	for (int r = topLeft.row(); r <= bottomRight.row(); r++) {
		for (int c = topLeft.column(); c <= bottomRight.column(); c++) {
			const QModelIndex index = m_devicePortParamModel.index(r, c);
			m_ui.DevicePortParamTable->openPersistentEditor(index);
		}
	}
}

} // namespace QSampler


// end of qsamplerDeviceForm.cpp
