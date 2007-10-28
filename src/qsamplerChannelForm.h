#ifndef QSAMPLER_CHANNEL_FORM_H
#define QSAMPLER_CHANNEL_FORM_H

#include "ui_qsamplerChannelForm.h"

#include "qsamplerDevice.h"
#include "qsamplerChannel.h"
#include "qsamplerDeviceForm.h"

#include <Q3PtrList>

namespace QSampler {

class ChannelForm : public QDialog {
Q_OBJECT
public:
    ChannelForm(QWidget* parent = 0);
   ~ChannelForm();

    void setup(qsamplerChannel* pChannel);
    void setupDevice(qsamplerDevice* pDevice, qsamplerDevice::qsamplerDeviceType deviceTypeMode, const QString& sDriverName);
    void selectMidiDriverItem(const QString& sMidiDriver);
    void selectMidiDeviceItem(int iMidiItem);
    void selectAudioDriverItem(const QString& sAudioDriver);
    void selectAudioDeviceItem(int iAudioItem);

protected:
    qsamplerChannel* m_pChannel;
    int m_iDirtySetup;
    int m_iDirtyCount;
    Q3PtrList<qsamplerDevice> m_audioDevices;
    Q3PtrList<qsamplerDevice> m_midiDevices;
    DeviceForm* m_pDeviceForm;
    qsamplerChannelRoutingMap m_audioRouting;
    ChannelRoutingModel routingModel;
    ChannelRoutingDelegate routingDelegate;

protected slots:
    void accept();
    void reject();
    void openInstrumentFile();
    void updateInstrumentName();
    void selectMidiDriver(const QString& sMidiDriver);
    void selectMidiDevice(int iMidiItem);
    void setupMidiDevice();
    void selectAudioDriver(const QString& sAudioDriver);
    void selectAudioDevice(int iAudioItem);
    void setupAudioDevice();
    void changeAudioRouting(int iRow, int iCol);
    void updateDevices();
    void optionsChanged();
    void stabilizeForm();

private:
    Ui::qsamplerChannelForm ui;
};

} // namespace QSampler

#endif // QSAMPLER_CHANNEL_FORM_H
