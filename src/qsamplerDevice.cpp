// qsamplerDevice.cpp
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

#include "qsamplerAbout.h"
#include "qsamplerDevice.h"

#include "qsamplerMainForm.h"
#include "qsamplerDeviceForm.h"

#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::DeviceParam - MIDI/Audio Device parameter structure.
//

// Constructors.
DeviceParam::DeviceParam ( lscp_param_info_t *pParamInfo,
	const char *pszValue )
{
	setParam(pParamInfo, pszValue);
}


// Default destructor.
DeviceParam::~DeviceParam (void)
{
}


// Initializer.
void DeviceParam::setParam ( lscp_param_info_t *pParamInfo,
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

	mandatory = (bool) pParamInfo->mandatory;
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
// QSampler::Device - MIDI/Audio Device structure.
//

// Constructor.
Device::Device ( DeviceType deviceType, int iDeviceID )
{
//	m_ports.setAutoDelete(true);

	setDevice(deviceType, iDeviceID);
}

// Default destructor.
Device::~Device (void)
{
	qDeleteAll(m_ports);
	m_ports.clear();
}

// Copy constructor.
Device::Device ( const Device& device )
	: m_params(device.m_params), m_ports(device.m_ports)
{
	m_iDeviceID   = device.m_iDeviceID;
	m_deviceType  = device.m_deviceType;
	m_sDeviceType = device.m_sDeviceType;
	m_sDriverName = device.m_sDriverName;
	m_sDeviceName = device.m_sDeviceName;
}


// Initializer.
void Device::setDevice ( DeviceType deviceType, int iDeviceID )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	// Device id and type should be always set.
	m_iDeviceID  = iDeviceID;
	m_deviceType = deviceType;

	// Reset device parameters and ports anyway.
	m_params.clear();
	qDeleteAll(m_ports);
	m_ports.clear();

	// Retrieve device info, if any.
	lscp_device_info_t *pDeviceInfo = NULL;
	switch (deviceType) {
	case Device::Audio:
		m_sDeviceType = QObject::tr("Audio");
		if (m_iDeviceID >= 0 && (pDeviceInfo = ::lscp_get_audio_device_info(
				pMainForm->client(), m_iDeviceID)) == NULL)
			appendMessagesClient("lscp_get_audio_device_info");
		break;
	case Device::Midi:
		m_sDeviceType = QObject::tr("MIDI");
		if (m_iDeviceID >= 0 && (pDeviceInfo = ::lscp_get_midi_device_info(
				pMainForm->client(), m_iDeviceID)) == NULL)
			appendMessagesClient("lscp_get_midi_device_info");
		break;
	case Device::None:
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
		case Device::Audio:
			if ((pParamInfo = ::lscp_get_audio_driver_param_info(
					pMainForm->client(), m_sDriverName.toUtf8().constData(),
					sParam.toUtf8().constData(), NULL)) == NULL)
				appendMessagesClient("lscp_get_audio_driver_param_info");
			break;
		case Device::Midi:
			if ((pParamInfo = ::lscp_get_midi_driver_param_info(
					pMainForm->client(), m_sDriverName.toUtf8().constData(),
					sParam.toUtf8().constData(), NULL)) == NULL)
				appendMessagesClient("lscp_get_midi_driver_param_info");
			break;
		case Device::None:
			break;
		}
		if (pParamInfo) {
			m_params[sParam.toUpper()] = DeviceParam(pParamInfo,
				pDeviceInfo->params[i].value);
		}
	}

	// Refresh parameter dependencies...
	refreshParams();
	// Set port/channel list...
	refreshPorts();
}


// Driver name initializer/settler.
void Device::setDriver ( const QString& sDriverName )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	// Valid only for scratch devices.
	if (m_sDriverName == sDriverName)
		return;

	// Reset device parameters and ports anyway.
	m_params.clear();
	qDeleteAll(m_ports);
	m_ports.clear();

	// Retrieve driver info, if any.
	lscp_driver_info_t *pDriverInfo = NULL;
	switch (m_deviceType) {
	case Device::Audio:
		if ((pDriverInfo = ::lscp_get_audio_driver_info(pMainForm->client(),
				sDriverName.toUtf8().constData())) == NULL)
			appendMessagesClient("lscp_get_audio_driver_info");
		break;
	case Device::Midi:
		if ((pDriverInfo = ::lscp_get_midi_driver_info(pMainForm->client(),
				sDriverName.toUtf8().constData())) == NULL)
			appendMessagesClient("lscp_get_midi_driver_info");
		break;
	case Device::None:
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
		case Device::Audio:
			if ((pParamInfo = ::lscp_get_audio_driver_param_info(
					pMainForm->client(), sDriverName.toUtf8().constData(),
					sParam.toUtf8().constData(), NULL)) == NULL)
				appendMessagesClient("lscp_get_audio_driver_param_info");
			break;
		case Device::Midi:
			if ((pParamInfo = ::lscp_get_midi_driver_param_info(
					pMainForm->client(), sDriverName.toUtf8().constData(),
					sParam.toUtf8().constData(), NULL)) == NULL)
				appendMessagesClient("lscp_get_midi_driver_param_info");
			break;
		case Device::None:
			break;
		}
		if (pParamInfo) {
			m_params[sParam.toUpper()] = DeviceParam(pParamInfo,
				pParamInfo->defaultv);
		}
	}

	// Refresh parameter dependencies...
	refreshParams();
	// Set port/channel list...
	refreshPorts();
}


// Device property accessors.
int Device::deviceID (void) const
{
	return m_iDeviceID;
}

Device::DeviceType Device::deviceType (void) const
{
	return m_deviceType;
}

const QString& Device::deviceTypeName (void) const
{
	return m_sDeviceType;
}

const QString& Device::driverName (void) const
{
	return m_sDriverName;
}

// Special device name formatter.
QString Device::deviceName (void) const
{
	QString sPrefix;
	if (m_iDeviceID >= 0)
		sPrefix += m_sDeviceType + ' ';
	return sPrefix + m_sDeviceName;
}


// Set the proper device parameter value.
bool Device::setParam ( const QString& sParam,
	const QString& sValue )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	// Set proper device parameter.
	m_params[sParam.toUpper()].value = sValue;

	// If the device already exists, things get immediate...
	int iRefresh = 0;
	if (m_iDeviceID >= 0 && sValue != QString::null) {

		// we need temporary byte arrrays with the final strings, because
		// i.e. QString::toUtf8() only returns a temporary object and the
		// C-style char* pointers for liblscp would immediately be invalidated
		QByteArray finalParamKey = sParam.toUtf8();
		QByteArray finalParamVal = sValue.toUtf8();

		// Prepare parameter struct.
		lscp_param_t param;
		param.key   = (char *) finalParamKey.constData();
		param.value = (char *) finalParamVal.constData();
		// Now it depends on the device type...
		lscp_status_t ret = LSCP_FAILED;
		switch (m_deviceType) {
		case Device::Audio:
			if (sParam == "CHANNELS") iRefresh++;
			if ((ret = ::lscp_set_audio_device_param(pMainForm->client(),
					m_iDeviceID, &param)) != LSCP_OK)
				appendMessagesClient("lscp_set_audio_device_param");
			break;
		case Device::Midi:
			if (sParam == "PORTS") iRefresh++;
			if ((ret = ::lscp_set_midi_device_param(pMainForm->client(),
					m_iDeviceID, &param)) != LSCP_OK)
				appendMessagesClient("lscp_set_midi_device_param");
			break;
		case Device::None:
			break;
		}
		// Show result.
		if (ret == LSCP_OK) {
			appendMessages(QString("%1: %2.").arg(sParam).arg(sValue));
			// Special care for specific parameter changes...
			if (iRefresh > 0)
				iRefresh += refreshPorts();
			iRefresh += refreshDepends(sParam);
		} else {
			// Oops...
			appendMessagesError(
				QObject::tr("Could not set device parameter value.\n\nSorry."));
		}
	}

	// Return whether we're need a view refresh.
	return (iRefresh > 0);
}


// Device parameter accessor.
const DeviceParamMap& Device::params (void) const
{
	return m_params;
}


// Device port/channel list accessor.
DevicePortList& Device::ports (void)
{
	return m_ports;
}


// Create a new device, as a copy of this current one.
bool Device::createDevice (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	// we need temporary lists with the final strings, because i.e.
	// QString::toUtf8() only returns a temporary object and the
	// C-style char* pointers for liblscp would immediately be invalidated
	QList<QByteArray> finalKeys;
	QList<QByteArray> finalVals;

	DeviceParamMap::ConstIterator iter;
	for (iter = m_params.begin(); iter != m_params.end(); ++iter) {
		if (iter.value().value == QString::null) continue;
		finalKeys.push_back(iter.key().toUtf8());
		finalVals.push_back(iter.value().value.toUtf8());
	}

	// yeah, we DO HAVE to do the two loops separately !

	// Build the parameter list...
	lscp_param_t *pParams = new lscp_param_t [finalKeys.count() + 1];
	int iParam;
	for (iParam = 0; iParam < finalKeys.count(); iParam++) {
		pParams[iParam].key   = (char *) finalKeys[iParam].constData();
		pParams[iParam].value = (char *) finalVals[iParam].constData();
	}
	// Null terminated.
	pParams[iParam].key   = NULL;
	pParams[iParam].value = NULL;

	// Now it depends on the device type...
	switch (m_deviceType) {
	case Device::Audio:
		if ((m_iDeviceID = ::lscp_create_audio_device(pMainForm->client(),
				m_sDriverName.toUtf8().constData(), pParams)) < 0)
			appendMessagesClient("lscp_create_audio_device");
		break;
	case Device::Midi:
		if ((m_iDeviceID = ::lscp_create_midi_device(pMainForm->client(),
				m_sDriverName.toUtf8().constData(), pParams)) < 0)
			appendMessagesClient("lscp_create_midi_device");
		break;
	case Device::None:
		break;
	}

	// Free used parameter array.
	delete pParams;

	// Show result.
	if (m_iDeviceID >= 0) {
		// Refresh our own stuff...
		setDevice(m_deviceType, m_iDeviceID);
		appendMessages(QObject::tr("created."));
	} else {
		appendMessagesError(QObject::tr("Could not create device.\n\nSorry."));
	}

	// Return whether we're a valid device...
	return (m_iDeviceID >= 0);
}


// Destroy existing device.
bool Device::deleteDevice (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	// Now it depends on the device type...
	lscp_status_t ret = LSCP_FAILED;
	switch (m_deviceType) {
	case Device::Audio:
		if ((ret = ::lscp_destroy_audio_device(pMainForm->client(),
				m_iDeviceID)) != LSCP_OK)
			appendMessagesClient("lscp_destroy_audio_device");
		break;
	case Device::Midi:
		if ((ret = ::lscp_destroy_midi_device(pMainForm->client(),
				m_iDeviceID)) != LSCP_OK)
			appendMessagesClient("lscp_destroy_midi_device");
		break;
	case Device::None:
		break;
	}

	// Show result.
	if (ret == LSCP_OK) {
		appendMessages(QObject::tr("deleted."));
		m_iDeviceID = -1;
	} else {
		appendMessagesError(QObject::tr("Could not delete device.\n\nSorry."));
	}

	// Return whether we've done it..
	return (ret == LSCP_OK);
}


// Device parameter dependencies refreshner.
int Device::refreshParams (void)
{
	// This should only make sense for scratch devices...
	if (m_iDeviceID >= 0)
		return 0;
	// Refresh all parameters that have dependencies...
	int iParams = 0;
	DeviceParamMap::ConstIterator iter;
	for (iter = m_params.begin(); iter != m_params.end(); ++iter)
		iParams += refreshParam(iter.key());
	// Return how many parameters have been refreshed...
	return iParams;
}


// Device port/channel list refreshner.
int Device::refreshPorts (void)
{
	// This should only make sense for actual devices...
	if (m_iDeviceID < 0)
		return 0;
	// Port/channel count determination...
	int iPorts = 0;
	switch (m_deviceType) {
	case Device::Audio:
		iPorts = m_params["CHANNELS"].value.toInt();
		break;
	case Device::Midi:
		iPorts = m_params["PORTS"].value.toInt();
		break;
	case Device::None:
		break;
	}
	// Retrieve port/channel information...
	qDeleteAll(m_ports);
	m_ports.clear();
	for (int iPort = 0; iPort < iPorts; iPort++)
		m_ports.append(new DevicePort(*this, iPort));
	// Return how many ports have been refreshed...
	return iPorts;
}


// Refresh/set dependencies given that some parameter has changed.
int Device::refreshDepends ( const QString& sParam )
{
	// This should only make sense for scratch devices...
	if (m_iDeviceID >= 0)
		return 0;
	// Refresh all parameters that depend on this one...
	int iDepends = 0;
	DeviceParamMap::ConstIterator iter;
	for (iter = m_params.begin(); iter != m_params.end(); ++iter) {
		const QStringList& depends = iter.value().depends;
		if (depends.indexOf(sParam) >= 0)
			iDepends += refreshParam(iter.key());
	}
	// Return how many dependencies have been refreshed...
	return iDepends;
}


// Refresh/set given parameter based on driver supplied dependencies.
int Device::refreshParam ( const QString& sParam )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return 0;
	if (pMainForm->client() == NULL)
		return 0;

	// Check if we have dependencies...
	DeviceParam& param = m_params[sParam.toUpper()];
	if (param.depends.isEmpty())
		return 0;

	int iRefresh = 0;

	// Build dependency list...
	lscp_param_t *pDepends = new lscp_param_t [param.depends.count() + 1];
	int iDepend = 0;

	// we need temporary lists with the final strings, because i.e.
	// QString::toUtf8() only returns a temporary object and the
	// C-style char* pointers for liblscp would immediately be invalidated
	QList<QByteArray> finalKeys;
	QList<QByteArray> finalVals;
	for (int i = 0; i < param.depends.count(); i++) {
		const QString& sDepend = param.depends[i];
		finalKeys.push_back(sDepend.toUtf8());
		finalVals.push_back(m_params[sDepend.toUpper()].value.toUtf8());
	}
	// yeah, we DO HAVE to do those two loops separately !
	for (int i = 0; i < param.depends.count(); i++) {
		pDepends[iDepend].key   = (char *) finalKeys[i].constData();
		pDepends[iDepend].value = (char *) finalVals[i].constData();
		++iDepend;
	}
	// Null terminated.
	pDepends[iDepend].key   = NULL;
	pDepends[iDepend].value = NULL;

	// FIXME: Some parameter dependencies (e.g.ALSA CARD)
	// are blocking for no reason, causing potential timeout-crashes.
	// hopefully this gets mitigated if this dependency hell is only
	// carried out for scratch devices...

	// Retrieve some modern parameters...
	lscp_param_info_t *pParamInfo = NULL;
	switch (m_deviceType) {
	case Device::Audio:
		if ((pParamInfo = ::lscp_get_audio_driver_param_info(
				pMainForm->client(), m_sDriverName.toUtf8().constData(),
				sParam.toUtf8().constData(), pDepends)) == NULL)
			appendMessagesClient("lscp_get_audio_driver_param_info");
		break;
	case Device::Midi:
		if ((pParamInfo = ::lscp_get_midi_driver_param_info(
				pMainForm->client(), m_sDriverName.toUtf8().constData(),
				sParam.toUtf8().constData(), pDepends)) == NULL)
			appendMessagesClient("lscp_get_midi_driver_param_info");
		break;
	case Device::None:
		break;
	}
	if (pParamInfo) {
		param = DeviceParam(pParamInfo,
			param.value.isEmpty() ? NULL : param.value.toUtf8().constData());
		iRefresh++;
	}

	// Free used parameter array.
	delete pDepends;

	// Return whether the parameters has been changed...
	return iRefresh;
}


// Redirected messages output methods.
void Device::appendMessages( const QString& s ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessages(deviceName() + ' ' + s);
}

void Device::appendMessagesColor( const QString& s,
	const QString& c ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessagesColor(deviceName() + ' ' + s, c);
}

void Device::appendMessagesText( const QString& s ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessagesText(deviceName() + ' ' + s);
}

void Device::appendMessagesError( const QString& s ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessagesError(deviceName() + "\n\n" + s);
}

void Device::appendMessagesClient( const QString& s ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessagesClient(deviceName() + ' ' + s);
}


// Device ids enumerator.
int *Device::getDevices ( lscp_client_t *pClient,
	DeviceType deviceType )
{
	int *piDeviceIDs = NULL;
	switch (deviceType) {
	case Device::Audio:
		piDeviceIDs = ::lscp_list_audio_devices(pClient);
		break;
	case Device::Midi:
		piDeviceIDs = ::lscp_list_midi_devices(pClient);
		break;
	case Device::None:
		break;
	}
	return piDeviceIDs;
}


// Driver names enumerator.
QStringList Device::getDrivers ( lscp_client_t *pClient,
	DeviceType deviceType )
{
	QStringList drivers;

	const char **ppszDrivers = NULL;
	switch (deviceType) {
	case Device::Audio:
		ppszDrivers = ::lscp_list_available_audio_drivers(pClient);
		break;
	case Device::Midi:
		ppszDrivers = ::lscp_list_available_midi_drivers(pClient);
		break;
	case Device::None:
		break;
	}

	for (int iDriver = 0; ppszDrivers && ppszDrivers[iDriver]; iDriver++)
		drivers.append(ppszDrivers[iDriver]);

	return drivers;
}


//-------------------------------------------------------------------------
// QSampler::DevicePort - MIDI/Audio Device port/channel structure.
//

// Constructor.
DevicePort::DevicePort ( Device& device,
	int iPortID ) : m_device(device)
{
	setDevicePort(iPortID);
}

// Default destructor.
DevicePort::~DevicePort (void)
{
}


// Initializer.
void DevicePort::setDevicePort ( int iPortID )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	// Device port id should be always set.
	m_iPortID = iPortID;

	// Reset port parameters anyway.
	m_params.clear();

	// Retrieve device port/channel info, if any.
	lscp_device_port_info_t *pPortInfo = NULL;
	switch (m_device.deviceType()) {
	case Device::Audio:
		if ((pPortInfo = ::lscp_get_audio_channel_info(pMainForm->client(),
				m_device.deviceID(), m_iPortID)) == NULL)
			m_device.appendMessagesClient("lscp_get_audio_channel_info");
		break;
	case Device::Midi:
		if ((pPortInfo = ::lscp_get_midi_port_info(pMainForm->client(),
				m_device.deviceID(), m_iPortID)) == NULL)
			m_device.appendMessagesClient("lscp_get_midi_port_info");
		break;
	case Device::None:
		break;
	}

	// If we're bogus, bail out...
	if (pPortInfo == NULL) {
		m_sPortName = QString::null;
		return;
	}

	// Set device port/channel properties...
	m_sPortName = pPortInfo->name;

	// Grab device port/channel parameters...
	m_params.clear();
	for (int i = 0; pPortInfo->params && pPortInfo->params[i].key; i++) {
		const QString sParam = pPortInfo->params[i].key;
		lscp_param_info_t *pParamInfo = NULL;
		switch (m_device.deviceType()) {
		case Device::Audio:
			if ((pParamInfo = ::lscp_get_audio_channel_param_info(
					pMainForm->client(), m_device.deviceID(),
					m_iPortID, sParam.toUtf8().constData())) == NULL)
				m_device.appendMessagesClient("lscp_get_audio_channel_param_info");
			break;
		case Device::Midi:
			if ((pParamInfo = ::lscp_get_midi_port_param_info(
					pMainForm->client(), m_device.deviceID(),
					m_iPortID, sParam.toUtf8().constData())) == NULL)
				m_device.appendMessagesClient("lscp_get_midi_port_param_info");
			break;
		case Device::None:
			break;
		}
		if (pParamInfo) {
			m_params[sParam.toUpper()] = DeviceParam(pParamInfo,
				pPortInfo->params[i].value);
		}
	}
}


// Device port/channel property accessors.
int DevicePort::portID (void) const
{
	return m_iPortID;
}

const QString& DevicePort::portName (void) const
{
	return m_sPortName;
}

// Device port/channel parameter accessor.
const DeviceParamMap& DevicePort::params (void) const
{
	return m_params;
}


// Set the proper device port/channel parameter value.
bool DevicePort::setParam ( const QString& sParam,
	const QString& sValue )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	// Set proper port/channel parameter.
	m_params[sParam.toUpper()].value = sValue;

	// If the device already exists, things get immediate...
	int iRefresh = 0;
	if (m_device.deviceID() >= 0 && m_iPortID >= 0) {

		// we need temporary byte arrrays with the final strings, because
		// i.e. QString::toUtf8() only returns a temporary object and the
		// C-style char* pointers for liblscp would immediately be invalidated
		QByteArray finalParamKey = sParam.toUtf8();
		QByteArray finalParamVal = sValue.toUtf8();

		// Prepare parameter struct.
		lscp_param_t param;
		param.key   = (char *) finalParamKey.constData();
		param.value = (char *) finalParamVal.constData();
		// Now it depends on the device type...
		lscp_status_t ret = LSCP_FAILED;
		switch (m_device.deviceType()) {
		case Device::Audio:
			if ((ret = ::lscp_set_audio_channel_param(pMainForm->client(),
					m_device.deviceID(), m_iPortID, &param)) != LSCP_OK)
				m_device.appendMessagesClient("lscp_set_audio_channel_param");
			break;
		case Device::Midi:
			if ((ret = ::lscp_set_midi_port_param(pMainForm->client(),
					m_device.deviceID(), m_iPortID, &param)) != LSCP_OK)
				m_device.appendMessagesClient("lscp_set_midi_port_param");
			break;
		case Device::None:
			break;
		}
		// Show result.
		if (ret == LSCP_OK) {
			m_device.appendMessages(m_sPortName
				+ ' ' + QString("%1: %2.").arg(sParam).arg(sValue));
			iRefresh++;
		} else {
			m_device.appendMessagesError(
				QObject::tr("Could not set %1 parameter value.\n\n"
					"Sorry.").arg(m_sPortName));
		}
	}

	// Return whether we're need a view refresh.
	return (iRefresh > 0);
}


//-------------------------------------------------------------------------
// QSampler::DeviceItem - QTreeWidget device item.
//

// Constructors.
DeviceItem::DeviceItem ( QTreeWidget* pTreeWidget,
	Device::DeviceType deviceType )
	: QTreeWidgetItem(pTreeWidget, QSAMPLER_DEVICE_ITEM),
	m_device(deviceType)
{
	switch(m_device.deviceType()) {
	case Device::Audio:
		setIcon(0, QPixmap(":/icons/audio1.png"));
		setText(0, QObject::tr("Audio Devices"));
		break;
	case Device::Midi:
		setIcon(0, QPixmap(":/icons/midi1.png"));
		setText(0, QObject::tr("MIDI Devices"));
		break;
	case Device::None:
		break;
	}

	// Root items are not selectable...
	setFlags(flags() & ~Qt::ItemIsSelectable);
}

DeviceItem::DeviceItem ( QTreeWidgetItem* pItem,
	Device::DeviceType deviceType,
	int iDeviceID )
	: QTreeWidgetItem(pItem, QSAMPLER_DEVICE_ITEM),
	m_device(deviceType, iDeviceID)
{
	switch(m_device.deviceType()) {
	case Device::Audio:
		setIcon(0, QPixmap(":/icons/audio2.png"));
		break;
	case Device::Midi:
		setIcon(0, QPixmap(":/icons/midi2.png"));
		break;
	case Device::None:
		break;
	}

	setText(0, m_device.deviceName());
}

// Default destructor.
DeviceItem::~DeviceItem ()
{
}

// Instance accessors.
Device& DeviceItem::device ()
{
	return m_device;
}


//-------------------------------------------------------------------------
// QSampler::AbstractDeviceParamModel - data model base class for device parameters
//

AbstractDeviceParamModel::AbstractDeviceParamModel ( QObject* pParent )
	: QAbstractTableModel(pParent), m_bEditable(false)
{
	m_pParams = NULL;
}


int AbstractDeviceParamModel::rowCount ( const QModelIndex& /*parent*/) const
{
	//std::cout << "model size=" << params.size() << "\n" << std::flush;
	return (m_pParams ? m_pParams->size() : 0);
}


int AbstractDeviceParamModel::columnCount ( const QModelIndex& /*parent*/) const
{
	return 3;
}


Qt::ItemFlags AbstractDeviceParamModel::flags ( const QModelIndex& /*index*/) const
{
	return Qt::ItemIsEditable | Qt::ItemIsEnabled;
}


QVariant AbstractDeviceParamModel::headerData (
	int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal) {
		switch (section) {
			case 0:  return tr("Parameter");
			case 1:  return tr("Value");
			case 2:  return tr("Description");
			default: return QVariant();
		}
	}

	return QVariant();
}


void AbstractDeviceParamModel::refresh (
	const DeviceParamMap* pParams, bool bEditable )
{
	m_pParams   = pParams;
	m_bEditable = bEditable;
	// inform the outer world (QTableView) that our data changed
	QAbstractTableModel::reset();
}


void AbstractDeviceParamModel::clear (void)
{
	m_pParams = NULL;
	// inform the outer world (QTableView) that our data changed
	QAbstractTableModel::reset();
}


//-------------------------------------------------------------------------
// QSampler::DeviceParamModel - data model for device parameters
//                              (used for QTableView)

DeviceParamModel::DeviceParamModel ( QObject *pParent )
	: AbstractDeviceParamModel(pParent)
{
	m_pDevice = NULL;
}

QVariant DeviceParamModel::data (
	const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	DeviceParameterRow item;
	item.name  = m_pParams->keys()[index.row()];
	item.param = (*m_pParams)[item.name];
	item.alive = (m_pDevice && m_pDevice->deviceID() >= 0);

	return QVariant::fromValue(item);
}


bool DeviceParamModel::setData (
	const QModelIndex& index, const QVariant& value, int /*role*/)
{
	if (!index.isValid())
		return false;

	QString key = m_pParams->keys()[index.row()];
	//m_pParams[key].value = value.toString();
	m_pDevice->setParam(key, value.toString());
	emit dataChanged(index, index);
	return true;
}


void DeviceParamModel::refresh ( Device* pDevice, bool bEditable )
{
	m_pDevice = pDevice;
	AbstractDeviceParamModel::refresh(&pDevice->params(), bEditable);
}


void DeviceParamModel::clear (void)
{
	AbstractDeviceParamModel::clear();
	m_pDevice = NULL;
}


//-------------------------------------------------------------------------
// QSampler::PortParamModel - data model for port parameters
//                            (used for QTableView)

PortParamModel::PortParamModel ( QObject *pParent)
	: AbstractDeviceParamModel(pParent)
{
	m_pPort = NULL;
}

QVariant PortParamModel::data ( const QModelIndex &index, int role ) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	DeviceParameterRow item;
	item.name  = m_pParams->keys()[index.row()];
	item.param = (*m_pParams)[item.name];
	item.alive = (m_pPort && m_pPort->portID() >= 0);

	return QVariant::fromValue(item);
}


bool PortParamModel::setData (
	const QModelIndex& index, const QVariant& value, int /*role*/)
{
	if (!index.isValid())
		return false;

	QString key = m_pParams->keys()[index.row()];
	//params[key].value = value.toString();
	m_pPort->setParam(key, value.toString());
	emit dataChanged(index, index);
	return true;
}


void PortParamModel::refresh ( DevicePort* pPort, bool bEditable )
{
	m_pPort = pPort;
	AbstractDeviceParamModel::refresh(&pPort->params(), bEditable);
}


void PortParamModel::clear (void)
{
	AbstractDeviceParamModel::clear();
	m_pPort = NULL;
}


//-------------------------------------------------------------------------
// QSampler::DeviceParamDelegate - table cell renderer for device/port parameters
//

DeviceParamDelegate::DeviceParamDelegate ( QObject *pParent)
	: QItemDelegate(pParent)
{
}


QWidget* DeviceParamDelegate::createEditor ( QWidget *pParent,
	const QStyleOptionViewItem& /* option */, const QModelIndex& index ) const
{
	if (!index.isValid())
		return NULL;

	DeviceParameterRow r = index.model()->data(index,
		Qt::DisplayRole).value<DeviceParameterRow>();

	const bool bEnabled = (r.alive) ? !r.param.fix : true;

	QString val = (r.alive) ? r.param.value : r.param.defaultv;

	switch (index.column()) {
		case 0:
			return new QLabel(r.name, pParent);
		case 1: {
			if (r.param.type == LSCP_TYPE_BOOL) {
				QCheckBox *pCheckBox = new QCheckBox(pParent);
				if (val != QString::null)
					pCheckBox->setChecked(val.toLower() == "true");
				pCheckBox->setEnabled(bEnabled);
				return pCheckBox;
			} else if (r.param.possibilities.count() > 0) {
				QStringList opts = r.param.possibilities;
				if (r.param.multiplicity)
					opts.prepend(tr("(none)"));
				QComboBox *pComboBox = new QComboBox(pParent);
				pComboBox->addItems(opts);
				if (r.param.value.isEmpty())
					pComboBox->setCurrentIndex(0);
				else
					pComboBox->setCurrentIndex(pComboBox->findText(val));
				pComboBox->setEnabled(bEnabled);
				return pComboBox;
			} else if (r.param.type == LSCP_TYPE_INT && bEnabled) {
				QSpinBox *pSpinBox = new QSpinBox(pParent);
				pSpinBox->setMinimum(
					(!r.param.range_min.isEmpty()) ?
						r.param.range_min.toInt() : 0 // or better a negative default min value ?
				);
				pSpinBox->setMaximum(
					(!r.param.range_max.isEmpty()) ?
						r.param.range_max.toInt() : (1 << 16) // or better a nigher default max value ?
				);
				pSpinBox->setValue(val.toInt());
				return pSpinBox;
			} else if (bEnabled) {
				QLineEdit *pLineEdit = new QLineEdit(val, pParent);
				return pLineEdit;
			} else {
				QLabel *pLabel = new QLabel(val, pParent);
				return pLabel;
			}
		}
		case 2:
			return new QLabel(r.param.description, pParent);
		default:
			return NULL;
	}
}


void DeviceParamDelegate::setEditorData (
	QWidget* /*pEditor*/, const QModelIndex& /*index*/) const
{
	// Unused, since we set the editor data already in createEditor()
}


void DeviceParamDelegate::setModelData ( QWidget *pEditor,
	QAbstractItemModel *pModel, const QModelIndex& index ) const
{
	if (index.column() == 1) {
		DeviceParameterRow r = index.model()->data(index,
			Qt::DisplayRole).value<DeviceParameterRow> ();
		if (pEditor->metaObject()->className() == QString("QCheckBox")) {
			QCheckBox* pCheckBox = static_cast<QCheckBox*> (pEditor);
			pModel->setData(index, QVariant(pCheckBox->checkState() == Qt::Checked));
		} else if (pEditor->metaObject()->className() == QString("QComboBox")) {
			QComboBox* pComboBox = static_cast<QComboBox*> (pEditor);
			pModel->setData(index, pComboBox->currentText());
		} else if (pEditor->metaObject()->className() == QString("QSpinBox")) {
			QSpinBox* pSpinBox = static_cast<QSpinBox*> (pEditor);
			pModel->setData(index, pSpinBox->value());
		} else if (pEditor->metaObject()->className() == QString("QLineEdit")) {
			QLineEdit* pLineEdit = static_cast<QLineEdit*> (pEditor);
			pModel->setData(index, pLineEdit->text());
		} else if (pEditor->metaObject()->className() == QString("QLabel")) {
			QLabel* pLabel = static_cast<QLabel*> (pEditor);
			pModel->setData(index, pLabel->text());
		}
	}
}

void DeviceParamDelegate::updateEditorGeometry ( QWidget *pEditor,
	const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	if (pEditor)
		pEditor->setGeometry(option.rect);
}

} // namespace QSampler

// end of qsamplerDevice.cpp
