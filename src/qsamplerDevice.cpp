// qsamplerDevice.cpp
//
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

#include "qsamplerDevice.h"

#include "qsamplerMainForm.h"
#include "qsamplerDeviceForm.h"

#include "config.h"


//-------------------------------------------------------------------------
// qsamplerDeviceParam - MIDI/Audio Device parameter structure.
//

// Constructors.
qsamplerDeviceParam::qsamplerDeviceParam ( lscp_param_info_t *pParamInfo,
	const char *pszValue )
{
	setParam(pParamInfo, pszValue);
}


// Default destructor.
qsamplerDeviceParam::~qsamplerDeviceParam (void)
{
}


// Initializer.
void qsamplerDeviceParam::setParam ( lscp_param_info_t *pParamInfo,
	const char *pszValue )
{
	if (pParamInfo == NULL)
		return;
		
	// Info structure field members.
	
	type = pParamInfo->type;
	
	if (pParamInfo->description)
		description = pParamInfo->description;
	else
		description = QString::null;
	
	mandatory = (bool) pParamInfo->multiplicity;
	fix = (bool) pParamInfo->fix;
	multiplicity = (bool) pParamInfo->multiplicity;
	
	depends.clear();
	for (int i = 0; pParamInfo->depends && pParamInfo->depends[i]; i++)
		depends.append(pParamInfo->depends[i]);
	
	if (pParamInfo->defaultv)
		defaultv = pParamInfo->defaultv;
	else
		defaultv = QString::null;
	
	if (pParamInfo->range_min)
		range_min = pParamInfo->range_min;
	else
		range_min = QString::null;
	
	if (pParamInfo->range_max)
		range_max = pParamInfo->range_max;
	else
		range_max = QString::null;
	
	possibilities.clear();
	for (int i = 0; pParamInfo->possibilities && pParamInfo->possibilities[i]; i++)
		possibilities.append(pParamInfo->possibilities[i]);
		
	// The current parameter value.
	if (pszValue)
		value = pszValue;
	else
		value = QString::null;
}


//-------------------------------------------------------------------------
// qsamplerDevice - MIDI/Audio Device structure.
//

// Constructor.
qsamplerDevice::qsamplerDevice ( lscp_client_t *pClient,
	qsamplerDeviceType deviceType, int iDeviceID )
{
	setDevice(pClient, deviceType, iDeviceID);
}

// Default destructor.
qsamplerDevice::~qsamplerDevice (void)
{
}


// Initializer.
void qsamplerDevice::setDevice ( lscp_client_t *pClient,
	qsamplerDeviceType deviceType, int iDeviceID )
{
	// Device id and type should be always set.
	m_iDeviceID   = iDeviceID;
	m_deviceType  = deviceType;

	// Retrieve device info, if any.
	QString sDeviceType;
	lscp_device_info_t *pDeviceInfo = NULL;
	switch (deviceType) {
	case qsamplerDevice::Audio:
		sDeviceType = QObject::tr("Audio");
		pDeviceInfo = ::lscp_get_audio_device_info(pClient, iDeviceID);
		break;
	case qsamplerDevice::Midi:
		sDeviceType = QObject::tr("MIDI");
		pDeviceInfo = ::lscp_get_midi_device_info(pClient, iDeviceID);
		break;
	}

	// If we're bogus, bail out...
	if (pDeviceInfo == NULL) {
		m_sDriverName = QString::null;
		m_sDeviceName = QObject::tr("New %1 device").arg(sDeviceType);
		return;
	}

	// Other device properties...
	m_sDriverName = pDeviceInfo->driver;
	m_sDeviceName = m_sDriverName + ' '
		+ QObject::tr("Device %1").arg(m_iDeviceID);

	// Grab device parameters...
	m_params.clear();
	for (int i = 0; pDeviceInfo->params && pDeviceInfo->params[i].key; i++) {
		const char *pszParam = pDeviceInfo->params[i].key;
		lscp_param_info_t *pParamInfo = NULL;
		switch (deviceType) {
		case qsamplerDevice::Audio:
			pParamInfo = ::lscp_get_audio_driver_param_info(pClient,
				m_sDriverName.latin1(), pszParam, NULL);
			break;
		case qsamplerDevice::Midi:
			pParamInfo = ::lscp_get_midi_driver_param_info(pClient,
				m_sDriverName.latin1(), pszParam, NULL);
			break;
		}
		m_params[pszParam] = qsamplerDeviceParam(pParamInfo, pDeviceInfo->params[i].value);
	}
}


// Driver name initializer.
void qsamplerDevice::setDriver ( lscp_client_t *pClient,
	const QString& sDriverName )
{
	// Valid only for scratch devices.
	if (m_sDriverName == sDriverName)
		return;

	// Retrieve driver info, if any.
	lscp_driver_info_t *pDriverInfo = NULL;
	switch (m_deviceType) {
	case qsamplerDevice::Audio:
		pDriverInfo = ::lscp_get_audio_driver_info(pClient,
			sDriverName.latin1());
		break;
	case qsamplerDevice::Midi:
		pDriverInfo = ::lscp_get_midi_driver_info(pClient,
			sDriverName.latin1());
		break;
	}

	// If we're bogus, bail out...
	if (pDriverInfo == NULL)
		return;

	// Remember device parameters...
	m_sDriverName = sDriverName;

	// Grab driver parameters...
	m_params.clear();
	for (int i = 0; pDriverInfo->parameters && pDriverInfo->parameters[i]; i++) {
		const char *pszParam = pDriverInfo->parameters[i];
		lscp_param_info_t *pParamInfo = NULL;
		switch (m_deviceType) {
		case qsamplerDevice::Audio:
			pParamInfo = ::lscp_get_audio_driver_param_info(pClient,
				sDriverName.latin1(), pszParam, NULL);
			break;
		case qsamplerDevice::Midi:
			pParamInfo = ::lscp_get_midi_driver_param_info(pClient,
				sDriverName.latin1(), pszParam, NULL);
			break;
		}
		m_params[pszParam] = qsamplerDeviceParam(pParamInfo, pParamInfo->defaultv);
	}
}


// Device property accessors.
int qsamplerDevice::deviceID (void) const
{
	return m_iDeviceID;
}

qsamplerDevice::qsamplerDeviceType qsamplerDevice::deviceType (void) const
{
	return m_deviceType;
}

const QString& qsamplerDevice::driverName (void) const
{
	return m_sDriverName;
}

const QString& qsamplerDevice::deviceName (void) const
{
	return m_sDeviceName;
}

// Device parameter accessor.
qsamplerDeviceParamMap& qsamplerDevice::params (void)
{
	return m_params;
}


// Update/refresh device/driver data.
void qsamplerDevice::refresh (void)
{
}

// Device ids enumerator.
int *qsamplerDevice::getDevices ( lscp_client_t *pClient,
	qsamplerDeviceType deviceType )
{
	int *piDeviceIDs = NULL;
	switch (deviceType) {
	case qsamplerDevice::Audio:
		piDeviceIDs = ::lscp_list_audio_devices(pClient);
		break;
	case qsamplerDevice::Midi:
		piDeviceIDs = ::lscp_list_midi_devices(pClient);
		break;
	}
	return piDeviceIDs;
}


// Driver names enumerator.
QStringList qsamplerDevice::getDrivers ( lscp_client_t *pClient,
	qsamplerDeviceType deviceType )
{
	QStringList drivers;
	
	const char **ppszDrivers = NULL;
	switch (deviceType) {
	case qsamplerDevice::Audio:
		ppszDrivers = ::lscp_get_available_audio_drivers(pClient);
		break;
	case qsamplerDevice::Midi:
		ppszDrivers = ::lscp_get_available_midi_drivers(pClient);
		break;
	}
	
	for (int iDriver = 0; ppszDrivers[iDriver]; iDriver++)
		drivers.append(ppszDrivers[iDriver]);

	return drivers;
}


//-------------------------------------------------------------------------
// qsamplerDeviceItem - QListView device item.
//

// Constructors.
qsamplerDeviceItem::qsamplerDeviceItem ( QListView *pListView, lscp_client_t *pClient,
	qsamplerDevice::qsamplerDeviceType deviceType )
	: QListViewItem(pListView), m_device(pClient, deviceType)
{
	switch(m_device.deviceType()) {
	case qsamplerDevice::Audio:
		QListViewItem::setPixmap(0, QPixmap::fromMimeSource("audio1.png"));
		QListViewItem::setText(0, QObject::tr("Audio devices"));
		break;
	case qsamplerDevice::Midi:
		QListViewItem::setPixmap(0, QPixmap::fromMimeSource("midi1.png"));
		QListViewItem::setText(0, QObject::tr("MIDI devices"));
		break;
	}
}

qsamplerDeviceItem::qsamplerDeviceItem ( QListViewItem *pItem, lscp_client_t *pClient,
	qsamplerDevice::qsamplerDeviceType deviceType, int iDeviceID )
	: QListViewItem(pItem), m_device(pClient, deviceType, iDeviceID)
{
	switch(m_device.deviceType()) {
	case qsamplerDevice::Audio:
		QListViewItem::setPixmap(0, QPixmap::fromMimeSource("audio2.png"));
		break;
	case qsamplerDevice::Midi:
		QListViewItem::setPixmap(0, QPixmap::fromMimeSource("midi2.png"));
		break;
	}

	QListViewItem::setText(0, m_device.deviceName());
}

// Default destructor.
qsamplerDeviceItem::~qsamplerDeviceItem (void)
{
}

// Instance accessors.
qsamplerDevice& qsamplerDeviceItem::device (void)
{
	return m_device;
}

// To virtually distinguish between list view items.
int qsamplerDeviceItem::rtti() const
{
	return QSAMPLER_DEVICE_ITEM;
}



//-------------------------------------------------------------------------
// qsamplerDeviceParamTable - Device parameter view table.
//

// Constructor.
qsamplerDeviceParamTable::qsamplerDeviceParamTable ( QWidget *pParent, const char *pszName )
	: QTable(pParent, pszName)
{
	// Set fixed number of columns.
	QTable::setNumCols(3);
	QTable::setShowGrid(false);
	QTable::setSorting(false);
	QTable::setFocusStyle(QTable::FollowStyle);
	QTable::setSelectionMode(QTable::NoSelection);
	// No vertical header.
	QTable::verticalHeader()->hide();
	QTable::setLeftMargin(0);
	// Initialize the fixed table column headings.
	QHeader *pHeader = QTable::horizontalHeader();
	pHeader->setLabel(0, tr("Parameter"));
	pHeader->setLabel(1, tr("Value"));
	pHeader->setLabel(2, tr("Description"));
	// Set read-onlyness of each column
	QTable::setColumnReadOnly(0, true);
//  QTable::setColumnReadOnly(1, true); -- of course not.
	QTable::setColumnReadOnly(2, true);
}

// Default destructor.
qsamplerDeviceParamTable::~qsamplerDeviceParamTable (void)
{
}


// The main table refresher.
void qsamplerDeviceParamTable::refresh ( qsamplerDevice& device )
{
	// Always (re)start it empty.
	QTable::setNumRows(0);

	// Now fill the parameter table...
	qsamplerDeviceParamMap& params = device.params();
	QTable::insertRows(0, params.count());
	int iRow = 0;
	qsamplerDeviceParamMap::ConstIterator iter;
	for (iter = params.begin(); iter != params.end(); ++iter) {
		QTable::setText(iRow, 0, iter.key());
		QTable::setText(iRow, 1, iter.data().value);
		QTable::setText(iRow, 2, iter.data().description);
		++iRow;
	}

	// Adjust optimal column width.
	for (int iCol = 0; iCol < QTable::numCols(); iCol++)
		QTable::adjustColumn(iCol);
}


// end of qsamplerDevice.cpp
