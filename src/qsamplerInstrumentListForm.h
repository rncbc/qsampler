// qsamplerInstrumentListForm.h
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

#ifndef __qsamplerInstrumentListForm_h
#define __qsamplerInstrumentListForm_h

#include "ui_qsamplerInstrumentListForm.h"

#include "qsamplerInstrumentList.h"

#include <QComboBox>

namespace QSampler {

class InstrumentListForm : public QMainWindow {
Q_OBJECT
public:
    MidiInstrumentsModel model;
    MidiInstrumentsDelegate delegate;

    InstrumentListForm(QWidget* parent = 0, Qt::WindowFlags flags = 0);
   ~InstrumentListForm();

public slots:
    void refreshInstruments();
    void activateMap(int);

protected:
    QComboBox* m_pMapComboBox;

    void showEvent(QShowEvent* pShowEvent);
    void hideEvent(QHideEvent* pHideEvent);

protected slots:
    void editInstrument();
    void editInstrument(const QModelIndex& index);
    void newInstrument();
    void deleteInstrument();

private:
    Ui::qsamplerInstrumentListForm ui;
};

} // namespace QSampler

#endif // __qsamplerInstrumentListForm_h


// end of qsamplerInstrumentListForm.h
