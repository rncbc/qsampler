// qsamplerDevice.h
//
/****************************************************************************
   Copyright (C) 2003-2005, rncbc aka Rui Nuno Capela. All rights reserved.

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


// Special QListViewItem::rtti() return values.
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
    enum qsamplerDeviceType { Midi, Audio };

    // Constructor.
    qsamplerDevice(lscp_client_t *pClient,
		qsamplerDeviceType deviceType, int iDeviceID = -1);
    // Default destructor.
    ~qsamplerDevice();

	// Initializer.
	void setDevice(lscp_client_t *pClient,
		qsamplerDeviceType deviceType, int iDeviceID = -1);
		
	// Device property accessors.
    int                 deviceID()   const;
    qsamplerDeviceType  deviceType() const;
    const QString&      driverName() const;
    const QString&      deviceName() const;

	// Device parameters accessor.
	qsamplerDeviceParamMap& params();

	// Update/refresh device/driver data.
	void refresh();

	// Device enumerator.
    static int *getDevices(lscp_client_t *pClient,
		qsamplerDeviceType deviceType);

private:

	// Instance variables.
    int                m_iDeviceID;
    qsamplerDeviceType m_deviceType;
    QString            m_sDriverName;
    QString            m_sDeviceName;

	// Device parameter list.
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
    const qsamplerDevice& device();

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

    // Client/device descriptor selector.
	void setDevice(lscp_client_t *pClient,
		qsamplerDevice::qsamplerDeviceType deviceType, int iDeviceID = -1);

    // Client/device descriptor accessors.
	lscp_client_t *client();
	int deviceID();

	// The main table refresher.
	void refresh();
	
private:

    // LSCP client/device references.
    lscp_client_t *m_pClient;
    qsamplerDevice::qsamplerDeviceType m_deviceType;
    int m_iDeviceID;
};


#endif  // __qsamplerDevice_h


// end of qsamplerDevice.h
