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

// A typedef'd parameter QMap.
typedef QMap<QString, qsamplerDeviceParam> qsamplerDeviceParamMap;


//-------------------------------------------------------------------------
// qsamplerDevice - MIDI/Audio Device structure.
//

class qsamplerDevice
{
public:

	// We use the same class for MIDI and audio device management
	enum qsamplerDeviceType { None, Midi, Audio };

	// Constructor.
	qsamplerDevice(lscp_client_t *pClient,
		qsamplerDeviceType deviceType, int iDeviceID = -1);
	// Default destructor.
	~qsamplerDevice();

	// Initializer.
	void setDevice(lscp_client_t *pClient,
		qsamplerDeviceType deviceType, int iDeviceID = -1);

	// Driver name initializer.
	void setDriver(lscp_client_t *pClient,
		const QString& sDriverName);

	// Device property accessors.
	int                 deviceID()   const;
	qsamplerDeviceType  deviceType() const;
	const QString&      deviceTypeName() const;
	const QString&      driverName() const;
	const QString&      deviceName() const;

	// Device parameters accessor.
	const qsamplerDeviceParamMap& params() const;

	// Set the proper device parameter value.
	void setParam (const QString& sParam, const QString& sValue);

	// Device ids enumerator.
	static int *getDevices(lscp_client_t *pClient,
		qsamplerDeviceType deviceType);

	// Driver names enumerator.
	static QStringList getDrivers(lscp_client_t *pClient,
		qsamplerDeviceType deviceType);

private:

	// Instance variables.
	int                m_iDeviceID;
	qsamplerDeviceType m_deviceType;
	QString            m_sDeviceType;
	QString            m_sDriverName;
	QString            m_sDeviceName;

	// Device parameter list.
	qsamplerDeviceParamMap m_params;
};


//-------------------------------------------------------------------------
// qsamplerDevicePort - MIDI/Audio Device port/channel structure.
//

class qsamplerDevicePort
{
public:

	// Constructor.
	qsamplerDevicePort(lscp_client_t *pClient,
		const qsamplerDevice& device, int iPortID);
	// Default destructor.
	~qsamplerDevicePort();

	// Initializer.
	void setDevicePort(lscp_client_t *pClient,
		const qsamplerDevice& device, int iPortID);

	// Device port property accessors.
	int            portID()   const;
	const QString& portName() const;

	// Device port parameters accessor.
	const qsamplerDeviceParamMap& params() const;

	// Set the proper device port/channel parameter value.
	void setParam (const QString& sParam, const QString& sValue);

private:

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
	qsamplerDeviceItem(QListView *pListView, lscp_client_t *pClient,
		qsamplerDevice::qsamplerDeviceType deviceType);
	qsamplerDeviceItem(QListViewItem *pItem, lscp_client_t *pClient,
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
