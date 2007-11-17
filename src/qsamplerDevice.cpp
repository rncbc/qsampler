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

#include <qspinbox.h>
#include <qlineedit.h>

using namespace QSampler;

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
// qsamplerDevice - MIDI/Audio Device structure.
//

// Constructor.
qsamplerDevice::qsamplerDevice ( qsamplerDeviceType deviceType, int iDeviceID )
{
	m_ports.setAutoDelete(true);

	setDevice(deviceType, iDeviceID);
}

// Default destructor.
qsamplerDevice::~qsamplerDevice (void)
{
}

// Copy constructor.
qsamplerDevice::qsamplerDevice ( const qsamplerDevice& device )
	: m_params(device.m_params), m_ports(m_ports)
{
	m_iDeviceID   = device.m_iDeviceID;
	m_deviceType  = device.m_deviceType;
	m_sDeviceType = device.m_sDeviceType;
	m_sDriverName = device.m_sDriverName;
	m_sDeviceName = device.m_sDeviceName;
}


// Initializer.
void qsamplerDevice::setDevice ( qsamplerDeviceType deviceType, int iDeviceID )
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
	m_ports.clear();

	// Retrieve device info, if any.
	lscp_device_info_t *pDeviceInfo = NULL;
	switch (deviceType) {
	case qsamplerDevice::Audio:
		m_sDeviceType = QObject::tr("Audio");
		if (m_iDeviceID >= 0 && (pDeviceInfo = ::lscp_get_audio_device_info(
				pMainForm->client(), m_iDeviceID)) == NULL)
			appendMessagesClient("lscp_get_audio_device_info");
		break;
	case qsamplerDevice::Midi:
		m_sDeviceType = QObject::tr("MIDI");
		if (m_iDeviceID >= 0 && (pDeviceInfo = ::lscp_get_midi_device_info(
				pMainForm->client(), m_iDeviceID)) == NULL)
			appendMessagesClient("lscp_get_midi_device_info");
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
			if ((pParamInfo = ::lscp_get_audio_driver_param_info(pMainForm->client(),
					m_sDriverName.latin1(), sParam.latin1(), NULL)) == NULL)
				appendMessagesClient("lscp_get_audio_driver_param_info");
			break;
		case qsamplerDevice::Midi:
			if ((pParamInfo = ::lscp_get_midi_driver_param_info(pMainForm->client(),
					m_sDriverName.latin1(), sParam.latin1(), NULL)) == NULL)
				appendMessagesClient("lscp_get_midi_driver_param_info");
			break;
		case qsamplerDevice::None:
			break;
		}
		if (pParamInfo) {
			m_params[sParam.upper()] = qsamplerDeviceParam(pParamInfo,
				pDeviceInfo->params[i].value);
		}
	}

	// Refresh parameter dependencies...
	refreshParams();
	// Set port/channel list...
	refreshPorts();
}


// Driver name initializer/settler.
void qsamplerDevice::setDriver ( const QString& sDriverName )
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
	m_ports.clear();

	// Retrieve driver info, if any.
	lscp_driver_info_t *pDriverInfo = NULL;
	switch (m_deviceType) {
	case qsamplerDevice::Audio:
		if ((pDriverInfo = ::lscp_get_audio_driver_info(pMainForm->client(),
				sDriverName.latin1())) == NULL)
			appendMessagesClient("lscp_get_audio_driver_info");
		break;
	case qsamplerDevice::Midi:
		if ((pDriverInfo = ::lscp_get_midi_driver_info(pMainForm->client(),
				sDriverName.latin1())) == NULL)
			appendMessagesClient("lscp_get_midi_driver_info");
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
			if ((pParamInfo = ::lscp_get_audio_driver_param_info(pMainForm->client(),
					sDriverName.latin1(), sParam.latin1(), NULL)) == NULL)
				appendMessagesClient("lscp_get_audio_driver_param_info");
			break;
		case qsamplerDevice::Midi:
			if ((pParamInfo = ::lscp_get_midi_driver_param_info(pMainForm->client(),
					sDriverName.latin1(), sParam.latin1(), NULL)) == NULL)
				appendMessagesClient("lscp_get_midi_driver_param_info");
			break;
		case qsamplerDevice::None:
			break;
		}
		if (pParamInfo) {
			m_params[sParam.upper()] = qsamplerDeviceParam(pParamInfo,
				pParamInfo->defaultv);
		}
	}

	// Refresh parameter dependencies...
	refreshParams();
	// Set port/channel list...
	refreshPorts();
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

// Special device name formatter.
QString qsamplerDevice::deviceName (void) const
{
	QString sPrefix;
	if (m_iDeviceID >= 0)
	    sPrefix += m_sDeviceType + ' ';
	return sPrefix + m_sDeviceName;
}


// Set the proper device parameter value.
bool qsamplerDevice::setParam ( const QString& sParam,
	const QString& sValue )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	// Set proper device parameter.
	m_params[sParam.upper()].value = sValue;

	// If the device already exists, things get immediate...
	int iRefresh = 0;
	if (m_iDeviceID >= 0 && sValue != QString::null) {
		// Prepare parameter struct.
		lscp_param_t param;
		param.key   = (char *) sParam.latin1();
		param.value = (char *) sValue.latin1();
		// Now it depends on the device type...
		lscp_status_t ret = LSCP_FAILED;
		switch (m_deviceType) {
		case qsamplerDevice::Audio:
		    if (sParam == "CHANNELS") iRefresh++;
			if ((ret = ::lscp_set_audio_device_param(pMainForm->client(),
					m_iDeviceID, &param)) != LSCP_OK)
				appendMessagesClient("lscp_set_audio_device_param");
			break;
		case qsamplerDevice::Midi:
		    if (sParam == "PORTS") iRefresh++;
			if ((ret = ::lscp_set_midi_device_param(pMainForm->client(),
					m_iDeviceID, &param)) != LSCP_OK)
				appendMessagesClient("lscp_set_midi_device_param");
			break;
		case qsamplerDevice::None:
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
const qsamplerDeviceParamMap& qsamplerDevice::params (void) const
{
	return m_params;
}


// Device port/channel list accessor.
qsamplerDevicePortList& qsamplerDevice::ports (void)
{
	return m_ports;
}


// Create a new device, as a copy of this current one.
bool qsamplerDevice::createDevice (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	// Build the parameter list...
	lscp_param_t *pParams = new lscp_param_t [m_params.count() + 1];
	int iParam = 0;
	qsamplerDeviceParamMap::ConstIterator iter;
	for (iter = m_params.begin(); iter != m_params.end(); ++iter) {
		if (iter.data().value == QString::null) continue;
		pParams[iParam].key   = (char *) iter.key().latin1();
		pParams[iParam].value = (char *) iter.data().value.latin1();
		++iParam;
	}
	// Null terminated.
	pParams[iParam].key   = NULL;
	pParams[iParam].value = NULL;

	// Now it depends on the device type...
	switch (m_deviceType) {
	case qsamplerDevice::Audio:
		if ((m_iDeviceID = ::lscp_create_audio_device(pMainForm->client(),
				m_sDriverName.latin1(), pParams)) < 0)
			appendMessagesClient("lscp_create_audio_device");
		break;
	case qsamplerDevice::Midi:
		if ((m_iDeviceID = ::lscp_create_midi_device(pMainForm->client(),
				m_sDriverName.latin1(), pParams)) < 0)
			appendMessagesClient("lscp_create_midi_device");
		break;
	case qsamplerDevice::None:
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
bool qsamplerDevice::deleteDevice (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	// Now it depends on the device type...
	lscp_status_t ret = LSCP_FAILED;
	switch (m_deviceType) {
	case qsamplerDevice::Audio:
		if ((ret = ::lscp_destroy_audio_device(pMainForm->client(),
				m_iDeviceID)) != LSCP_OK)
			appendMessagesClient("lscp_destroy_audio_device");
		break;
	case qsamplerDevice::Midi:
		if ((ret = ::lscp_destroy_midi_device(pMainForm->client(),
				m_iDeviceID)) != LSCP_OK)
			appendMessagesClient("lscp_destroy_midi_device");
		break;
	case qsamplerDevice::None:
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
int qsamplerDevice::refreshParams (void)
{
	// This should only make sense for scratch devices...
	if (m_iDeviceID >= 0)
	    return 0;
	// Refresh all parameters that have dependencies...
	int iParams = 0;
	qsamplerDeviceParamMap::ConstIterator iter;
	for (iter = m_params.begin(); iter != m_params.end(); ++iter)
		iParams += refreshParam(iter.key());
	// Return how many parameters have been refreshed...
	return iParams;
}


// Device port/channel list refreshner.
int qsamplerDevice::refreshPorts (void)
{
	// This should only make sense for actual devices...
	if (m_iDeviceID < 0)
	    return 0;
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
		m_ports.append(new qsamplerDevicePort(*this, iPort));
	// Return how many ports have been refreshed...
	return iPorts;
}


// Refresh/set dependencies given that some parameter has changed.
int qsamplerDevice::refreshDepends ( const QString& sParam )
{
	// This should only make sense for scratch devices...
	if (m_iDeviceID >= 0)
	    return 0;
	// Refresh all parameters that depend on this one...
	int iDepends = 0;
	qsamplerDeviceParamMap::ConstIterator iter;
	for (iter = m_params.begin(); iter != m_params.end(); ++iter) {
		const QStringList& depends = iter.data().depends;
		if (depends.find(sParam) != depends.end())
			iDepends += refreshParam(iter.key());
	}
	// Return how many dependencies have been refreshed...
	return iDepends;
}


// Refresh/set given parameter based on driver supplied dependencies.
int qsamplerDevice::refreshParam ( const QString& sParam )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return 0;
	if (pMainForm->client() == NULL)
		return 0;

	// Check if we have dependencies...
	qsamplerDeviceParam& param = m_params[sParam.upper()];
	if (param.depends.isEmpty())
		return 0;

	int iRefresh = 0;

	// Build dependency list...
	lscp_param_t *pDepends = new lscp_param_t [param.depends.count() + 1];
	int iDepend = 0;
	QStringList::ConstIterator iter;
	for (iter = param.depends.begin(); iter != param.depends.end(); ++iter) {
		const QString& sDepend = *iter;
		pDepends[iDepend].key   = (char *) sDepend.latin1();
		pDepends[iDepend].value = (char *) m_params[sDepend.upper()].value.latin1();
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
	case qsamplerDevice::Audio:
		if ((pParamInfo = ::lscp_get_audio_driver_param_info(pMainForm->client(),
				m_sDriverName.latin1(), sParam.latin1(), pDepends)) == NULL)
			appendMessagesClient("lscp_get_audio_driver_param_info");
		break;
	case qsamplerDevice::Midi:
		if ((pParamInfo = ::lscp_get_midi_driver_param_info(pMainForm->client(),
				m_sDriverName.latin1(), sParam.latin1(), pDepends)) == NULL)
			appendMessagesClient("lscp_get_midi_driver_param_info");
		break;
	case qsamplerDevice::None:
		break;
	}
	if (pParamInfo) {
		if (param.value != QString::null)
			param = qsamplerDeviceParam(pParamInfo, param.value);
		else
			param = qsamplerDeviceParam(pParamInfo, NULL);
		iRefresh++;
	}

	// Free used parameter array.
	delete pDepends;

	// Return whether the parameters has been changed...
	return iRefresh;
}


// Redirected messages output methods.
void qsamplerDevice::appendMessages( const QString& s ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessages(deviceName() + ' ' + s);
}

void qsamplerDevice::appendMessagesColor( const QString& s,
	const QString& c ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessagesColor(deviceName() + ' ' + s, c);
}

void qsamplerDevice::appendMessagesText( const QString& s ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessagesText(deviceName() + ' ' + s);
}

void qsamplerDevice::appendMessagesError( const QString& s ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessagesError(deviceName() + "\n\n" + s);
}

void qsamplerDevice::appendMessagesClient( const QString& s ) const
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->appendMessagesClient(deviceName() + ' ' + s);
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
		ppszDrivers = ::lscp_list_available_audio_drivers(pClient);
		break;
	case qsamplerDevice::Midi:
		ppszDrivers = ::lscp_list_available_midi_drivers(pClient);
		break;
	case qsamplerDevice::None:
		break;
	}

	for (int iDriver = 0; ppszDrivers && ppszDrivers[iDriver]; iDriver++)
		drivers.append(ppszDrivers[iDriver]);

	return drivers;
}


//-------------------------------------------------------------------------
// qsamplerDevicePort - MIDI/Audio Device port/channel structure.
//

// Constructor.
qsamplerDevicePort::qsamplerDevicePort ( qsamplerDevice& device,
	int iPortID ) : m_device(device)
{
	setDevicePort(iPortID);
}

// Default destructor.
qsamplerDevicePort::~qsamplerDevicePort (void)
{
}


// Initializer.
void qsamplerDevicePort::setDevicePort ( int iPortID )
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
	case qsamplerDevice::Audio:
		if ((pPortInfo = ::lscp_get_audio_channel_info(pMainForm->client(),
				m_device.deviceID(), m_iPortID)) == NULL)
			m_device.appendMessagesClient("lscp_get_audio_channel_info");
		break;
	case qsamplerDevice::Midi:
		if ((pPortInfo = ::lscp_get_midi_port_info(pMainForm->client(),
				m_device.deviceID(), m_iPortID)) == NULL)
			m_device.appendMessagesClient("lscp_get_midi_port_info");
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
	m_sPortName = pPortInfo->name;

	// Grab device port/channel parameters...
	m_params.clear();
	for (int i = 0; pPortInfo->params && pPortInfo->params[i].key; i++) {
		const QString sParam = pPortInfo->params[i].key;
		lscp_param_info_t *pParamInfo = NULL;
		switch (m_device.deviceType()) {
		case qsamplerDevice::Audio:
			if ((pParamInfo = ::lscp_get_audio_channel_param_info(
					pMainForm->client(), m_device.deviceID(),
					m_iPortID, sParam.latin1())) == NULL)
				m_device.appendMessagesClient("lscp_get_audio_channel_param_info");
			break;
		case qsamplerDevice::Midi:
			if ((pParamInfo = ::lscp_get_midi_port_param_info(
					pMainForm->client(), m_device.deviceID(),
					m_iPortID, sParam.latin1())) == NULL)
				m_device.appendMessagesClient("lscp_get_midi_port_param_info");
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
bool qsamplerDevicePort::setParam ( const QString& sParam,
	const QString& sValue )
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	// Set proper port/channel parameter.
	m_params[sParam.upper()].value = sValue;

	// If the device already exists, things get immediate...
	int iRefresh = 0;
	if (m_device.deviceID() >= 0 && m_iPortID >= 0) {
		// Prepare parameter struct.
		lscp_param_t param;
		param.key   = (char *) sParam.latin1();
		param.value = (char *) sValue.latin1();
		// Now it depends on the device type...
		lscp_status_t ret = LSCP_FAILED;
		switch (m_device.deviceType()) {
		case qsamplerDevice::Audio:
			if ((ret = ::lscp_set_audio_channel_param(pMainForm->client(),
					m_device.deviceID(), m_iPortID, &param)) != LSCP_OK)
				m_device.appendMessagesClient("lscp_set_audio_channel_param");
			break;
		case qsamplerDevice::Midi:
			if ((ret = ::lscp_set_midi_port_param(pMainForm->client(),
					m_device.deviceID(), m_iPortID, &param)) != LSCP_OK)
				m_device.appendMessagesClient("lscp_set_midi_port_param");
			break;
		case qsamplerDevice::None:
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
// qsamplerDeviceItem - QTreeWidget device item.
//

// Constructors.
qsamplerDeviceItem::qsamplerDeviceItem ( QTreeWidget* pTreeWidget,
	qsamplerDevice::qsamplerDeviceType deviceType )
	: QTreeWidgetItem(pTreeWidget, QSAMPLER_DEVICE_ITEM),
      m_device(deviceType)
{
	switch(m_device.deviceType()) {
	case qsamplerDevice::Audio:
		setIcon(0, QPixmap(":/icons/audio1.png"));
		setText(0, QObject::tr("Audio Devices"));
		break;
	case qsamplerDevice::Midi:
		setIcon(0, QPixmap(":/icons/midi1.png"));
		setText(0, QObject::tr("MIDI Devices"));
		break;
	case qsamplerDevice::None:
		break;
	}
}

qsamplerDeviceItem::qsamplerDeviceItem ( QTreeWidgetItem* pItem,
	qsamplerDevice::qsamplerDeviceType deviceType,
	int iDeviceID )
	: QTreeWidgetItem(pItem, QSAMPLER_DEVICE_ITEM),
      m_device(deviceType, iDeviceID)
{
	switch(m_device.deviceType()) {
	case qsamplerDevice::Audio:
		setIcon(0, QPixmap(":/icons/audio2.png"));
		break;
	case qsamplerDevice::Midi:
		setIcon(0, QPixmap(":/icons/midi2.png"));
		break;
	case qsamplerDevice::None:
		break;
	}

	setText(0, m_device.deviceName());
}

// Default destructor.
qsamplerDeviceItem::~qsamplerDeviceItem ()
{
}

// Instance accessors.
qsamplerDevice& qsamplerDeviceItem::device ()
{
	return m_device;
}


//-------------------------------------------------------------------------
// AbstractDeviceParamModel - data model base class for device parameters
//

AbstractDeviceParamModel::AbstractDeviceParamModel(QObject* parent) : QAbstractTableModel(parent), bEditable(false) {
    params = NULL;
}

int AbstractDeviceParamModel::rowCount(const QModelIndex& /*parent*/) const {
    //std::cout << "model size=" << params.size() << "\n" << std::flush;
    return (params) ? params->size() : 0;
}

int AbstractDeviceParamModel::columnCount(const QModelIndex& /*parent*/) const {
    return 3;
}

Qt::ItemFlags AbstractDeviceParamModel::flags(const QModelIndex& /*index*/) const {
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

QVariant AbstractDeviceParamModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        //std::cout << "inavlid device model index\n" << std::flush;
        return QVariant();
    }
    if (role != Qt::DisplayRole) {
        //std::cout << "inavlid display role\n" << std::flush;
        return QVariant();
    }

    DeviceParameterRow item;
    item.name  = params->keys()[index.row()];
    item.param = (*params)[item.name];

    //std::cout << "item["<<index.row()<<"]=[" << item.name.toLatin1().data() << "]\n" << std::flush;

    return QVariant::fromValue(item);
}

QVariant AbstractDeviceParamModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return QVariant();

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

void AbstractDeviceParamModel::refresh(const qsamplerDeviceParamMap* params, bool bEditable) {
    this->params    = params;
    this->bEditable = bEditable;
    // inform the outer world (QTableView) that our data changed
    QAbstractTableModel::reset();
}

void AbstractDeviceParamModel::clear() {
    params = NULL;
    // inform the outer world (QTableView) that our data changed
    QAbstractTableModel::reset();
}


//-------------------------------------------------------------------------
// DeviceParamModel - data model for device parameters (used for QTableView)
//

DeviceParamModel::DeviceParamModel(QObject* parent) : AbstractDeviceParamModel(parent) {
    device = NULL;
}

bool DeviceParamModel::setData(const QModelIndex& index, const QVariant& value, int /*role*/) {
    if (!index.isValid()) {
        return false;
    }
    QString key = params->keys()[index.row()];
    //params[key].value = value.toString();
    device->setParam(key, value.toString());
    emit dataChanged(index, index);
    return true;
}

void DeviceParamModel::refresh(qsamplerDevice* pDevice, bool bEditable) {
    device = pDevice;
    AbstractDeviceParamModel::refresh(&pDevice->params(), bEditable);
}

void DeviceParamModel::clear() {
    AbstractDeviceParamModel::clear();
    device = NULL;
}


//-------------------------------------------------------------------------
// PortParamModel - data model for port parameters (used for QTableView)
//

PortParamModel::PortParamModel(QObject* parent) : AbstractDeviceParamModel(parent) {
    port = NULL;
}

bool PortParamModel::setData(const QModelIndex& index, const QVariant& value, int /*role*/) {
    if (!index.isValid()) {
        return false;
    }
    QString key = params->keys()[index.row()];
    //params[key].value = value.toString();
    port->setParam(key, value.toString());
    emit dataChanged(index, index);
    return true;
}

void PortParamModel::refresh(qsamplerDevicePort* pPort, bool bEditable) {
    port = pPort;
    AbstractDeviceParamModel::refresh(&pPort->params(), bEditable);
}

void PortParamModel::clear() {
    AbstractDeviceParamModel::clear();
    port = NULL;
}


//-------------------------------------------------------------------------
// DeviceParamDelegate - table cell renderer for device/port parameters
//

DeviceParamDelegate::DeviceParamDelegate(QObject *parent) : QItemDelegate(parent) {
}

QWidget* DeviceParamDelegate::createEditor(QWidget *parent,
	const QStyleOptionViewItem &/* option */,
	const QModelIndex& index) const
{
    if (!index.isValid()) {
        return NULL;
    }

    DeviceParameterRow r = index.model()->data(index, Qt::DisplayRole).value<DeviceParameterRow>();

    const bool bEnabled = (/*index.model()->bEditable ||*/ !r.param.fix);

    switch (index.column()) {
        case 0:
            return new QLabel(r.name, parent);
        case 1: {
            if (r.param.type == LSCP_TYPE_BOOL) {
                QCheckBox* pCheckBox = new QCheckBox(parent);
                pCheckBox->setChecked(r.param.value.lower() == "true");
                pCheckBox->setEnabled(bEnabled);
                return pCheckBox;
            } else if (r.param.possibilities.count() > 0) {
                QStringList opts = r.param.possibilities;
                if (r.param.multiplicity)
                    opts.prepend(tr("(none)"));
                QComboBox* pComboBox = new QComboBox(parent);
                pComboBox->addItems(opts);
                if (r.param.value.isEmpty())
                    pComboBox->setCurrentIndex(0);
                else
                    pComboBox->setCurrentIndex(pComboBox->findText(r.param.value));
                pComboBox->setEnabled(bEnabled);
                return pComboBox;
            } else if (r.param.type == LSCP_TYPE_INT
                       && !r.param.range_min.isEmpty()
                       && !r.param.range_max.isEmpty()) {
                QSpinBox* pSpinBox = new QSpinBox(parent);
                pSpinBox->setValue(r.param.value.toInt());
                pSpinBox->setMinimum(r.param.range_min.toInt());
                pSpinBox->setMaximum(r.param.range_max.toInt());
                pSpinBox->setEnabled(bEnabled);
                return pSpinBox;
            } else {
                QLineEdit* pLineEdit = new QLineEdit(r.param.value, parent);
                pLineEdit->setEnabled(bEnabled);
                return pLineEdit;
            }
        }
        case 2:
            return new QLabel(r.param.description, parent);
        default:
            return NULL;
    }
}

void DeviceParamDelegate::setEditorData(QWidget* /*editor*/, const QModelIndex& /*index*/) const {
    // unused, since we set the editor data already in createEditor()
}

void DeviceParamDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    if (index.column() == 1) {
        DeviceParameterRow r = index.model()->data(index, Qt::DisplayRole).value<DeviceParameterRow>();
        if (r.param.type == LSCP_TYPE_BOOL) {
            QCheckBox* pCheckBox = static_cast<QCheckBox*>(editor);
            model->setData(index, QVariant(pCheckBox->checkState() == Qt::Checked));
        } else if (r.param.possibilities.count() > 0) {
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            model->setData(index, pComboBox->currentText());
        } else if (r.param.type == LSCP_TYPE_INT) {
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            model->setData(index, pSpinBox->value());
        } else {
            QLineEdit* pLineEdit = static_cast<QLineEdit*>(editor);
            model->setData(index, pLineEdit->text());
        }
    }
}

void DeviceParamDelegate::updateEditorGeometry(QWidget* editor,
	const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    if (editor) editor->setGeometry(option.rect);
}

// end of qsamplerDevice.cpp
