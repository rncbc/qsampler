// qsamplerChannelForm.h
//
/****************************************************************************
   Copyright (C) 2004-2016, rncbc aka Rui Nuno Capela. All rights reserved.
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

#ifndef __qsamplerChannelForm_h
#define __qsamplerChannelForm_h

#include "ui_qsamplerChannelForm.h"

#include "qsamplerDevice.h"
#include "qsamplerChannel.h"
#include "qsamplerDeviceForm.h"


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::Channelform -- Channel form interface.
//

class ChannelForm : public QDialog
{
	Q_OBJECT

public:

	ChannelForm(QWidget* pParent = NULL);
	~ChannelForm();

	void setup(Channel* pChannel);

	void setupDevice(Device* pDevice,
		Device::DeviceType deviceTypeMode,
		const QString& sDriverName);

	void selectMidiDriverItem(const QString& sMidiDriver);
	void selectMidiDeviceItem(int iMidiItem);
	void selectAudioDriverItem(const QString& sAudioDriver);
	void selectAudioDeviceItem(int iAudioItem);

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
	void updateDevices();
	void optionsChanged();
	void stabilizeForm();

	void updateTableCellRenderers();
	void updateTableCellRenderers(
		const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:

	Ui::qsamplerChannelForm m_ui;

	Channel* m_pChannel;
	int m_iDirtySetup;
	int m_iDirtyCount;
	QList<Device *> m_audioDevices;
	QList<Device *> m_midiDevices;
	DeviceForm* m_pDeviceForm;
	ChannelRoutingModel m_routingModel;
	ChannelRoutingDelegate m_routingDelegate;
};

} // namespace QSampler

#endif // __qsamplerChannelForm_h


// end of qsamplerChannelForm.h
