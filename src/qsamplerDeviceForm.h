#ifndef QSAMPLER_DEVICE_FORM_H
#define QSAMPLER_DEVICE_FORM_H

#include "ui_qsamplerDeviceForm.h"

#include "qsamplerMainForm.h"

namespace QSampler {

class MainForm;

class DeviceForm : public QDialog {
Q_OBJECT
public:
    DeviceForm(QWidget* parent = 0, Qt::WFlags f = 0);
   ~DeviceForm();
    void showEvent(QShowEvent* pShowEvent);

    void setDeviceTypeMode(qsamplerDevice::qsamplerDeviceType deviceType);
    void setDriverName(const QString& sDriverName);
    void setDevice(qsamplerDevice* pDevice);

signals:
    void devicesChanged();

public slots:
    void createDevice();
    void deleteDevice();
    void refreshDevices();
    void selectDriver(const QString& sDriverName);
    void selectDevice();
    void selectDevicePort(int iPort);
    void changeDeviceParam(int iRow, int iCol);
    void changeDevicePortParam(int iRow, int iCol);
    void contextMenu(QTreeWidgetItem* item, const QPoint&, int);
    void stabilizeForm();

protected:
    MainForm *m_pMainForm;
    lscp_client_t *m_pClient;
    int m_iDirtySetup;
    int m_iDirtyCount;
    bool m_bNewDevice;
    qsamplerDevice::qsamplerDeviceType m_deviceType;
    qsamplerDevice::qsamplerDeviceType m_deviceTypeMode;
    qsamplerDeviceItem *m_pAudioItems;
    qsamplerDeviceItem *m_pMidiItems;

    void hideEvent(QHideEvent* pHideEvent);

private:
    Ui::qsamplerDeviceForm ui;

    DeviceParamModel    deviceParamModel;
    DeviceParamDelegate deviceParamDelegate;

    DeviceParamModel    devicePortParamModel;
    DeviceParamDelegate devicePortParamDelegate;
};

} // namespace QSampler

#endif // QSAMPLER_DEVICE_FORM_H
