// qsamplerMainForm.h
//
/****************************************************************************
   Copyright (C) 2004-2024, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007-2019 Christian Schoenebeck

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

#include "ui_qsamplerMainForm.h"

#include <lscp/client.h>

class QProcess;
class QMdiSubWindow;
class QSocketNotifier;
class QSpinBox;
class QSlider;
class QLabel;

namespace QSampler {

class Workspace;
class Options;
class Messages;
class Channel;
class ChannelStrip;
class DeviceForm;
class InstrumentListForm;

//-------------------------------------------------------------------------
// QSampler::MainForm -- Main window form implementation.
//

class MainForm : public QMainWindow
{
	Q_OBJECT

public:

	MainForm(QWidget *pParent = nullptr);
	~MainForm();

	void setup(Options *pOptions);

	Options *options() const;
	lscp_client_t *client() const;

	QString sessionName(const QString& sFilename);

	void appendMessages(const QString& s);
	void appendMessagesColor(const QString& s, const QColor& rgb);
	void appendMessagesText(const QString& s);
	void appendMessagesError(const QString& s);
	void appendMessagesClient(const QString& s);

	ChannelStrip *createChannelStrip(Channel *pChannel);
	void destroyChannelStrip(ChannelStrip *pChannelStrip);
	ChannelStrip *activeChannelStrip();
	ChannelStrip *channelStripAt(int iChannel);
	ChannelStrip *channelStrip(int iChannelID);

	void channelsArrangeAuto();
	void contextMenuEvent(QContextMenuEvent *pEvent);
	void sessionDirty();

	static MainForm *getInstance();

public slots:

	void fileNew();
	void fileOpen();
	void fileOpenRecent();
	void fileSave();
	void fileSaveAs();
	void fileReset();
	void fileRestart();
	void fileExit();
	void editAddChannel();
	void editRemoveChannel();
	void editSetupChannel();
	void editEditChannel();
	void editResetChannel();
	void editResetAllChannels();
	void viewMenubar(bool bOn);
	void viewToolbar(bool bOn);
	void viewStatusbar(bool bOn);
	void viewMessages(bool bOn);
	void viewInstruments();
	void viewDevices();
	void viewOptions();
	void channelsArrange();
	void channelsAutoArrange(bool bOn);
	void helpAboutQt();
	void helpAbout();

	void stabilizeForm();

protected slots:

	void updateRecentFilesMenu();

	void volumeChanged(int iVolume);
	void channelStripChanged(ChannelStrip *pChannelStrip);
	void channelsMenuAboutToShow();
	void channelsMenuActivated();
	void timerSlot();
	void readServerStdout();
	void processServerExit();
	void autoReconnectClient();

	void handle_sigusr1();
	void handle_sigterm();

	// Channel strip activation/selection.
	void activateStrip(QMdiSubWindow *pMdiSubWindow);

	// Channel toolbar orientation change.
	void channelsToolbarOrientation(Qt::Orientation orientation);

protected:

	void addChannelStrip();
	void removeChannelStrip();

	bool queryClose();
	void closeEvent(QCloseEvent* pCloseEvent);
	void dragEnterEvent(QDragEnterEvent *pDragEnterEvent);
	void dropEvent(QDropEvent *pDropEvent);
	void customEvent(QEvent *pCustomEvent);
	bool newSession();
	bool openSession();
	bool saveSession(bool bPrompt);
	bool closeSession(bool bForce);
	bool loadSessionFile(const QString& sFilename);
	bool saveSessionFile(const QString& sFilename);
	void updateSession();
	void updateRecentFiles(const QString& sFilename);
	void updateInstrumentNames();
	void updateDisplayFont();
	void updateDisplayEffect();
	void updateMaxVolume();
	void updateMessagesFont();
	void updateMessagesLimit();
	void updateMessagesCapture();
	void updateViewMidiDeviceStatusMenu();
	void updateAllChannelStrips(bool bRemoveDeadStrips);

	void startSchedule(int iStartDelay);
	void stopSchedule();
	void startServer();
	void stopServer(bool bInteractive = false);
	bool startClient(bool bReconnectOnly = false);
	void stopClient();
	void startAutoReconnectClient();

private:

	Ui::qsamplerMainForm m_ui;

	Options *m_pOptions;
	Messages *m_pMessages;
	Workspace *m_pWorkspace;
	QSocketNotifier *m_pSigusr1Notifier;
	QSocketNotifier *m_pSigtermNotifier;
	QString m_sFilename;
	int m_iUntitled;
	int m_iDirtySetup;
	int m_iDirtyCount;
	lscp_client_t *m_pClient;
	QProcess *m_pServer;
	bool m_bForceServerStop;
	int m_iStartDelay;
	int m_iTimerDelay;
	int m_iTimerSlot;
	QLabel *m_statusItem[5];
	QList<ChannelStrip *> m_changedStrips;
	InstrumentListForm *m_pInstrumentListForm;
	DeviceForm *m_pDeviceForm;
	static MainForm *g_pMainForm;
	QSlider *m_pVolumeSlider;
	QSpinBox *m_pVolumeSpinBox;
	int m_iVolumeChanging;
};

} // namespace QSampler

#endif // __qsamplerMainForm_h


// end of qsamplerMainForm.h
