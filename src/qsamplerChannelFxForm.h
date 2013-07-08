// qsamplerChannelFxForm.h
//
/****************************************************************************
   Copyright (C) 2010-2013, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2008, Christian Schoenebeck

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

#ifndef __qsamplerChannelFxForm_h
#define __qsamplerChannelFxForm_h

#include "ui_qsamplerChannelFxForm.h"

#include "qsamplerChannel.h"
#include "qsamplerDevice.h"

#include <QDialog>

namespace QSampler {

class ChannelFxForm : public QDialog {
	Q_OBJECT
public:
	ChannelFxForm(Channel* pSamplerChannel, QWidget* pParent = NULL, Qt::WindowFlags wflags = 0);
	~ChannelFxForm();

protected slots:
	void onFxSendSelection(const QModelIndex& index);
	void onButtonClicked(QAbstractButton* button);
	void onCreateFxSend();
	void onDestroyFxSend();
	void onDepthCtrlChanged(int iMidiCtrl);
	void onCurrentSendDepthChanged(int depthPercent);
	void onRoutingTableChanged();
	void updateTableCellRenderers();
	void updateTableCellRenderers(const QModelIndex& topLeft,
		const QModelIndex& bottomRight);

private:
	Ui::qsamplerChannelFxForm m_ui;

	Channel* m_pSamplerChannel;
	//int m_SamplerChannelID;
	Device* m_pAudioDevice;
};

} // namespace QSampler

#endif // __qsamplerChannelFxForm_h

// end of qsamplerChannelFxForm.h
