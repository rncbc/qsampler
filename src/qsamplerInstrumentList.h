// qsamplerInstrumentList.h
//
/****************************************************************************
   Copyright (C) 2003-2010, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007, Christian Schoenebeck

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

#ifndef __qsamplerInstrumentList_h
#define __qsamplerInstrumentList_h

#include <QListWidget>
#include <QItemDelegate>

#include <lscp/client.h>

#include "qsamplerInstrument.h"

namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::MidiInstrumentsModel - data model for MIDI prog mappings
//                                  (used for QTableView)

class MidiInstrumentsModel : public QAbstractItemModel
{
	Q_OBJECT

public:

	MidiInstrumentsModel(QObject *pParent = NULL);

	// Overridden methods from subclass(es)
	int rowCount(const QModelIndex& parent) const;
	int columnCount(const QModelIndex& parent) const;

	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const;

	// Make the following method public
	void beginReset();
	void endReset();

	// Own methods
	Instrument *addInstrument(int iMap = 0,
		int iBank = -1, int iProg = -1);
	void removeInstrument(const Instrument& instrument);

	void resort(const Instrument& instrument);

	// Map selector.
	void setMidiMap(int iMidiMap);
	int midiMap() const;

signals:

	// Instrument map/session change signal.
	void instrumentsChanged();

public slots:

	// General reloader.
	void refresh();

protected:

	QModelIndex index(int row, int col, const QModelIndex& parent) const;
	QModelIndex parent(const QModelIndex& child) const;

private:

	typedef QList<Instrument> InstrumentList;
	typedef QMap<int, InstrumentList> InstrumentsMap;

	InstrumentsMap m_instruments;

	// Current map selection.
	int m_iMidiMap;
};


} // namespace QSampler

#endif  // __qsamplerInstrumentList_h


// end of qsamplerInstrumentList.h
