// qsamplerInstrumentListForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2004-2007, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qsamplerInstrument.h"

#include "qsamplerMainForm.h"
#include "qsamplerOptions.h"

#include <qcombobox.h>
#include <qtooltip.h>


// Kind of constructor.
void qsamplerInstrumentListForm::init (void)
{
	// Setup toolbar widgets.
	m_pMapComboBox = new QComboBox(InstrumentToolbar);
	m_pMapComboBox->setMinimumWidth(120);
	m_pMapComboBox->setEnabled(false);
	QToolTip::add(m_pMapComboBox, tr("Instrument Map"));

	InstrumentToolbar->addSeparator();
//	InstrumentList->newGroupAction()->addTo(InstrumentToolbar);
	InstrumentList->newItemAction()->addTo(InstrumentToolbar);
	InstrumentList->editItemAction()->addTo(InstrumentToolbar);
	InstrumentList->deleteAction()->addTo(InstrumentToolbar);
	InstrumentToolbar->addSeparator();
	InstrumentList->refreshAction()->addTo(InstrumentToolbar);

	QObject::connect(m_pMapComboBox,
		SIGNAL(activated(int)),
		SLOT(activateMap(int)));
}


// Kind of destructor.
void qsamplerInstrumentListForm::destroy (void)
{
	delete m_pMapComboBox;
}


// Notify our parent that we're emerging.
void qsamplerInstrumentListForm::showEvent ( QShowEvent *pShowEvent )
{
	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();

	QWidget::showEvent(pShowEvent);
}


// Notify our parent that we're closing.
void qsamplerInstrumentListForm::hideEvent ( QHideEvent *pHideEvent )
{
	QWidget::hideEvent(pHideEvent);

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Refresh all instrument list and views.
void qsamplerInstrumentListForm::refreshInstruments (void)
{
	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return;
		
	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// Get/save current map selection...
	int iMap = m_pMapComboBox->currentItem();
	if (iMap < 0 || m_pMapComboBox->count() < 2)
		iMap = pOptions->iMidiMap + 1;
 
	// Populate maps list.
	m_pMapComboBox->clear();
	m_pMapComboBox->insertItem(tr("(All)"));
	m_pMapComboBox->insertStringList(qsamplerInstrument::getMapNames());

	// Adjust to saved selection...
	if (iMap < 0 || iMap >= m_pMapComboBox->count())
		iMap = 0;
	m_pMapComboBox->setCurrentItem(iMap);
	m_pMapComboBox->setEnabled(m_pMapComboBox->count() > 1);

	activateMap(iMap);
}


// Refresh instrument maps selector.
void qsamplerInstrumentListForm::activateMap ( int iMap )
{
	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return;
		
	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	int iMidiMap = iMap - 1;
	if (iMidiMap >= 0)
		pOptions->iMidiMap = iMidiMap;

	InstrumentList->setMidiMap(iMidiMap);
	InstrumentList->refresh();
}


// end of qsamplerInstrumentListForm.ui.h
