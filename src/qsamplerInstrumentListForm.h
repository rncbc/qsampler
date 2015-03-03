// qsamplerInstrumentListForm.h
//
/****************************************************************************
   Copyright (C) 2003-2015, rncbc aka Rui Nuno Capela. All rights reserved.
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

#ifndef __qsamplerInstrumentListForm_h
#define __qsamplerInstrumentListForm_h

#include "ui_qsamplerInstrumentListForm.h"

class QModelIndex;
class QComboBox;

namespace QSampler {

class InstrumentListView;

//-------------------------------------------------------------------------
// QSampler::InstrumentListForm -- Instrument map list form interface.
//

class InstrumentListForm : public QMainWindow
{
	Q_OBJECT

public:

	InstrumentListForm(QWidget *pParent = NULL, Qt::WindowFlags wflags = 0);
	~InstrumentListForm();

public slots:

	void newInstrument();
	void editInstrument();
	void editInstrument(const QModelIndex& index);
	void deleteInstrument();
	void refreshInstruments();
	void activateMap(int);

	void stabilizeForm();

protected:

	void showEvent(QShowEvent *);
	void hideEvent(QHideEvent *);
	void closeEvent(QCloseEvent *);

	void contextMenuEvent(QContextMenuEvent *);

private:

	Ui::qsamplerInstrumentListForm m_ui;

	QComboBox *m_pMapComboBox;

	InstrumentListView *m_pInstrumentListView;
};

} // namespace QSampler

#endif // __qsamplerInstrumentListForm_h


// end of qsamplerInstrumentListForm.h
