// qsamplerChannelFxForm.cpp
//
/****************************************************************************
   Copyright (C) 2008, Christian Schoenebeck

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
#include "qsamplerChannelFxForm.h"
#include "qsamplerFxSendsModel.h"

#include <QAbstractButton>

namespace QSampler {

ChannelFxForm::ChannelFxForm (
	int SamplerChannelID, QWidget* pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags)
{
	m_ui.setupUi(this);

	m_SamplerChannelID = SamplerChannelID;

	FxSendsModel* pModel =
		new FxSendsModel(SamplerChannelID, m_ui.SendsListView);
	m_ui.SendsListView->setModel(pModel);

	QAbstractButton* pApplyButton =
		m_ui.buttonBox->button(QDialogButtonBox::Apply);
	pApplyButton->setEnabled(false);
	pApplyButton->setIcon(QIcon(":/icons/formEdit.png"));

	QAbstractButton* pCancelButton =
		m_ui.buttonBox->button(QDialogButtonBox::Cancel);
	pCancelButton->setIcon(QIcon(":/icons/formRemove.png"));

	QAbstractButton* pOkButton =
		m_ui.buttonBox->button(QDialogButtonBox::Ok);
	pOkButton->setIcon(QIcon(":/icons/formAccept.png"));

	QAbstractButton* pResetButton =
		m_ui.buttonBox->button(QDialogButtonBox::Reset);
	pResetButton->setEnabled(false);

	m_ui.destroyPushButton->setEnabled(false);

	connect(
		m_ui.buttonBox, SIGNAL(clicked(QAbstractButton*)),
		this, SLOT(onButtonClicked(QAbstractButton*))
	);
	connect(
		m_ui.createPushButton, SIGNAL(clicked()),
		this, SLOT(onCreateFxSend())
	);
	connect(
		m_ui.destroyPushButton, SIGNAL(clicked()),
		this, SLOT(onDestroyFxSend())
	);
	connect(
		pModel, SIGNAL(fxSendsDirtyChanged(bool)),
		pApplyButton, SLOT(setEnabled(bool))
	);
	connect(
		pModel, SIGNAL(fxSendsDirtyChanged(bool)),
		pResetButton, SLOT(setEnabled(bool))
	);
	connect(
		m_ui.SendsListView, SIGNAL(clicked(const QModelIndex&)),
		this, SLOT(onFxSendSelection(const QModelIndex&))
	);
}

ChannelFxForm::~ChannelFxForm() {
}

void ChannelFxForm::onButtonClicked(QAbstractButton* button) {
	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	switch (m_ui.buttonBox->buttonRole(button)) {
		case QDialogButtonBox::AcceptRole:
		case QDialogButtonBox::ApplyRole:
			pModel->applyToSampler();
			break;
		case QDialogButtonBox::ResetRole:
			pModel->cleanRefresh();
			break;
		default: // to avoid gcc warnings
			break;
	}
}

void ChannelFxForm::onFxSendSelection(const QModelIndex& index) {
	m_ui.destroyPushButton->setEnabled(index.isValid());
}

void ChannelFxForm::onCreateFxSend() {
	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	pModel->addFxSend();
}

void ChannelFxForm::onDestroyFxSend() {
	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	pModel->removeFxSend(m_ui.SendsListView->currentIndex());
}

} // namespace QSampler

// end of qsamplerChannelFxForm.cpp
