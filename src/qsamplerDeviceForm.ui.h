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

#include "qsamplerMainForm.h"

#include "config.h"


// Kind of constructor.
void qsamplerDeviceForm::init (void)
{
	// Initialize locals.
	m_pMainForm   = (qsamplerMainForm *) QWidget::parentWidget();
	m_pClient     = NULL;
	m_iDirtySetup = 0;
	m_iDirtyCount = 0;
	m_iUntitled   = 1;
	m_bNewDevice  = false;

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


// Format the displayable device configuration filename.
QString qsamplerDeviceForm::devicesName ( const QString& sFilename )
{
	QString sDevicesName = sFilename;
	qsamplerOptions *pOptions = m_pMainForm->options();
	if (pOptions) {
		bool bCompletePath = (pOptions && pOptions->bCompletePath);
		if (sDevicesName.isEmpty())
			sDevicesName = tr("Untitled") + QString::number(m_iUntitled);
		else if (!bCompletePath)
			sDevicesName = QFileInfo(sDevicesName).fileName();
	}
	return sDevicesName;
}


// Window close event handlers.
bool qsamplerDeviceForm::queryClose (void)
{
	bool bQueryClose = true;

	if (m_iDirtyCount > 0) {
		switch (QMessageBox::warning(this, tr("Warning"),
			tr("The device configuration has been changed.\n\n"
			"\"%1\"\n\n"
			"Do you want to save the changes?")
			.arg(devicesName(m_sFilename)),
			tr("Save"), tr("Discard"), tr("Cancel"))) {
		case 0:     // Save...
			saveDevices();
			// Fall thru....
		case 1:     // Discard
			break;
		default:    // Cancel.
			bQueryClose = false;
		}
	}

	return bQueryClose;
}



// Dirty up settings.
void qsamplerDeviceForm::contentsChanged (void)
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtyCount++;
	stabilizeForm();
}


// Load device configuration slot.
void qsamplerDeviceForm::loadDevices (void)
{
	QString sFilename = QFileDialog::getOpenFileName(
			m_sFilename,                                    // Start here.
			tr("Device Configuration files") + " (*.lscp)", // Filter (XML files)
			this, 0,                                        // Parent and name (none)
			tr("Load Device Configuration")                 // Caption.
	);

	if (sFilename.isEmpty())
		return;

	// Check if we're going to discard safely the current one...
	if (!queryClose())
		return;

	// Load it right away...
	loadDevicesFile(sFilename);
}


// Save device configuration slot.
void qsamplerDeviceForm::saveDevices (void)
{
	QString sFilename = QFileDialog::getSaveFileName(
			m_sFilename,                                    // Start here.
			tr("Device Configuration files") + " (*.lscp)", // Filter (XML files)
			this, 0,                                        // Parent and name (none)
			tr("Save Device Configuration")                 // Caption.
	);

	if (sFilename.isEmpty())
		return;

	// Enforce .xml extension...
	if (QFileInfo(sFilename).extension().isEmpty())
		sFilename += ".lscp";

	// Save it right away...
	saveDevicesFile(sFilename);
}


// Load device configuration from file.
void qsamplerDeviceForm::loadDevicesFile ( const QString& sFilename )
{
	//
	// TODO: Load device configuration from file...
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::loadDevicesFile(\"" + sFilename + "\")...");

	m_sFilename   = sFilename;
	m_iDirtyCount = 0;

	refreshDevices();
}


// Save device configuration into file.
void qsamplerDeviceForm::saveDevicesFile ( const QString& sFilename )
{
	//
	// TODO: Save device configuration into file...
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::saveDevicesFile(\"" + sFilename + "\")...");

	m_sFilename   = sFilename;
	m_iDirtyCount = 0;
	stabilizeForm();
}


// Create a new device from current table view.
void qsamplerDeviceForm::createDevice (void)
{
	//
	// TODO: Create a new device from current table view...
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::createDevice()...");
	
	m_iDirtyCount++;
	stabilizeForm();
}


// Update current device in table view.
void qsamplerDeviceForm::updateDevice (void)
{
	//
	// TODO: Update current device in table view...
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::updateDevice()...");

	m_iDirtyCount++;
	stabilizeForm();
}


// Delete current device in table view.
void qsamplerDeviceForm::deleteDevice (void)
{
	//
	// TODO: Delete current device in table view...
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::deleteDevice()...");

	m_iDirtyCount++;
	stabilizeForm();
}


// Refresh all device list and views.
void qsamplerDeviceForm::refreshDevices (void)
{
	// Avoid nested changes.
	m_iDirtySetup++;

	//
	// TODO: Load device configuration data ...
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::refreshDevices()");

	DeviceListView->clear();
	if (m_pClient) {
		qsamplerDeviceItem *pItem;
		int *piDeviceIDs;
		// Grab and pop Audio devices...
		pItem = new qsamplerDeviceItem(DeviceListView, m_pClient,
			qsamplerDevice::Audio);
		if (pItem) {
			pItem->setText(0, tr("Audio"));
			piDeviceIDs = qsamplerDevice::getDevices(m_pClient, qsamplerDevice::Audio);
			for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
				new qsamplerDeviceItem(pItem, m_pClient,
					qsamplerDevice::Audio, piDeviceIDs[i]);
			}
			pItem->setOpen(true);
		}
		// Grab and pop MIDI devices...
		pItem = new qsamplerDeviceItem(DeviceListView, m_pClient,
			qsamplerDevice::Midi);
		if (pItem) {
			pItem->setText(0, tr("MIDI"));
			piDeviceIDs = qsamplerDevice::getDevices(m_pClient, qsamplerDevice::Midi);
			for (int i = 0; piDeviceIDs && piDeviceIDs[i] >= 0; i++) {
				new qsamplerDeviceItem(pItem, m_pClient,
					qsamplerDevice::Midi, piDeviceIDs[i]);
			}
			pItem->setOpen(true);
		}
	}

	// Done.
	selectDevice();
	m_iDirtySetup--;
}


// Driver selection slot.
void qsamplerDeviceForm::selectDriver ( const QString& sDriverName )
{
	//
	//  TODO: Driver name has changed for a new device...
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::selectDriver(\"" + sDriverName + "\")");

	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();
	if (m_bNewDevice) {
		device.setDriver(m_pClient, sDriverName);
		m_iDirtyCount++;
	}

	// Done.
	stabilizeForm();
}


// Device selection slot.
void qsamplerDeviceForm::selectDevice (void)
{
	//
	//  TODO: Device selection has changed...
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::selectDevice()");

	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM) {
		stabilizeForm();
		return;
	}

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();
	m_bNewDevice = (device.deviceID() < 0);

	// Fill the device heading...
	DeviceNameTextLabel->setText(' ' + device.deviceName());
	DriverNameComboBox->clear();
	DriverNameComboBox->insertStringList(
		qsamplerDevice::getDrivers(m_pClient, device.deviceType()));
	const QString& sDriverName = device.driverName();
	if (m_bNewDevice || sDriverName.isEmpty()) {
		device.setDriver(m_pClient, DriverNameComboBox->currentText());
	} else {
		if (DriverNameComboBox->listBox()->findItem(sDriverName, Qt::ExactMatch) == NULL)
			DriverNameComboBox->insertItem(sDriverName);
		DriverNameComboBox->setCurrentText(sDriverName);
	}
	DriverNameTextLabel->setEnabled(m_bNewDevice);
	DriverNameComboBox->setEnabled(m_bNewDevice);
	
	// Fill the device parameter table...
	DeviceParamTable->refresh(device);

	// Done.
	stabilizeForm();
}


// parameter value change slot.
void qsamplerDeviceForm::changeValue ( int iRow, int iCol )
{
	//
	//  TODO: Device parameter change...
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::changeValue()");
	
	QListViewItem *pItem = DeviceListView->selectedItem();
	if (pItem == NULL || pItem->rtti() != QSAMPLER_DEVICE_ITEM)
		return;

	qsamplerDevice& device = ((qsamplerDeviceItem *) pItem)->device();

	// Table 3rd column has the parameter name;
	qsamplerDeviceParamMap& params = device.params();
	QString sParam = DeviceParamTable->text(iRow, 2);
	params[sParam].value = DeviceParamTable->text(iRow, iCol);

	m_iDirtyCount++;
	stabilizeForm();
}


// Stabilize current form state.
void qsamplerDeviceForm::stabilizeForm (void)
{
	// Update the main caption...
	QString sDevicesName = devicesName(m_sFilename);
	if (m_iDirtyCount > 0)
		sDevicesName += '*';
	setCaption(tr("Devices - [%1]").arg(sDevicesName));

	//
	// TODO: Enable/disable available command buttons.
	//
	m_pMainForm->appendMessages("qsamplerDeviceForm::stabilizeForm()");

	SaveDevicesPushButton->setEnabled(m_iDirtyCount > 0);

	QListViewItem *pItem = DeviceListView->selectedItem();
	bool bEnabled = (pItem != NULL);
	DeviceNameTextLabel->setEnabled(bEnabled);
	DriverNameTextLabel->setEnabled(bEnabled && m_bNewDevice);
	DriverNameComboBox->setEnabled(bEnabled && m_bNewDevice);
	DeviceParamTable->setEnabled(bEnabled);
	CreateDevicePushButton->setEnabled(bEnabled && (m_iDirtyCount > 0 ||  m_bNewDevice));
	UpdateDevicePushButton->setEnabled(bEnabled && (m_iDirtyCount > 0 && !m_bNewDevice));
	DeleteDevicePushButton->setEnabled(bEnabled && (m_iDirtyCount > 0 && !m_bNewDevice));
}


// end of qsamplerDeviceForm.ui.h

