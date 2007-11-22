// qsamplerDeviceForm.h
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

#ifndef __qsamplerDeviceForm_h
#define __qsamplerDeviceForm_h

#include "ui_qsamplerDeviceForm.h"

#include "qsamplerDevice.h"


namespace QSampler {

class MainForm;

class DeviceForm : public QDialog
{
	Q_OBJECT

public:

    DeviceForm(QWidget *pParent = NULL, Qt::WindowFlags wflags = 0);
   ~DeviceForm();

    void setDeviceTypeMode(qsamplerDevice::DeviceType deviceType);
    void setDriverName(const QString& sDriverName);
    void setDevice(qsamplerDevice* pDevice);

public slots:

    void createDevice();
    void deleteDevice();
    void refreshDevices();
    void selectDriver(const QString& sDriverName);
    void selectDevice();
    void selectDevicePort(int iPort);
    void changeDeviceParam(int iRow, int iCol);
    void changeDevicePortParam(int iRow, int iCol);
    void deviceListViewContextMenu(const QPoint& pos);
    void stabilizeForm();
    void updateCellRenderers();
    void updateCellRenderers(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void updatePortCellRenderers();
    void updatePortCellRenderers(const QModelIndex& topLeft, const QModelIndex& bottomRight);

signals:

    void devicesChanged();

protected:

    void showEvent(QShowEvent* pShowEvent);
    void hideEvent(QHideEvent* pHideEvent);

private:

    Ui::qsamplerDeviceForm m_ui;

    DeviceParamModel    m_deviceParamModel;
    DeviceParamDelegate m_deviceParamDelegate;

    PortParamModel      m_devicePortParamModel;
    DeviceParamDelegate m_devicePortParamDelegate;

    lscp_client_t *m_pClient;
    int m_iDirtySetup;
    int m_iDirtyCount;
    bool m_bNewDevice;
    qsamplerDevice::DeviceType m_deviceType;
    qsamplerDevice::DeviceType m_deviceTypeMode;
    qsamplerDeviceItem *m_pAudioItems;
    qsamplerDeviceItem *m_pMidiItems;
};

} // namespace QSampler

#endif // __qsamplerDeviceForm_h


// end of qsamplerDeviceForm.h
