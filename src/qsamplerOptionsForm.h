// qsamplerOptionsForm.h
//
/****************************************************************************
   Copyright (C) 2004-2009, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007, Christian Schoenebeck

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

#ifndef __qsamplerOptionsForm_h
#define __qsamplerOptionsForm_h

#include "ui_qsamplerOptionsForm.h"

#include "qsamplerOptions.h"


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::OptionsForm -- Options form interface.
//

class OptionsForm : public QDialog
{
	Q_OBJECT

public:

	OptionsForm(QWidget *pParent = NULL);
	~OptionsForm();

	void setup(Options* pOptions);

protected slots:

	void accept();
	void reject();
	void optionsChanged();
	void stabilizeForm();
	void browseMessagesLogPath();
	void chooseDisplayFont();
	void chooseMessagesFont();
	void toggleDisplayEffect(bool bOn);
	void maxVoicesChanged(int iMaxVoices);
	void maxStreamsChanged(int iMaxStreams);

private:

	Ui::qsamplerOptionsForm m_ui;

	Options* m_pOptions;
	int m_iDirtySetup;
	int m_iDirtyCount;
	bool bMaxVoicesModified;
	bool bMaxStreamsModified;
};

} // namespace QSampler

#endif // __qsamplerOptionsForm_h


// end of qsamplerOptionsForm.h
