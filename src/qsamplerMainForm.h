// qsamplerMainForm.h
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

#ifndef __qsamplerMainForm_h
#define __qsamplerMainForm_h

#include <Qt3Support>

#include <QWorkspace>
#include <QList>
#include <Q3PtrList>
#include <QProcess>
#include <QMenu>

#include "ui_qsamplerMainForm.h"

#include "qsamplerChannelStrip.h"
#include "qsamplerMessages.h"
#include "qsamplerInstrumentListForm.h"
#include "qsamplerDeviceForm.h"

namespace QSampler {

class DeviceForm;

class MainForm : public QMainWindow {
Q_OBJECT
public:
    MainForm(QWidget* parent = 0);
   ~MainForm();
    void setup(qsamplerOptions* pOptions);
    void contextMenuEvent(QContextMenuEvent *pEvent);
    qsamplerOptions* options(void);
    lscp_client_t* client(void);
    QString sessionName(const QString& sFilename);
    void appendMessages(const QString& s);
    void appendMessagesColor(const QString& s, const QString& c);
    void appendMessagesText(const QString& s);
    void appendMessagesError(const QString& s);
    void appendMessagesClient(const QString& s);
    ChannelStrip* createChannelStrip(qsamplerChannel *pChannel);
    ChannelStrip* activeChannelStrip(void);
    ChannelStrip* channelStripAt(int iChannel);
    ChannelStrip* channelStrip(int iChannelID);
    static MainForm* getInstance(void);

public slots:
    void sessionDirty(void);
    void stabilizeForm(void);

protected:
    bool queryClose(void);
    void closeEvent(QCloseEvent* pCloseEvent);
    bool decodeDragFiles(const QMimeSource* pEvent, QStringList& files);
    void dragEnterEvent(QDragEnterEvent *pDragEnterEvent);
    void dropEvent(QDropEvent *pDropEvent);
    void customEvent(QEvent* pCustomEvent);
    bool newSession(void);
    bool openSession(void);
    bool saveSession(bool bPrompt);
    bool closeSession(bool bForce);
    bool loadSessionFile(const QString& sFilename);
    bool saveSessionFile(const QString& sFilename);
    void updateSession();
    void updateRecentFiles(const QString& sFilename);
    void updateRecentFilesMenu(void);
    void updateInstrumentNames(void);
    void updateDisplayFont(void);
    void updateDisplayEffect(void);
    void updateMaxVolume(void);
    void updateMessagesFont(void);
    void updateMessagesLimit(void);
    void updateMessagesCapture(void);
    void startSchedule(int iStartDelay);
    void stopSchedule(void);
    void startServer(void);
    void stopServer(void);
    bool startClient(void);
    void stopClient(void);

private:
    Ui::qsamplerMainForm ui;

    qsamplerOptions *m_pOptions;
    qsamplerMessages *m_pMessages;
    QWorkspace *m_pWorkspace;
    QString m_sFilename;
    int m_iUntitled;
    int m_iDirtyCount;
    lscp_client_t *m_pClient;
    QProcess *m_pServer;
    int m_iStartDelay;
    int m_iTimerDelay;
    int m_iTimerSlot;
    QLabel *m_statusItem[5];
    QMenu *m_pRecentFilesMenu;
    Q3PtrList<ChannelStrip> m_changedStrips;
    InstrumentListForm *m_pInstrumentListForm;
    DeviceForm *m_pDeviceForm;
    static MainForm *g_pMainForm;
    QSlider *m_pVolumeSlider;
    QSpinBox *m_pVolumeSpinBox;
    int m_iVolumeChanging;
    QToolBar* fileToolbar;
    QToolBar* editToolbar;
    QToolBar* channelsToolbar;

private slots:
    void fileNew(void);
    void fileOpen(void);
    void fileOpenRecent(int iIndex);
    void fileSave(void);
    void fileSaveAs(void);
    void fileReset(void);
    void fileRestart(void);
    void fileExit(void);
    void editAddChannel(void);
    void editRemoveChannel(void);
    void editSetupChannel(void);
    void editEditChannel(void);
    void editResetChannel(void);
    void editResetAllChannels(void);
    void viewMenubar(bool bOn);
    void viewToolbar(bool bOn);
    void viewStatusbar(bool bOn);
    void viewMessages(bool bOn);
    void viewInstruments(void);
    void viewDevices(void);
    void viewOptions(void);
    void channelsArrange(void);
    void channelsAutoArrange(bool bOn);
    void helpAboutQt(void);
    void helpAbout(void);
    void volumeChanged(int iVolume);
    void channelStripChanged(ChannelStrip *pChannelStrip);
    void channelsMenuAboutToShow();
    void channelsMenuActivated(int iChannel);
    void timerSlot(void);
    void readServerStdout(void);
    void processServerExit(void);
};

} // namespace QSampler

#endif // __qsamplerMainForm_h


// end of qsamplerMainForm.h