// qsamplerChannelStrip.h
//
/****************************************************************************
   Copyright (C) 2004-201, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007, 2008, 2014 Christian Schoenebeck

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

#ifndef __qsamplerChannelStrip_h
#define __qsamplerChannelStrip_h

#include "ui_qsamplerChannelStrip.h"

#include "qsamplerChannel.h"

class QDragEnterEvent;
class QTimer;
class QMenu;


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::ChannelStrip -- Channel strip form interface.
//

class ChannelStrip : public QWidget
{
	Q_OBJECT

public:

	ChannelStrip(QWidget* pParent = NULL, Qt::WindowFlags wflags = 0);
	~ChannelStrip();

	void setup(Channel *pChannel);

	Channel *channel() const;

	void setDisplayFont(const QFont& font);
	QFont displayFont() const;

	void setDisplayEffect(bool bDisplayEffect);

	void setMaxVolume(int iMaxVolume);

	bool updateInstrumentName(bool bForce);
	bool updateChannelVolume();
	bool updateChannelInfo();
	bool updateChannelUsage();

	void resetErrorCount();

	// Channel strip activation/selection.
	void setSelected(bool bSelected);
	bool isSelected() const;

signals:

	void channelChanged(ChannelStrip*);

public slots:

	bool channelSetup();
	bool channelMute(bool bMute);
	bool channelSolo(bool bSolo);
	void channelEdit();
	bool channelFxEdit();
	bool channelReset();
	void volumeChanged(int iVolume);

	void midiActivityLedOn();

protected:

	void dragEnterEvent(QDragEnterEvent* pDragEnterEvent);
	void dropEvent(QDropEvent* pDropEvent);
	void contextMenuEvent(QContextMenuEvent* pEvent);

protected slots:

	void midiActivityLedOff();
	void instrumentListPopupItemClicked(QAction* action);

private:

	Ui::qsamplerChannelStrip m_ui;

	Channel *m_pChannel;
	int m_iDirtyChange;
	int m_iErrorCount;
	QMenu* m_instrumentListPopupMenu;

	QTimer  *m_pMidiActivityTimer;

	// MIDI activity pixmap common resources.
	static int      g_iMidiActivityRefCount;
	static QPixmap *g_pMidiActivityLedOn;
	static QPixmap *g_pMidiActivityLedOff;

	// Channel strip activation/selection.
	static ChannelStrip *g_pSelectedStrip;
};

} // namespace QSampler

#endif // __qsamplerChannelStrip_h


// end of qsamplerChannelStrip.h
