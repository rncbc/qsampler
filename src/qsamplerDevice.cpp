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

#include <qspinbox.h>
#include <qlineedit.h>

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
	m_ports.setAutoDelete(true);
	
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
	m_iDeviceID  = iDeviceID;
	m_deviceType = deviceType;
	
	// Reset device parameters and ports anyway.
   	m_params.clear();
   	m_ports.clear();

	// Retrieve device info, if any.
	lscp_device_info_t *pDeviceInfo = NULL;
	switch (deviceType) {
	case qsamplerDevice::Audio:
		m_sDeviceType = QObject::tr("Audio");
		pDeviceInfo = ::lscp_get_audio_device_info(pClient, iDeviceID);
		break;
	case qsamplerDevice::Midi:
		m_sDeviceType = QObject::tr("MIDI");
		pDeviceInfo = ::lscp_get_midi_device_info(pClient, iDeviceID);
		break;
	case qsamplerDevice::None:
		m_sDeviceType = QString::null;
		break;
	}

	// If we're bogus, bail out...
	if (pDeviceInfo == NULL) {
		m_sDriverName = QString::null;
		m_sDeviceName = QObject::tr("New %1 device").arg(m_sDeviceType);
		return;
	}

	// Other device properties...
	m_sDriverName = pDeviceInfo->driver;
	m_sDeviceName = m_sDriverName + ' '
		+ QObject::tr("Device %1").arg(m_iDeviceID);

	// Grab device parameters...
	for (int i = 0; pDeviceInfo->params && pDeviceInfo->params[i].key; i++) {
		const QString sParam = pDeviceInfo->params[i].key;
		lscp_param_info_t *pParamInfo = NULL;
		switch (deviceType) {
		case qsamplerDevice::Audio:
			pParamInfo = ::lscp_get_audio_driver_param_info(pClient,
				m_sDriverName.latin1(), sParam.latin1(), NULL);
			break;
		case qsamplerDevice::Midi:
			pParamInfo = ::lscp_get_midi_driver_param_info(pClient,
				m_sDriverName.latin1(), sParam.latin1(), NULL);
			break;
		case qsamplerDevice::None:
			break;
		}
		if (pParamInfo) {
			m_params[sParam.upper()] = qsamplerDeviceParam(pParamInfo,
				pDeviceInfo->params[i].value);
		}
	}

	// Grab port/channel list...
	refresh(pClient);
}


// Driver name initializer/settler.
void qsamplerDevice::setDriver ( lscp_client_t *pClient,
	const QString& sDriverName )
{
	// Valid only for scratch devices.
	if (m_sDriverName == sDriverName)
		return;

	// Reset device parameters and ports anyway.
   	m_params.clear();
   	m_ports.clear();

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
	case qsamplerDevice::None:
		break;
	}

	// If we're bogus, bail out...
	if (pDriverInfo == NULL)
		return;

	// Remember device parameters...
	m_sDriverName = sDriverName;

	// Grab driver parameters...
	for (int i = 0; pDriverInfo->parameters && pDriverInfo->parameters[i]; i++) {
		const QString sParam = pDriverInfo->parameters[i];
		lscp_param_info_t *pParamInfo = NULL;
		switch (m_deviceType) {
		case qsamplerDevice::Audio:
			pParamInfo = ::lscp_get_audio_driver_param_info(pClient,
				sDriverName.latin1(), sParam.latin1(), NULL);
			break;
		case qsamplerDevice::Midi:
			pParamInfo = ::lscp_get_midi_driver_param_info(pClient,
				sDriverName.latin1(), sParam.latin1(), NULL);
			break;
		case qsamplerDevice::None:
			break;
		}
		if (pParamInfo) {
			m_params[sParam.upper()] = qsamplerDeviceParam(pParamInfo,
				pParamInfo->defaultv);
		}
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

const QString& qsamplerDevice::deviceTypeName (void) const
{
	return m_sDeviceType;
}

const QString& qsamplerDevice::driverName (void) const
{
	return m_sDriverName;
}

const QString& qsamplerDevice::deviceName (void) const
{
	return m_sDeviceName;
}


// Set the proper device parameter value.
void qsamplerDevice::setParam ( const QString& sParam,
	const QString& sValue )
{
	m_params[sParam.upper()].value = sValue;
}


// Device parameter accessor.
const qsamplerDeviceParamMap& qsamplerDevice::params (void) const
{
	return m_params;
}


// Device port/channel list accessor.
qsamplerDevicePortList& qsamplerDevice::ports (void)
{
	return m_ports;
}


// Device port/channel list refreshner.
void qsamplerDevice::refresh ( lscp_client_t *pClient )
{
	// Port/channel count determination...
	int iPorts = 0;
	switch (m_deviceType) {
	case qsamplerDevice::Audio:
		iPorts = m_params["CHANNELS"].value.toInt();
		break;
	case qsamplerDevice::Midi:
		iPorts = m_params["PORTS"].value.toInt();
		break;
	case qsamplerDevice::None:
		break;
	}

	// Retrieve port/channel information...
	m_ports.clear();
	for (int iPort = 0; iPort < iPorts; iPort++)
		m_ports.append(new qsamplerDevicePort(pClient, *this, iPort));
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
	case qsamplerDevice::None:
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
	case qsamplerDevice::None:
		break;
	}
	
	for (int iDriver = 0; ppszDrivers[iDriver]; iDriver++)
		drivers.append(ppszDrivers[iDriver]);

	return drivers;
}


//-------------------------------------------------------------------------
// qsamplerDevicePort - MIDI/Audio Device port/channel structure.
//

// Constructor.
qsamplerDevicePort::qsamplerDevicePort ( lscp_client_t *pClient,
	const qsamplerDevice& device, int iPortID )
{
	setDevicePort(pClient, device, iPortID);
}

// Default destructor.
qsamplerDevicePort::~qsamplerDevicePort (void)
{
}


// Initializer.
void qsamplerDevicePort::setDevicePort ( lscp_client_t *pClient,
	const qsamplerDevice& device, int iPortID )
{
	// Device port id should be always set.
	m_iPortID = iPortID;

	// Reset port parameters anyway.
   	m_params.clear();

	// Retrieve device port/channel info, if any.
	QString sPrefix = device.driverName() + ' ';
	lscp_device_port_info_t *pPortInfo = NULL;
	switch (device.deviceType()) {
	case qsamplerDevice::Audio:
	    sPrefix += QObject::tr("Channel");
		pPortInfo = ::lscp_get_audio_channel_info(pClient, device.deviceID(), iPortID);
		break;
	case qsamplerDevice::Midi:
	    sPrefix += QObject::tr("Port");
		pPortInfo = ::lscp_get_midi_port_info(pClient, device.deviceID(), iPortID);
		break;
	case qsamplerDevice::None:
		break;
	}

	// If we're bogus, bail out...
	if (pPortInfo == NULL) {
		m_sPortName = QString::null;
		return;
	}

	// Set device port/channel properties...
	sPrefix += " %1:";
	m_sPortName = sPrefix.arg(m_iPortID)  + ' ' + pPortInfo->name;

	// Grab device port/channel parameters...
	m_params.clear();
	for (int i = 0; pPortInfo->params && pPortInfo->params[i].key; i++) {
		const QString sParam = pPortInfo->params[i].key;
		lscp_param_info_t *pParamInfo = NULL;
		switch (device.deviceType()) {
		case qsamplerDevice::Audio:
			pParamInfo = ::lscp_get_audio_channel_param_info(pClient,
				device.deviceID(), iPortID, sParam.latin1());
			break;
		case qsamplerDevice::Midi:
			pParamInfo = ::lscp_get_midi_port_param_info(pClient,
				device.deviceID(), iPortID, sParam.latin1());
			break;
		case qsamplerDevice::None:
			break;
		}
		if (pParamInfo) {
			m_params[sParam.upper()] = qsamplerDeviceParam(pParamInfo,
				pPortInfo->params[i].value);
		}
	}
}


// Device port/channel property accessors.
int qsamplerDevicePort::portID (void) const
{
	return m_iPortID;
}

const QString& qsamplerDevicePort::portName (void) const
{
	return m_sPortName;
}

// Device port/channel parameter accessor.
const qsamplerDeviceParamMap& qsamplerDevicePort::params (void) const
{
	return m_params;
}


// Set the proper device port/channel parameter value.
void qsamplerDevicePort::setParam ( const QString& sParam,
	const QString& sValue )
{
	m_params[sParam.upper()].value = sValue;
}


//-------------------------------------------------------------------------
// qsamplerDeviceItem - QListView device item.
//

// Constructors.
qsamplerDeviceItem::qsamplerDeviceItem ( QListView *pListView,
	lscp_client_t *pClient,	qsamplerDevice::qsamplerDeviceType deviceType )
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
	case qsamplerDevice::None:
		break;
	}
}

qsamplerDeviceItem::qsamplerDeviceItem ( QListViewItem *pItem,
	lscp_client_t *pClient, qsamplerDevice::qsamplerDeviceType deviceType,
	int iDeviceID )
	: QListViewItem(pItem), m_device(pClient, deviceType, iDeviceID)
{
	switch(m_device.deviceType()) {
	case qsamplerDevice::Audio:
		QListViewItem::setPixmap(0, QPixmap::fromMimeSource("audio2.png"));
		break;
	case qsamplerDevice::Midi:
		QListViewItem::setPixmap(0, QPixmap::fromMimeSource("midi2.png"));
		break;
	case qsamplerDevice::None:
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
qsamplerDeviceParamTable::qsamplerDeviceParamTable ( QWidget *pParent,
	const char *pszName )
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
	pHeader->setLabel(1, tr("Description"));
	pHeader->setLabel(2, tr("Value"));
	// Set read-onlyness of each column
	QTable::setColumnReadOnly(0, true);
	QTable::setColumnReadOnly(1, true);
//  QTable::setColumnReadOnly(2, true); -- of course not.
	QTable::setColumnStretchable(1, true);
}

// Default destructor.
qsamplerDeviceParamTable::~qsamplerDeviceParamTable (void)
{
}


// Common parameter table renderer.
void qsamplerDeviceParamTable::refresh ( const qsamplerDeviceParamMap& params,
	bool bEditable )
{
	// Always (re)start it empty.
	QTable::setUpdatesEnabled(false);
	QTable::setNumRows(0);

	// Fill the parameter table...
	QTable::insertRows(0, params.count());
	int iRow = 0;
	qsamplerDeviceParamMap::ConstIterator iter;
	for (iter = params.begin(); iter != params.end(); ++iter) {
		const qsamplerDeviceParam& param = iter.data();
		bool bEnabled = (bEditable || !param.fix);
		QTable::setText(iRow, 0, iter.key());
		QTable::setText(iRow, 1, param.description);
		if (param.type == LSCP_TYPE_BOOL) {
			QStringList opts;
			opts.append(tr("false"));
			opts.append(tr("true"));
			QComboTableItem *pComboItem = new QComboTableItem(this, opts);
			pComboItem->setCurrentItem(param.value.lower() == "true" ? 1 : 0);
			pComboItem->setEnabled(bEnabled);
			QTable::setItem(iRow, 2, pComboItem);
		} else if (param.possibilities.count() > 0 && bEnabled) {
			QComboTableItem *pComboItem = new QComboTableItem(this,
				param.possibilities);
			pComboItem->setCurrentItem(param.value);
			pComboItem->setEnabled(bEnabled);
			pComboItem->setEditable(bEnabled && param.multiplicity);
			QTable::setItem(iRow, 2, pComboItem);
		} else if (param.type == LSCP_TYPE_INT && bEnabled
				&& !param.range_min.isEmpty()
				&& !param.range_max.isEmpty()) {
			qsamplerDeviceParamTableSpinBox *pSpinItem =
				new qsamplerDeviceParamTableSpinBox(this,
					bEnabled ? QTableItem::OnTyping : QTableItem::Never,
					param.value);
			pSpinItem->setMinValue(param.range_min.toInt());
			pSpinItem->setMaxValue(param.range_max.toInt());
			QTable::setItem(iRow, 2, pSpinItem);
		} else {
			qsamplerDeviceParamTableEditBox *pEditItem =
				new qsamplerDeviceParamTableEditBox(this,
					bEnabled ? QTableItem::OnTyping : QTableItem::Never,
					param.value);
			QTable::setItem(iRow, 2, pEditItem);
		}
		++iRow;
	}

	// Adjust optimal column widths.
	QTable::adjustColumn(0);
	QTable::adjustColumn(2);

	QTable::setUpdatesEnabled(true);
	QTable::updateContents();
}


//-------------------------------------------------------------------------
// qsamplerDeviceParamTableSpinBox - Custom spin box for parameter table.
//

// Constructor.
qsamplerDeviceParamTableSpinBox::qsamplerDeviceParamTableSpinBox (
	QTable *pTable, EditType editType, const QString& sText )
	: QTableItem(pTable, editType, sText)
{
	m_iValue = sText.toInt();
	m_iMinValue = m_iMaxValue = 0;
}

// Public accessors.
void qsamplerDeviceParamTableSpinBox::setValue ( int iValue )
{
	m_iValue = iValue;
	QTableItem::setText(QString::number(m_iValue));
}

void qsamplerDeviceParamTableSpinBox::setMinValue ( int iMinValue )
{
	m_iMinValue = iMinValue;
}

void qsamplerDeviceParamTableSpinBox::setMaxValue ( int iMaxValue )
{
	m_iMaxValue = iMaxValue;
}

// Virtual implemetations.
QWidget *qsamplerDeviceParamTableSpinBox::createEditor (void) const
{
	QSpinBox *pSpinBox = new QSpinBox(QTableItem::table()->viewport());
	QObject::connect(pSpinBox, SIGNAL(valueChanged(int)),
		QTableItem::table(), SLOT(doValueChanged()));
	if (m_iValue >= m_iMinValue && m_iMaxValue >= m_iValue) {
		pSpinBox->setMinValue(m_iMinValue);
		pSpinBox->setMaxValue(m_iMaxValue);
	}
	pSpinBox->setValue(m_iValue);
	return pSpinBox;
}

void qsamplerDeviceParamTableSpinBox::setContentFromEditor ( QWidget *pWidget )
{
	if (pWidget->inherits("QSpinBox"))
		QTableItem::setText(QString::number(((QSpinBox *) pWidget)->value()));
	else
		QTableItem::setContentFromEditor(pWidget);
}


//-------------------------------------------------------------------------
// qsamplerDeviceParamTableEditBox - Custom edit box for parameter table.
//

// Constructor.
qsamplerDeviceParamTableEditBox::qsamplerDeviceParamTableEditBox (
	QTable *pTable, EditType editType, const QString& sText )
	: QTableItem(pTable, editType, sText)
{
}

// Virtual implemetations.
QWidget *qsamplerDeviceParamTableEditBox::createEditor (void) const
{
	QLineEdit *pEditBox = new QLineEdit(QTableItem::table()->viewport());
	QObject::connect(pEditBox, SIGNAL(returnPressed()),
		QTableItem::table(), SLOT(doValueChanged()));
	pEditBox->setText(QTableItem::text());
	return pEditBox;
}

void qsamplerDeviceParamTableEditBox::setContentFromEditor ( QWidget *pWidget )
{
	if (pWidget->inherits("QLineEdit"))
		QTableItem::setText(((QLineEdit *) pWidget)->text());
	else
		QTableItem::setContentFromEditor(pWidget);
}


// end of qsamplerDevice.cpp
