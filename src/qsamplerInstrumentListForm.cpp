// qsamplerInstrumentListForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2007, rncbc aka Rui Nuno Capela. All rights reserved.
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

#include "qsamplerInstrumentListForm.h"

#include "qsamplerInstrumentForm.h"
#include "qsamplerMainForm.h"
#include "qsamplerOptions.h"
#include "qsamplerInstrument.h"


namespace QSampler {

InstrumentListForm::InstrumentListForm ( QWidget* parent, Qt::WindowFlags flags )
	: QMainWindow(parent, flags)
{
    ui.setupUi(this);

    ui.newInstrumentAction->setText(tr("New &Instrument..."));
    ui.newInstrumentAction->setShortcut(Qt::Key_Insert);
    ui.editInstrumentAction->setText(tr("&Edit..."));
    ui.editInstrumentAction->setShortcut(Qt::Key_Enter);
    ui.deleteInstrumentAction->setText(tr("&Delete"));
    ui.deleteInstrumentAction->setShortcut(Qt::Key_Delete);
    ui.refreshInstrumentsAction->setText(tr("&Refresh"));
    ui.refreshInstrumentsAction->setShortcut(Qt::Key_F5);

    // Setup toolbar widgets.
    m_pMapComboBox = new QComboBox(ui.InstrumentToolbar);
    m_pMapComboBox->setMinimumWidth(120);
    m_pMapComboBox->setEnabled(false);
    m_pMapComboBox->setToolTip(tr("Instrument Map"));
    ui.InstrumentToolbar->addWidget(m_pMapComboBox);

    ui.InstrumentToolbar->addSeparator();
    ui.InstrumentToolbar->addAction(ui.newInstrumentAction);
    ui.InstrumentToolbar->addAction(ui.editInstrumentAction);
    ui.InstrumentToolbar->addAction(ui.deleteInstrumentAction);
    ui.InstrumentToolbar->addSeparator();
    ui.InstrumentToolbar->addAction(ui.refreshInstrumentsAction);

    ui.InstrumentTable->setModel(&model);
    ui.InstrumentTable->setItemDelegate(&delegate);

	QObject::connect(m_pMapComboBox,
		SIGNAL(activated(int)),
		SLOT(activateMap(int)));

	QObject::connect(
		ui.refreshInstrumentsAction,
		SIGNAL(triggered()),
		SLOT(refreshInstruments(void))
    );

	connect(
		ui.InstrumentTable,
		SIGNAL(activated(const QModelIndex&)),
		SLOT(editInstrument(const QModelIndex&))
	);
	connect(
		ui.newInstrumentAction,
		SIGNAL(triggered()),
		SLOT(newInstrument())
	);
	connect(
		ui.deleteInstrumentAction,
		SIGNAL(triggered()),
		SLOT(deleteInstrument())
	);
	connect(
		ui.editInstrumentAction,
		SIGNAL(triggered()),
		SLOT(editInstrument())
	);
}

InstrumentListForm::~InstrumentListForm() {
	delete m_pMapComboBox;
}


// Notify our parent that we're emerging.
void InstrumentListForm::showEvent ( QShowEvent *pShowEvent )
{
	//MainForm* pMainForm = MainForm::getInstance();
	//if (pMainForm)
	//	pMainForm->stabilizeForm();

	QWidget::showEvent(pShowEvent);
}


// Notify our parent that we're closing.
void InstrumentListForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	//MainForm* pMainForm = MainForm::getInstance();
	//if (pMainForm)
	//	pMainForm->stabilizeForm();
}


// Refresh all instrument list and views.
void InstrumentListForm::refreshInstruments (void)
{
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// Get/save current map selection...
	int iMap = m_pMapComboBox->currentIndex();
	if (iMap < 0 || m_pMapComboBox->count() < 2)
		iMap = pOptions->iMidiMap + 1;

	// Populate maps list.
	m_pMapComboBox->clear();
	m_pMapComboBox->addItem(tr("(All)"));
	m_pMapComboBox->insertItems(1, qsamplerInstrument::getMapNames());

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

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	int iMidiMap = iMap - 1;
	if (iMidiMap >= 0)
		pOptions->iMidiMap = iMidiMap;

	model.setMidiMap(iMidiMap);
	model.refresh();
}

void InstrumentListForm::editInstrument() {
	const QModelIndex index = ui.InstrumentTable->currentIndex();
	editInstrument(index);
}

void InstrumentListForm::editInstrument(const QModelIndex& index) {
	if (!index.isValid() || !index.data(Qt::UserRole).isValid())
		return;

	qsamplerInstrument* pInstrument =
		(qsamplerInstrument*) index.data(Qt::UserRole).value<void*>();

	if (!pInstrument) return;

	// Save current key values...
	qsamplerInstrument oldInstrument(*pInstrument);
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
			// just update tree item...
			//pItem->update();
		} else {
			// Unmap old instance...
			oldInstrument.unmapInstrument();
			// correct the position of the instrument in the model
			model.resort(*pInstrument);
		}
		// Notify we've changes...
		emit model.reset();
	}
}

void InstrumentListForm::newInstrument() {
	qsamplerInstrument instrument;

	InstrumentForm form(this);
	form.setup(&instrument);
	if (!form.exec()) return;

	// Commit...
	instrument.mapInstrument();
	// add new item to the table model
	model.resort(instrument);
	// Notify we've changes...
	//emit model.reset();
	//FIXME: call above didnt really refresh, so we use this for now ...
	refreshInstruments();
}

void InstrumentListForm::deleteInstrument() {
	const QModelIndex index = ui.InstrumentTable->currentIndex();
	if (!index.isValid() || !index.data(Qt::UserRole).isValid()) return;

	qsamplerInstrument* pInstrument =
		(qsamplerInstrument*) index.data(Qt::UserRole).value<void*>();

	if (!pInstrument) return;

	pInstrument->unmapInstrument();
	// let the instrument vanish from the table model
	model.removeInstrument(*pInstrument);
	// Notify we've changes...
	emit model.reset();
}

} // namespace QSampler


// end of qsamplerInstrumentListForm.cpp
