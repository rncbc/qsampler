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
#include "qsamplerInstrumentForm.h"

#include "qsamplerOptions.h"
#include "qsamplerMainForm.h"

#include <QApplication>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QCursor>
#include <QFileInfo>

namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::MidiInstrumentsModel - data model for MIDI prog mappings
//                                  (used for QTreeView)

MidiInstrumentsModel::MidiInstrumentsModel ( QObject *pParent )
	: QAbstractItemModel(pParent)
{
	m_iMidiMap = LSCP_MIDI_MAP_ALL;

	QAbstractItemModel::reset();
}


int MidiInstrumentsModel::rowCount ( const QModelIndex& /*parent*/) const
{
	int nrows = 0;

	if (m_iMidiMap == LSCP_MIDI_MAP_ALL) {
		InstrumentsMap::const_iterator itMap = m_instruments.begin();
		for ( ; itMap != m_instruments.end(); ++itMap)
			nrows += (*itMap).size();
	} else {
		InstrumentsMap::const_iterator itMap
			= m_instruments.find(m_iMidiMap);
		if (itMap != m_instruments.end())
			nrows += (*itMap).size();
	}

	return nrows;
}


int MidiInstrumentsModel::columnCount ( const QModelIndex& /*parent*/) const
{
	return 9;
}


QVariant MidiInstrumentsModel::data ( const QModelIndex &index, int role ) const
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


QModelIndex MidiInstrumentsModel::index ( int row, int col,
	const QModelIndex& /*parent*/ ) const
{
	const Instrument *pInstr = NULL;

	if (m_iMidiMap == LSCP_MIDI_MAP_ALL) {
		int nrows = 0;
		InstrumentsMap::const_iterator itMap = m_instruments.begin();
		for ( ;	itMap != m_instruments.end(); ++itMap) {
			nrows += (*itMap).size();
			if (row < nrows) {
				pInstr = &(*itMap)[row + (*itMap).size() - nrows];
				break;
			}
		}
	} else {
		// Resolve MIDI instrument map...
		InstrumentsMap::const_iterator itMap
			= m_instruments.find(m_iMidiMap);
		if (itMap != m_instruments.end()) {
			// resolve instrument in that map
			if (row < (*itMap).size())
				pInstr = &(*itMap)[row];
		}
	}

	if (pInstr)
		return createIndex(row, col, (void *) pInstr);
	else
		return QModelIndex();
}


QModelIndex MidiInstrumentsModel::parent ( const QModelIndex& child ) const
{
	return QModelIndex();
}


QVariant MidiInstrumentsModel::headerData (
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


Instrument *MidiInstrumentsModel::addInstrument (
	int iMap, int iBank, int iProg )
{
	// Check it there's already one instrument item
	// with the very same key (bank, program);
	// if yes, just remove it without prejudice...
	InstrumentList& list = m_instruments[iMap];
	for (int i = 0; i < list.size(); ++i) {
		const Instrument& instr = list[i];
		if (instr.bank() == iBank && instr.prog() == iProg) {
			list.removeAt(i);
			break;
		}
	}

	// Resolve the appropriate place, we keep the list sorted that way...
	int i = 0;
	for ( ; i < list.size(); ++i) {
		const Instrument& instr = list[i];
		if (iBank < instr.bank()
			|| (iBank == instr.bank() && iProg < instr.prog())) {
			break;
		}
	}

	list.insert(i, Instrument(iMap, iBank, iProg));

	Instrument& instr = list[i];
	if (!instr.getInstrument())
		list.removeAt(i);

	return &instr;
}


void MidiInstrumentsModel::removeInstrument (
	const Instrument& instrument )
{
	const int iMap  = instrument.map();
	const int iBank = instrument.bank();
	const int iProg = instrument.prog();

	InstrumentList& list = m_instruments[iMap];
	for (int i = 0; i < list.size(); ++i) {
		const Instrument& instr = list[i];
		if (instr.bank() == iBank && instr.prog() == iProg) {
			list.removeAt(i);
			break;
		}
	}
}


// Reposition the instrument in the model (called when map/bank/prg changed)
void MidiInstrumentsModel::resort ( const Instrument& instrument )
{
	const int iMap  = instrument.map();
	const int iBank = instrument.bank();
	const int iProg = instrument.prog();

	// Remove given instrument from its current list...
	removeInstrument(instrument);
	// Re-add the instrument...
	addInstrument(iMap, iBank, iProg);
}


void MidiInstrumentsModel::setMidiMap ( int iMidiMap )
{
	if (iMidiMap < 0)
		iMidiMap = LSCP_MIDI_MAP_ALL;

	m_iMidiMap = iMidiMap;
}


int MidiInstrumentsModel::midiMap (void) const
{
	return m_iMidiMap;
}

void MidiInstrumentsModel::refresh (void)
{
	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	beginReset();

	m_instruments.clear();

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

	// Inform the outer world (QTableView) that our data changed...
	endReset();

	QApplication::restoreOverrideCursor();

	if (pInstrs == NULL && ::lscp_client_get_errno(pMainForm->client())) {
		pMainForm->appendMessagesClient("lscp_list_midi_instruments");
		pMainForm->appendMessagesError(
			tr("Could not get current list of MIDI instrument mappings.\n\nSorry."));
	}
}

void MidiInstrumentsModel::beginReset (void)
{
#if QT_VERSION >= 0x040600
	QAbstractItemModel::beginResetModel();
#endif
}

void MidiInstrumentsModel::endReset (void)
{
#if QT_VERSION >= 0x040600
	QAbstractItemModel::endResetModel();
#else
	QAbstractItemModel::reset();
#endif
}


} // namespace QSampler


// end of qsamplerInstrumentList.cpp
