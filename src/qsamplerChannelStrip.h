#ifndef QSAMPLER_CHANNEL_STRIP_H
#define QSAMPLER_CHANNEL_STRIP_H

#include "ui_qsamplerChannelStrip.h"

#include "qsamplerChannel.h"

#include <QDragEnterEvent>

namespace QSampler {

class ChannelStrip : public QWidget {
Q_OBJECT
public:
    ChannelStrip(QWidget* parent = 0, Qt::WFlags f = 0);
   ~ChannelStrip();

    void setup(qsamplerChannel* pChannel);
    qsamplerChannel* channel();
    QFont displayFont();
    void setDisplayFont(const QFont& font);
    void setDisplayEffect(bool bDisplayEffect);
    void setDisplayBackground(const QPixmap& pm);
    void setMaxVolume(int iMaxVolume);
    bool updateInstrumentName(bool bForce);
    bool updateChannelVolume();
    bool updateChannelInfo();
    bool updateChannelUsage();
    void resetErrorCount();

signals:
    void channelChanged(ChannelStrip*);

public slots:
    bool channelSetup();
    bool channelMute(bool bMute);
    bool channelSolo(bool bSolo);
    void channelEdit();
    bool channelReset();
    void volumeChanged(int iVolume);

protected:
    bool decodeDragFile(const QMimeSource* pEvent, QString& sInstrumentFile);
    void dragEnterEvent(QDragEnterEvent* pDragEnterEvent);
    void dropEvent(QDropEvent* pDropEvent);
    void contextMenuEvent(QContextMenuEvent* pEvent);

private:
    Ui::qsamplerChannelStrip ui;

    qsamplerChannel* m_pChannel;
    int m_iDirtyChange;
    int m_iErrorCount;
};

} // namespace QSampler

#endif // QSAMPLER_CHANNEL_STRIP_H
