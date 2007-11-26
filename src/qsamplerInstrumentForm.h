// qsamplerInstrumentForm.h
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

#ifndef __qsamplerInstrumentForm_h
#define __qsamplerInstrumentForm_h

#include "ui_qsamplerInstrumentForm.h"

#include "qsamplerInstrument.h"


namespace QSampler {

class InstrumentForm : public QDialog
{
	Q_OBJECT

public:

	InstrumentForm(QWidget* pParent = NULL);
	~InstrumentForm();

	void setup(qsamplerInstrument* pInstrument);

public slots:

	void nameChanged(const QString& sName);
	void openInstrumentFile();
	void updateInstrumentName();
	void instrumentNrChanged();
	void accept();
	void reject();
	void changed();
	void stabilizeForm();

private:

	Ui::qsamplerInstrumentForm m_ui;

	qsamplerInstrument* m_pInstrument;
	int m_iDirtySetup;
	int m_iDirtyCount;
	int m_iDirtyName;
};

} // namespace QSampler

#endif // __qsamplerInstrumentForm_h


// end of qsamplerInstrumentForm.h
