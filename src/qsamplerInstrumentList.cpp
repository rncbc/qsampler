// qsamplerInstrumentList.cpp
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

#include "qsamplerAbout.h"
#include "qsamplerInstrumentList.h"

#include "qsamplerInstrument.h"

#include "qsamplerOptions.h"
#include "qsamplerMainForm.h"

#include <QApplication>
#include <QCursor>


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::InstrumentListModel - data model for MIDI prog mappings
//

InstrumentListModel::InstrumentListModel ( QObject *pParent )
	: QAbstractItemModel(pParent), m_iMidiMap(LSCP_MIDI_MAP_ALL)
{
//	QAbstractItemModel::reset();
}

InstrumentListModel::~InstrumentListModel (void)
{
	clear();
}


int InstrumentListModel::rowCount ( const QModelIndex& /*parent*/) const
{
	int nrows = 0;

	if (m_iMidiMap == LSCP_MIDI_MAP_ALL) {
		InstrumentMap::const_iterator itMap = m_instruments.constBegin();
		for ( ; itMap != m_instruments.constEnd(); ++itMap)
			nrows += (*itMap).size();
	} else {
		InstrumentMap::const_iterator itMap = m_instruments.find(m_iMidiMap);
		if (itMap != m_instruments.constEnd())
			nrows += (*itMap).size();
	}

	return nrows;
}


int InstrumentListModel::columnCount ( const QModelIndex& /*parent*/) const
{
	return 9;
}


QVariant InstrumentListModel::data (
	const QModelIndex &index, int role ) const
{
	if (!index.isValid())
		return QVariant();
	const Instrument *pInstr
		= static_cast<Instrument *> (index.internalPointer());

	if (pInstr && role == Qt::DisplayRole) {
		switch (index.column()) {
			case 0: return pInstr->name();
			case 1: return QVariant::fromValue(pInstr->map());
			case 2: return QVariant::fromValue(pInstr->bank());
			case 3: return QVariant::fromValue(pInstr->prog() + 1);
			case 4: return pInstr->engineName();
			case 5: return pInstr->instrumentFile();
			case 6: return QVariant::fromValue(pInstr->instrumentNr());
			case 7: return QString::number(pInstr->volume() * 100.0) + " %";
			case 8: {
				switch (pInstr->loadMode()) {
					case 3: return tr("Persistent");
					case 2: return tr("On Demand Hold");
					case 1: return tr("On Demand");
				}
			}
			default:
				break;
		}
	}

	return QVariant();
}


QModelIndex InstrumentListModel::index (
	int row, int col, const QModelIndex& /*parent*/ ) const
{
	const Instrument *pInstr = NULL;

	if (m_iMidiMap == LSCP_MIDI_MAP_ALL) {
		int nrows = 0;
		InstrumentMap::const_iterator itMap = m_instruments.constBegin();
		for ( ;	itMap != m_instruments.constEnd(); ++itMap) {
			const InstrumentList& list = *itMap;
			nrows += list.size();
			if (row < nrows) {
				pInstr = list.at(row + list.size() - nrows);
				break;
			}
		}
	} else {
		// Resolve MIDI instrument map...
		InstrumentMap::const_iterator itMap	= m_instruments.find(m_iMidiMap);
		if (itMap != m_instruments.constEnd()) {
			const InstrumentList& list = *itMap;
			if (row < list.size())
				pInstr = list.at(row);
		}
	}

	if (pInstr)
		return createIndex(row, col, (void *) pInstr);
	else
		return QModelIndex();
}


QModelIndex InstrumentListModel::parent ( const QModelIndex& /*child*/ ) const
{
	return QModelIndex();
}


QVariant InstrumentListModel::headerData (
	int section, Qt::Orientation orientation, int role ) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch (section) {
			case 0: return tr("Name");
			case 1: return tr("Map");
			case 2: return tr("Bank");
			case 3: return tr("Prog");
			case 4: return tr("Engine");
			case 5: return tr("File");
			case 6: return tr("Nr");
			case 7: return tr("Vol");
			case 8: return tr("Mode");
		}
	}

	return QAbstractItemModel::headerData(section, orientation, role);
}


void InstrumentListModel::setMidiMap ( int iMidiMap )
{
	if (iMidiMap < 0)
		iMidiMap = LSCP_MIDI_MAP_ALL;

	m_iMidiMap = iMidiMap;
}


int InstrumentListModel::midiMap (void) const
{
	return m_iMidiMap;
}


const Instrument *InstrumentListModel::addInstrument (
	int iMap, int iBank, int iProg )
{
	// Check it there's already one instrument item
	// with the very same key (bank, program);
	// if yes, just remove it without prejudice...
	InstrumentList& list = m_instruments[iMap];
	for (int i = 0; i < list.size(); ++i) {
		const Instrument *pInstr = list.at(i);
		if (pInstr->bank() == iBank && pInstr->prog() == iProg) {
			delete pInstr;
			list.removeAt(i);
			break;
		}
	}

	// Resolve the appropriate place, we keep the list sorted that way...
	int i = 0;
	for ( ; i < list.size(); ++i) {
		const Instrument *pInstr = list.at(i);
		if (iBank < pInstr->bank()
			|| (iBank == pInstr->bank() && iProg < pInstr->prog())) {
			break;
		}
	}

	Instrument *pInstr = new Instrument(iMap, iBank, iProg);
	if (pInstr->getInstrument()) {
		list.insert(i, pInstr);
	} else {
		delete pInstr;
		pInstr = NULL;
	}

	return pInstr;
}


void InstrumentListModel::removeInstrument ( const Instrument *pInstrument )
{
	const int iMap  = pInstrument->map();
	const int iBank = pInstrument->bank();
	const int iProg = pInstrument->prog();

	if (m_instruments.contains(iMap)) {
		InstrumentList& list = m_instruments[iMap];
		for (int i = 0; i < list.size(); ++i) {
			const Instrument *pInstr = list.at(i);
			if (pInstr->bank() == iBank && pInstr->prog() == iProg) {
				delete pInstr;
				list.removeAt(i);
				break;
			}
		}
	}
}


// Reposition the instrument in the model (called when map/bank/prg changed)
void InstrumentListModel::updateInstrument ( const Instrument *pInstrument )
{
	const int iMap  = pInstrument->map();
	const int iBank = pInstrument->bank();
	const int iProg = pInstrument->prog();

	// Remove given instrument from its current list...
	removeInstrument(pInstrument);

	// Re-add the instrument...
	addInstrument(iMap, iBank, iProg);
}


void InstrumentListModel::refresh (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	clear();

	// Load the whole bunch of instrument items...
	lscp_midi_instrument_t *pInstrs
		= ::lscp_list_midi_instruments(pMainForm->client(), m_iMidiMap);
	for (int iInstr = 0; pInstrs && pInstrs[iInstr].map >= 0; ++iInstr) {
		const int iMap  = pInstrs[iInstr].map;
		const int iBank = pInstrs[iInstr].bank;
		const int iProg = pInstrs[iInstr].prog;
		addInstrument(iMap, iBank, iProg);
		// Try to keep it snappy :)
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	QApplication::restoreOverrideCursor();

	if (pInstrs == NULL && ::lscp_client_get_errno(pMainForm->client())) {
		pMainForm->appendMessagesClient("lscp_list_midi_instruments");
		pMainForm->appendMessagesError(
			tr("Could not get current list of MIDI instrument mappings.\n\nSorry."));
	}
}

void InstrumentListModel::beginReset (void)
{
#if QT_VERSION >= 0x040600
	QAbstractItemModel::beginResetModel();
#endif
}

void InstrumentListModel::endReset (void)
{
#if QT_VERSION >= 0x040600
	QAbstractItemModel::endResetModel();
#else
	QAbstractItemModel::reset();
#endif
}


// Map clear.
void InstrumentListModel::clear (void)
{
	InstrumentMap::iterator itMap = m_instruments.begin();
	for ( ;	itMap != m_instruments.end(); ++itMap) {
		InstrumentList& list = itMap.value();
		qDeleteAll(list);
		list.clear();
	}

	m_instruments.clear();
}


//-------------------------------------------------------------------------
// QSampler::InstrumentListView - list view for MIDI prog mappings
//

// Constructor.
InstrumentListView::InstrumentListView ( QWidget *pParent )
	: QTreeView(pParent)
{
	m_pListModel = new InstrumentListModel(this);

	QTreeView::setModel(m_pListModel);

	QTreeView::setRootIsDecorated(false);
	QTreeView::setUniformRowHeights(true);
	QTreeView::setAlternatingRowColors(true);
	QTreeView::setSelectionBehavior(QAbstractItemView::SelectRows);
	QTreeView::setSelectionMode(QAbstractItemView::SingleSelection);
	
	QHeaderView *pHeader = QTreeView::header();
	pHeader->setDefaultAlignment(Qt::AlignLeft);
	pHeader->setMovable(false);
	pHeader->setStretchLastSection(true);
	pHeader->resizeSection(0, 120);			// Name
	QTreeView::resizeColumnToContents(1);	// Map
	QTreeView::resizeColumnToContents(2);	// Bank
	QTreeView::resizeColumnToContents(3);	// Prog
	QTreeView::resizeColumnToContents(4);	// Engine
	pHeader->resizeSection(5, 240);			// File
	QTreeView::resizeColumnToContents(6);	// Nr
	pHeader->resizeSection(7, 60);			// Vol
}


// Destructor.
InstrumentListView::~InstrumentListView (void)
{
	delete m_pListModel;
}


void InstrumentListView::setMidiMap ( int iMidiMap )
{
	m_pListModel->setMidiMap(iMidiMap);
}


int InstrumentListView::midiMap (void) const
{
	return m_pListModel->midiMap();
}


const Instrument *InstrumentListView::addInstrument (
	int iMap, int iBank, int iProg )
{
	m_pListModel->beginReset();
	const Instrument *pInstrument
		= m_pListModel->addInstrument(iMap, iBank, iProg);
	m_pListModel->endReset();

	return pInstrument;
}


void InstrumentListView::removeInstrument ( const Instrument *pInstrument )
{
	m_pListModel->beginReset();
	m_pListModel->removeInstrument(pInstrument);
	m_pListModel->endReset();
}


// Reposition the instrument in the model (called when map/bank/prg changed)
void InstrumentListView::updateInstrument ( const Instrument *pInstrument )
{
	m_pListModel->beginReset();
	m_pListModel->updateInstrument(pInstrument);
	m_pListModel->endReset();
}


// Refreshener.
void InstrumentListView::refresh (void)
{
	m_pListModel->beginReset();
	m_pListModel->refresh();
	m_pListModel->endReset();
}


} // namespace QSampler


// end of qsamplerInstrumentList.cpp
