// qsamplerDevice.h
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

#ifndef __qsamplerDevice_h
#define __qsamplerDevice_h

#include <qlistview.h>
#include <qtable.h>

#include <lscp/client.h>
#include <lscp/device.h>

#include "qsamplerOptions.h"


// Special QListViewItem::rtti() unique return value.
#define	QSAMPLER_DEVICE_ITEM    1001

// Early forward declarations.
class qsamplerMainForm;
class qsamplerDevicePort;


//-------------------------------------------------------------------------
// qsamplerDeviceParam - MIDI/Audio Device parameter structure.
//
class qsamplerDeviceParam
{
public:

	// Constructor.
	qsamplerDeviceParam(lscp_param_info_t *pParamInfo = NULL,
		const char *pszValue = NULL);
	// Default destructor.
	~qsamplerDeviceParam();

	// Initializer.
	void setParam(lscp_param_info_t *pParamInfo,
		const char *pszValue = NULL);

	// Info structure field members.
	lscp_type_t	type;
	QString 	description;
	bool    	mandatory;
	bool    	fix;
	bool    	multiplicity;
	QStringList depends;
	QString 	defaultv;
	QString 	range_min;
	QString 	range_max;
	QStringList possibilities;
	// The current parameter value.
	QString 	value;
};

// Typedef'd parameter QMap.
typedef QMap<QString, qsamplerDeviceParam> qsamplerDeviceParamMap;

// Typedef'd device port/channels QptrList.
typedef QPtrList<qsamplerDevicePort> qsamplerDevicePortList;


//-------------------------------------------------------------------------
// qsamplerDevice - MIDI/Audio Device structure.
//

class qsamplerDevice
{
public:

	// We use the same class for MIDI and audio device management
	enum qsamplerDeviceType { None, Midi, Audio };

	// Constructor.
	qsamplerDevice(qsamplerMainForm *pMainForm,
		qsamplerDeviceType deviceType, int iDeviceID = -1);
	// Copy constructor.
    qsamplerDevice(const qsamplerDevice& device);
	// Default destructor.
	~qsamplerDevice();

	// Initializer.
	void setDevice(qsamplerDeviceType deviceType, int iDeviceID = -1);

	// Driver name initializer.
	void setDriver(const QString& sDriverName);

	// LSCP client descriptor accessor.
	lscp_client_t *client() const;

	// Device property accessors.
	int                 deviceID()   const;
	qsamplerDeviceType  deviceType() const;
	const QString&      deviceTypeName() const;
	const QString&      driverName() const;
	// Special device name formatter.
	QString deviceName() const;

	// Set the proper device parameter value.
	bool setParam (const QString& sParam, const QString& sValue);

	// Device parameters accessor.
	const qsamplerDeviceParamMap& params() const;

	// Device port/channel list accessor.
	qsamplerDevicePortList& ports();

	// Device parameter dependency list refreshner.
	int refreshParams();
	// Device port/channel list refreshner.
	int refreshPorts();
	// Refresh/set dependencies given that some parameter has changed.
	int refreshDepends(const QString& sParam);

	// Create/destroy device methods.
	bool createDevice();
	bool deleteDevice();

	// Message logging methods (brainlessly mapped to main form's).
	void appendMessages       (const QString& s) const;
	void appendMessagesColor  (const QString& s, const QString & c) const;
	void appendMessagesText   (const QString& s) const;
	void appendMessagesError  (const QString& s) const;
	void appendMessagesClient (const QString& s) const;

	// Device ids enumerator.
	static int *getDevices(lscp_client_t *pClient,
		qsamplerDeviceType deviceType);

	// Driver names enumerator.
	static QStringList getDrivers(lscp_client_t *pClient,
		qsamplerDeviceType deviceType);

private:

	// Refresh/set given parameter based on driver supplied dependencies.
	int refreshParam(const QString& sParam);

	// Main application form reference.
	qsamplerMainForm  *m_pMainForm;

	// Instance variables.
	int                m_iDeviceID;
	qsamplerDeviceType m_deviceType;
	QString            m_sDeviceType;
	QString            m_sDriverName;
	QString            m_sDeviceName;

	// Device parameter list.
	qsamplerDeviceParamMap m_params;

	// Device port/channel list.
	qsamplerDevicePortList m_ports;
};


//-------------------------------------------------------------------------
// qsamplerDevicePort - MIDI/Audio Device port/channel structure.
//

class qsamplerDevicePort
{
public:

	// Constructor.
	qsamplerDevicePort(qsamplerDevice& device, int iPortID);
	// Default destructor.
	~qsamplerDevicePort();

	// Initializer.
	void setDevicePort(int iPortID);

	// Device port property accessors.
	int            portID()   const;
	const QString& portName() const;

	// Device port parameters accessor.
	const qsamplerDeviceParamMap& params() const;

	// Set the proper device port/channel parameter value.
	bool setParam (const QString& sParam, const QString& sValue);

private:

	// Device reference.
	qsamplerDevice& m_device;

	// Instance variables.
	int     m_iPortID;
	QString m_sPortName;

	// Device port parameter list.
	qsamplerDeviceParamMap m_params;
};


//-------------------------------------------------------------------------
// qsamplerDeviceItem - QListView device item.
//

class qsamplerDeviceItem : public QListViewItem
{
public:

	// Constructors.
	qsamplerDeviceItem(QListView *pListView, qsamplerMainForm *pMainForm,
		qsamplerDevice::qsamplerDeviceType deviceType);
	qsamplerDeviceItem(QListViewItem *pItem, qsamplerMainForm *pMainForm,
		qsamplerDevice::qsamplerDeviceType deviceType, int iDeviceID);
	// Default destructor.
	~qsamplerDeviceItem();

	// Instance accessors.
	qsamplerDevice& device();

	// To virtually distinguish between list view items.
	virtual int rtti() const;

private:

	// Instance variables.
	qsamplerDevice m_device;
};


//-------------------------------------------------------------------------
// qsamplerDeviceParamTable - Device parameter view table.
//

class qsamplerDeviceParamTable : public QTable
{
	Q_OBJECT

public:

	// Constructor.
	qsamplerDeviceParamTable(QWidget *pParent = 0, const char *pszName = 0);
	// Default destructor.
	~qsamplerDeviceParamTable();

	// Common parameter table renderer.
	void refresh(const qsamplerDeviceParamMap& params, bool bEditable);
};


//-------------------------------------------------------------------------
// qsamplerDeviceParamTableSpinBox - Custom spin box for parameter table.
//

class qsamplerDeviceParamTableSpinBox : public QTableItem
{
public:

	// Constructor.
	qsamplerDeviceParamTableSpinBox (QTable *pTable, EditType editType,
		const QString& sText);

	// Public accessors.
	void setMinValue(int iMinValue);
	void setMaxValue(int iMaxValue);
	void setValue(int iValue);

protected:

	// Virtual implemetations.
	QWidget *createEditor() const;
	void setContentFromEditor(QWidget *pWidget);

private:

	// Initial value holders.
	int m_iValue;
	int m_iMinValue;
	int m_iMaxValue;
};


//-------------------------------------------------------------------------
// qsamplerDeviceParamTableEditBox - Custom edit box for parameter table.
//

class qsamplerDeviceParamTableEditBox : public QTableItem
{
public:

	// Constructor.
	qsamplerDeviceParamTableEditBox (QTable *pTable, EditType editType,
		const QString& sText);

protected:

	// Virtual implemetations.
	QWidget *createEditor() const;
	void setContentFromEditor(QWidget *pWidget);
};


#endif  // __qsamplerDevice_h


// end of qsamplerDevice.h
