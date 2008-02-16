// qsamplerDeviceStatusForm.h
//
/****************************************************************************
   Copyright (C) 2008, Christian Schoenebeck

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#ifndef __qsamplerDeviceStatusForm_h
#define __qsamplerDeviceStatusForm_h

#include "qsamplerDevice.h"

#include <QMainWindow>
#include <QAction>
#include <QCloseEvent>
#include <QLabel>
#include <QTimer>
#include <map>

namespace QSampler {

class MidiActivityLED : public QLabel {
	Q_OBJECT
public:
	MidiActivityLED(QString text = "", QWidget* parent = 0);
	void midiDataArrived();

protected slots:
	void midiDataCeased();

private:
	QTimer timer;
};

class DeviceStatusForm : public QMainWindow {
	Q_OBJECT
public:
	DeviceStatusForm(int DeviceID, QWidget* pParent = NULL, Qt::WindowFlags wflags = 0);
	~DeviceStatusForm();
	QAction* visibleAction();
	void midiArrived(int iPort);

	static DeviceStatusForm* getInstance(int iDeviceID);
	static const std::map<int, DeviceStatusForm*>& getInstances();
	static void onDevicesChanged();

protected:
	void closeEvent(QCloseEvent* event);

	std::vector<MidiActivityLED*> midiActivityLEDs;

private:
	int m_DeviceID;
	Device* m_pDevice;
	QAction* m_pVisibleAction;

	static std::map<int, DeviceStatusForm*> instances;
};

} // namespace QSampler

#endif // __qsamplerDeviceStatusForm_h

// end of qsamplerDeviceStatusForm.h
