// qsamplerInstrumentListForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2009, rncbc aka Rui Nuno Capela. All rights reserved.
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
#include "qsamplerInstrumentListForm.h"

#include "qsamplerInstrumentForm.h"

#include "qsamplerOptions.h"
#include "qsamplerInstrument.h"
#include "qsamplerMainForm.h"

#include <QHeaderView>
#include <QMessageBox>


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::InstrumentListForm -- Instrument map list form implementation.
//

InstrumentListForm::InstrumentListForm (
	QWidget* pParent, Qt::WindowFlags wflags )
	: QMainWindow(pParent, wflags)
{
	m_ui.setupUi(this);

	// Setup toolbar widgets.
	m_pMapComboBox = new QComboBox(m_ui.InstrumentToolbar);
	m_pMapComboBox->setMinimumWidth(120);
	m_pMapComboBox->setEnabled(false);
	m_pMapComboBox->setToolTip(tr("Instrument Map"));
	m_ui.InstrumentToolbar->addWidget(m_pMapComboBox);

	m_ui.InstrumentToolbar->addSeparator();
	m_ui.InstrumentToolbar->addAction(m_ui.newInstrumentAction);
	m_ui.InstrumentToolbar->addAction(m_ui.editInstrumentAction);
	m_ui.InstrumentToolbar->addAction(m_ui.deleteInstrumentAction);
	m_ui.InstrumentToolbar->addSeparator();
	m_ui.InstrumentToolbar->addAction(m_ui.refreshInstrumentsAction);

	int iRowHeight = m_ui.InstrumentTable->fontMetrics().height() + 4;
	m_ui.InstrumentTable->verticalHeader()->setDefaultSectionSize(iRowHeight);

	m_ui.InstrumentTable->setModel(&m_model);
	m_ui.InstrumentTable->setItemDelegate(&m_delegate);
	m_ui.InstrumentTable->verticalHeader()->hide();

	QHeaderView *pHeader = m_ui.InstrumentTable->horizontalHeader();
	pHeader->setDefaultAlignment(Qt::AlignLeft);
	pHeader->setMovable(false);
	pHeader->setStretchLastSection(true);
	pHeader->resizeSection(0, 120);						// Name
	m_ui.InstrumentTable->resizeColumnToContents(1);	// Map
	m_ui.InstrumentTable->resizeColumnToContents(2);	// Bank
	m_ui.InstrumentTable->resizeColumnToContents(3);	// Prog
	m_ui.InstrumentTable->resizeColumnToContents(4);	// Engine
	pHeader->resizeSection(5, 240);						// File
	m_ui.InstrumentTable->resizeColumnToContents(6);	// Nr
	pHeader->resizeSection(7, 60);						// Vol

	// Enable custom context menu...
	m_ui.InstrumentTable->setContextMenuPolicy(Qt::CustomContextMenu);

	QObject::connect(m_pMapComboBox,
		SIGNAL(activated(int)),
		SLOT(activateMap(int)));
	QObject::connect(
		m_ui.InstrumentTable,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(contextMenu(const QPoint&)));
	QObject::connect(
		m_ui.InstrumentTable,
		SIGNAL(pressed(const QModelIndex&)),
		SLOT(stabilizeForm()));
	QObject::connect(
		m_ui.InstrumentTable,
		SIGNAL(activated(const QModelIndex&)),
		SLOT(editInstrument(const QModelIndex&)));
	QObject::connect(
		m_ui.newInstrumentAction,
		SIGNAL(triggered()),
		SLOT(newInstrument()));
	QObject::connect(
		m_ui.deleteInstrumentAction,
		SIGNAL(triggered()),
		SLOT(deleteInstrument()));
	QObject::connect(
		m_ui.editInstrumentAction,
		SIGNAL(triggered()),
		SLOT(editInstrument()));
	QObject::connect(
		m_ui.refreshInstrumentsAction,
		SIGNAL(triggered()),
		SLOT(refreshInstruments()));

	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm) {
		QObject::connect(&m_model,
			SIGNAL(instrumentsChanged()),
			pMainForm, SLOT(sessionDirty()));
	}

	// Things must be stable from the start.
	stabilizeForm();
}


InstrumentListForm::~InstrumentListForm (void)
{
	delete m_pMapComboBox;
}


// Notify our parent that we're emerging.
void InstrumentListForm::showEvent ( QShowEvent *pShowEvent )
{
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	QWidget::showEvent(pShowEvent);
}


// Notify our parent that we're closing.
void InstrumentListForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Just about to notify main-window that we're closing.
void InstrumentListForm::closeEvent ( QCloseEvent * /*pCloseEvent*/ )
{
	QWidget::hide();

	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Refresh all instrument list and views.
void InstrumentListForm::refreshInstruments (void)
{
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	Options *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// Get/save current map selection...
	int iMap = m_pMapComboBox->currentIndex();
	if (iMap < 0 || m_pMapComboBox->count() < 2)
		iMap = pOptions->iMidiMap + 1;

	// Populate maps list.
	m_pMapComboBox->clear();
	m_pMapComboBox->addItem(tr("(All)"));
	m_pMapComboBox->insertItems(1, Instrument::getMapNames());

	// Adjust to saved selection...
	if (iMap < 0 || iMap >= m_pMapComboBox->count())
		iMap = 0;
	m_pMapComboBox->setCurrentIndex(iMap);
	m_pMapComboBox->setEnabled(m_pMapComboBox->count() > 1);

	activateMap(iMap);
}


// Refresh instrument maps selector.
void InstrumentListForm::activateMap ( int iMap )
{
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	Options *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	int iMidiMap = iMap - 1;
	if (iMidiMap >= 0)
		pOptions->iMidiMap = iMidiMap;

	m_model.setMidiMap(iMidiMap);
	m_model.refresh();

	stabilizeForm();
}


void InstrumentListForm::editInstrument (void)
{
	editInstrument(m_ui.InstrumentTable->currentIndex());
}


void InstrumentListForm::editInstrument ( const QModelIndex& index )
{
	if (!index.isValid() || !index.data(Qt::UserRole).isValid())
		return;

	Instrument* pInstrument
		= static_cast<Instrument *> (
			index.data(Qt::UserRole).value<void *> ());

	if (pInstrument == NULL)
		return;

	// Save current key values...
	Instrument oldInstrument(*pInstrument);
	// Do the edit dance...
	InstrumentForm form(this);
	form.setup(pInstrument);
	if (form.exec()) {
		// Commit...
		pInstrument->mapInstrument();
		// Check whether we changed instrument key...
		if (oldInstrument.map()  == pInstrument->map()  &&
			oldInstrument.bank() == pInstrument->bank() &&
			oldInstrument.prog() == pInstrument->prog()) {
			// Just update tree item...
			//pItem->update();
		} else {
			// Unmap old instance...
			oldInstrument.unmapInstrument();
			// correct the position of the instrument in the model
			m_model.resort(*pInstrument);
		}
		// Notify we've changes...
		emit m_model.reset();
	}
}


void InstrumentListForm::newInstrument (void)
{
	Instrument instrument;

	InstrumentForm form(this);
	form.setup(&instrument);
	if (!form.exec())
		return;

	// Commit...
	instrument.mapInstrument();
	// add new item to the table model
	m_model.resort(instrument);
	// Notify we've changes...
	//emit model.reset();
	//FIXME: call above didnt really refresh, so we use this for now ...
	refreshInstruments();
}


void InstrumentListForm::deleteInstrument (void)
{
	const QModelIndex& index = m_ui.InstrumentTable->currentIndex();
	if (!index.isValid() || !index.data(Qt::UserRole).isValid())
		return;

	Instrument *pInstrument =
		static_cast<Instrument*> (
			index.data(Qt::UserRole).value<void *> ());

	if (pInstrument == NULL)
		return;

	MainForm *pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	// Prompt user if this is for real...
	Options *pOptions = pMainForm->options();
	if (pOptions && pOptions->bConfirmRemove) {
		if (QMessageBox::warning(this,
			QSAMPLER_TITLE ": " + tr("Warning"),
			tr("About to delete instrument map entry:\n\n"
			"%1\n\n"
			"Are you sure?")
			.arg(pInstrument->name()),
			QMessageBox::Ok | QMessageBox::Cancel)
			== QMessageBox::Cancel)
			return;
	}

	pInstrument->unmapInstrument();
	// let the instrument vanish from the table model
	m_model.removeInstrument(*pInstrument);
	// Notify we've changes...
	emit m_model.reset();
}


// Update form actions enablement...
void InstrumentListForm::stabilizeForm (void)
{
	MainForm *pMainForm = MainForm::getInstance();

	bool bEnabled = (pMainForm && pMainForm->client());
	m_ui.newInstrumentAction->setEnabled(bEnabled);
	const QModelIndex& index = m_ui.InstrumentTable->currentIndex();
	bEnabled = (bEnabled && index.isValid());
	m_ui.editInstrumentAction->setEnabled(bEnabled);
	m_ui.deleteInstrumentAction->setEnabled(bEnabled);
}


// Handle custom context menu here...
void InstrumentListForm::contextMenu ( const QPoint& pos )
{
	if (!m_ui.newInstrumentAction->isEnabled())
		return;

	m_ui.contextMenu->exec(
		(m_ui.InstrumentTable->viewport())->mapToGlobal(pos));
}


} // namespace QSampler


// end of qsamplerInstrumentListForm.cpp
