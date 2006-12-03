// qsamplerInstrumentListForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2004-2006, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qsamplerMainForm.h"


// Kind of constructor.
void qsamplerInstrumentListForm::init (void)
{
}


// Kind of destructor.
void qsamplerInstrumentListForm::destroy (void)
{
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
	InstrumentList->refresh();
}


// end of qsamplerInstrumentListForm.ui.h
