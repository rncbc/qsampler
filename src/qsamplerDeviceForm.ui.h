// qsamplerDeviceForm.ui.h
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

#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>

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
	
	// Set our main client reference.
    DeviceParameterTable->setClient(m_pClient);

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
}


// Update current device in table view.
void qsamplerDeviceForm::updateDevice (void)
{
	//
    // TODO: Update current device in table view...
    //
	m_pMainForm->appendMessages("qsamplerDeviceForm::updateDevice()...");
}


// Delete current device in table view.
void qsamplerDeviceForm::deleteDevice (void)
{
	//
    // TODO: Delete current device in table view...
    //
	m_pMainForm->appendMessages("qsamplerDeviceForm::deleteDevice()...");
}


// Refresh all device list and views.
void qsamplerDeviceForm::refreshDevices (void)
{
    // Avoid nested changes.
    m_iDirtySetup++;

    DeviceListView->clear();
    if (m_pClient)
    	new QListViewItem(DeviceListView, tr("<New device>"));

	//
    // TODO: Load device configuration data ...
    //
	m_pMainForm->appendMessages("qsamplerDeviceForm::refreshDevices()");

	DeviceParameterTable->setNumRows(0);
	if (m_pClient) {
		DeviceParameterTable->insertRows(0, 3);
		for (int c = 0; c < DeviceParameterTable->numCols(); c++) {
			for (int r = 0; r < DeviceParameterTable->numRows(); r++)
				DeviceParameterTable->setText(r, c, QString("R%1C%1").arg(r).arg(c));
			DeviceParameterTable->adjustColumn(c);
		}
	}

    // Done.
    m_iDirtySetup--;
//  stabilizeForm();
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
    
    CreateDevicePushButton->setEnabled(m_iDirtyCount > 0);
    UpdateDevicePushButton->setEnabled(m_iDirtyCount > 0);
    DeleteDevicePushButton->setEnabled(m_iDirtyCount > 0);
}


// end of qsamplerDeviceForm.ui.h

