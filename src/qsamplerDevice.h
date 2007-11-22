// qsamplerDevice.h
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

#ifndef __qsamplerDevice_h
#define __qsamplerDevice_h

#include <QListWidget>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QFontMetrics>
#include <QModelIndex>
#include <QSize>
#include <QList>

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
typedef QList<qsamplerDevicePort *> qsamplerDevicePortList;


//-------------------------------------------------------------------------
// qsamplerDevice - MIDI/Audio Device structure.
//

class qsamplerDevice
{
public:

	// We use the same class for MIDI and audio device management
	enum DeviceType { None, Midi, Audio };

	// Constructor.
	qsamplerDevice(DeviceType deviceType, int iDeviceID = -1);
	// Copy constructor.
	qsamplerDevice(const qsamplerDevice& device);
	// Default destructor.
	~qsamplerDevice();

	// Initializer.
	void setDevice(DeviceType deviceType, int iDeviceID = -1);

	// Driver name initializer.
	void setDriver(const QString& sDriverName);

	// Device property accessors.
	int            deviceID() const;
	DeviceType     deviceType() const;
	const QString& deviceTypeName() const;
	const QString& driverName() const;

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
		DeviceType deviceType);

	// Driver names enumerator.
	static QStringList getDrivers(lscp_client_t *pClient,
		DeviceType deviceType);

private:

	// Refresh/set given parameter based on driver supplied dependencies.
	int refreshParam(const QString& sParam);

	// Instance variables.
	int        m_iDeviceID;
	DeviceType m_deviceType;
	QString    m_sDeviceType;
	QString    m_sDriverName;
	QString    m_sDeviceName;

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

class qsamplerDeviceItem : public QTreeWidgetItem
{
public:

	// Constructors.
	qsamplerDeviceItem(QTreeWidget *pTreeWidget,
		qsamplerDevice::DeviceType deviceType);
	qsamplerDeviceItem(QTreeWidgetItem *pItem,
		qsamplerDevice::DeviceType deviceType, int iDeviceID);
	// Default destructor.
	~qsamplerDeviceItem();

	// Instance accessors.
	qsamplerDevice& device();

private:

	// Instance variables.
	qsamplerDevice m_device;
};

struct DeviceParameterRow {
	QString             name;
	qsamplerDeviceParam param;
	bool                alive; // whether these params refer to an existing device or for a device that is yet to be created
};

// so we can use it i.e. through QVariant
Q_DECLARE_METATYPE(DeviceParameterRow)


//-------------------------------------------------------------------------
// AbstractDeviceParamModel - data model base class for device parameters
//
class AbstractDeviceParamModel : public QAbstractTableModel
{
	Q_OBJECT

public:

	AbstractDeviceParamModel(QObject *pParent = NULL);

	// Overridden methods from subclass(es)
	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex() ) const;
	QVariant headerData(int section,
		Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex& index) const;

	virtual void clear();

	void refresh(const qsamplerDeviceParamMap* params, bool bEditable);

protected:

	const qsamplerDeviceParamMap *m_params;
	bool m_bEditable;
};


//-------------------------------------------------------------------------
// DeviceParamModel - data model for device parameters (used for QTableView)
//

class DeviceParamModel : public AbstractDeviceParamModel
{
	Q_OBJECT

public:

	DeviceParamModel(QObject *pParent = NULL);

	// Overridden methods from subclass(es)
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex& index,
		const QVariant& value, int role = Qt::EditRole);

	void clear();

public slots:

	void refresh(qsamplerDevice* pDevice, bool bEditable);

private:

	qsamplerDevice *m_device;
};


//-------------------------------------------------------------------------
// PortParamModel - data model for port parameters (used for QTableView)
//

class PortParamModel : public AbstractDeviceParamModel
{
	Q_OBJECT

public:

	PortParamModel(QObject *pParent = 0);

	// overridden methods from subclass(es)
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex& index,
		const QVariant& value, int role = Qt::EditRole);

	void clear();

public slots:

	void refresh(qsamplerDevicePort* pPort, bool bEditable);

private:

	qsamplerDevicePort* m_port;
};


//-------------------------------------------------------------------------
// DeviceParamDelegate - table cell renderer for device/port parameters
//
class DeviceParamDelegate : public QItemDelegate
{
	Q_OBJECT

public:

	DeviceParamDelegate(QObject *pParent = NULL);

	QWidget* createEditor(QWidget *pParent,
		const QStyleOptionViewItem& option, const QModelIndex& index) const;
	void setEditorData(QWidget *pEditor, const QModelIndex& index) const;
	void setModelData(QWidget *pEditor, QAbstractItemModel* model,
		const QModelIndex& index) const;
	void updateEditorGeometry(QWidget* pEditor,
		const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

#endif  // __qsamplerDevice_h

// end of qsamplerDevice.h
