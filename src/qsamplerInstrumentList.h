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

#include <QTreeView>

namespace QSampler {

class Instrument;

//-------------------------------------------------------------------------
// QSampler:InstrumentListModel - data model for MIDI prog mappings
//

class InstrumentListModel : public QAbstractItemModel
{
	Q_OBJECT

public:

	// Constructor.
	InstrumentListModel(QObject *pParent = NULL);

	// Destructor.
	~InstrumentListModel();

	// Overridden methods from subclass(es)
	int rowCount(const QModelIndex& parent) const;
	int columnCount(const QModelIndex& parent) const;

	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const;

	// Map selector.
	void setMidiMap(int iMidiMap);
	int midiMap() const;

	// Own methods
	const Instrument *addInstrument(int iMap, int iBank, int iProg);
	void removeInstrument(const Instrument *pInstrument);
	void updateInstrument(const Instrument *pInstrument);

	// General reloader.
	void refresh();

	// Make the following method public
	void beginReset();
	void endReset();

	// Map clear.
	void clear();

protected:

	QModelIndex index(int row, int col, const QModelIndex& parent) const;
	QModelIndex parent(const QModelIndex& child) const;

private:

	typedef QList<Instrument *> InstrumentList;
	typedef QMap<int, InstrumentList> InstrumentMap;

	InstrumentMap m_instruments;

	// Current map selection.
	int m_iMidiMap;
};


//-------------------------------------------------------------------------
// QSampler::InstrumentListView - list view for MIDI prog mappings
//

class InstrumentListView : public QTreeView
{
	Q_OBJECT

public:

	// Constructor.
	InstrumentListView(QWidget *pParent = 0);

	// Destructor.
	~InstrumentListView();

	// Map selector.
	void setMidiMap(int iMidiMap);
	int midiMap() const;

	// Own methods
	const Instrument *addInstrument(int iMap, int iBank, int iProg);
	void removeInstrument(const Instrument *pInstrument);
	void updateInstrument(const Instrument *pInstrument);

	// General reloader.
	void refresh();

private:

	// Instance variables.
	InstrumentListModel *m_pListModel;
};


} // namespace QSampler

#endif  // __qsamplerInstrumentList_h


// end of qsamplerInstrumentList.h
