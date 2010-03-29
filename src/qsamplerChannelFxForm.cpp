// qsamplerChannelFxForm.cpp
//
/****************************************************************************
   Copyright (C) 2008, Christian Schoenebeck
   Copyright (C) 2010, rncbc aka Rui Nuno Capela. All rights reserved.

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

// let's not reinvent the wheel for audio routing
#include "qsamplerChannel.h"

#include <math.h>

#include <QAbstractButton>
#include <QLineEdit>
#include <QHeaderView>
#include <QMap>

namespace { // private namespace

static const char* _midiControllerName(int iMidiCtrl) {
	switch (iMidiCtrl) {
		case 0:   return "Bank select MSB";
		case 1:   return "Modulation MSB";
		case 2:   return "Breath Controller";
		case 4:   return "Foot Controller MSB";
		case 5:   return "Portamento Time MSB";
		case 6:   return "(N)RPN Data Byte";
		case 7:   return "Main Volume";
		case 8:   return "Balance";
		case 10:  return "Panorama";
		case 11:  return "Expression";
		case 12:  return "Effect Control 1";
		case 13:  return "Effect Control 2";
		case 16:  return "General Purpose Controller 1";
		case 17:  return "General Purpose Controller 2";
		case 18:  return "General Purpose Controller 3";
		case 19:  return "General Purpose Controller 4";
		case 32:  return "Bank select LSB";
		case 63:  return "LSB for Controllers 0?31";
		case 64:  return "Hold 1";
		case 65:  return "Portamento";
		case 66:  return "Sostenuto";
		case 67:  return "Soft Pedal";
		case 68:  return "Legato Footswitch";
		case 69:  return "Hold 2";
		case 70:  return "Sound Controller 1 (Sound Variation)";
		case 71:  return "Sound Controller 2 (Harmonic Content)";
		case 72:  return "Sound Controller 3 (Release Time)";
		case 73:  return "Sound Controller 4 (Attack Time)";
		case 74:  return "Sound Controller 5 (Brightness)";
		case 75:  return "Sound Controller 6";
		case 76:  return "Sound Controller 7";
		case 77:  return "Sound Controller 8";
		case 78:  return "Sound Controller 9";
		case 79:  return "Sound Controller 10";
		case 80:  return "General Purpose 5";
		case 81:  return "General Purpose 6";
		case 82:  return "General Purpose 7";
		case 83:  return "General Purpose 8";
		case 84:  return "Portamento Control";
		case 91:  return "Effects 1 Depth";
		case 92:  return "Effects 2 Depth";
		case 93:  return "Effects 3 Depth";
		case 94:  return "Effects 4 Depth";
		case 95:  return "Effects 5 Depth";
		case 96:  return "Data Increment (N)RPN";
		case 97:  return "Data Decrement (N)RPN";
		case 98:  return "NRPN LSB";
		case 99:  return "NRPN MSB";
		case 100: return "RPN LSB";
		case 101: return "RPN MSB";
		case 120: return "All Sounds Off";
		case 121: return "Controller Reset";
		case 122: return "Local Control on/off";
		case 123: return "All Notes Off";
		case 124: return "Omni Off";
		case 125: return "Omni On";
		case 126: return "Mono On / Poly Off";
		case 127: return "Poly On / Mono Off";
		default: return "";
	}
}

} // private namespace


namespace QSampler {

ChannelFxForm::ChannelFxForm (
	Channel* pSamplerChannel, QWidget* pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags)
{
	m_ui.setupUi(this);

	m_pSamplerChannel = pSamplerChannel;

	m_pAudioDevice = NULL;

	FxSendsModel* pModel =
		new FxSendsModel(m_pSamplerChannel->channelID(), m_ui.SendsListView);
	m_ui.SendsListView->setModel(pModel);
#if QT_VERSION >= 0x040300
	m_ui.SendsListView->setSelectionRectVisible(true);
#endif

	const int iRowHeight = m_ui.audioRoutingTable->fontMetrics().height() + 4;
	m_ui.audioRoutingTable->verticalHeader()->setDefaultSectionSize(iRowHeight);
	m_ui.audioRoutingTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	ChannelRoutingModel* pRoutingModel =
		new ChannelRoutingModel(m_ui.audioRoutingTable);
	m_ui.audioRoutingTable->setModel(pRoutingModel);
	ChannelRoutingDelegate* pRoutingDelegate =
		new ChannelRoutingDelegate(m_ui.audioRoutingTable);
	m_ui.audioRoutingTable->setItemDelegate(pRoutingDelegate);
	m_ui.audioRoutingTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
//	m_ui.audioRoutingTable->verticalHeader()->hide();

	QAbstractButton* pApplyButton =
		m_ui.buttonBox->button(QDialogButtonBox::Apply);
	pApplyButton->setEnabled(false);
	pApplyButton->setIcon(QIcon(":/images/formEdit.png"));

	QAbstractButton* pCancelButton =
		m_ui.buttonBox->button(QDialogButtonBox::Cancel);
	pCancelButton->setIcon(QIcon(":/images/formRemove.png"));

	QAbstractButton* pOkButton =
		m_ui.buttonBox->button(QDialogButtonBox::Ok);
	pOkButton->setIcon(QIcon(":/images/formAccept.png"));

	QAbstractButton* pResetButton =
		m_ui.buttonBox->button(QDialogButtonBox::Reset);
	pResetButton->setEnabled(false);
	pResetButton->setToolTip("Revert all changes.");

	m_ui.destroyPushButton->setEnabled(false);

	m_ui.mainParametersGroupBox->setEnabled(false);
	m_ui.audioRoutingGroupBox->setEnabled(false);

	for (int i = 0; i < 128; ++i) {
		m_ui.depthCtrlComboBox->addItem(
			QString("[") + QString::number(i) + "] " + _midiControllerName(i)
		);
	}

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
	connect(
		m_ui.depthCtrlComboBox, SIGNAL(activated(int)),
		this, SLOT(onDepthCtrlChanged(int))
	);
	connect(
		m_ui.depthSpinBox, SIGNAL(valueChanged(int)),
		this, SLOT(onCurrentSendDepthChanged(int))
	);
	connect(
		pRoutingModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
		this, SLOT(updateTableCellRenderers(const QModelIndex&, const QModelIndex&))
	);
	connect(
		pRoutingModel, SIGNAL(modelReset()),
		this, SLOT(updateTableCellRenderers())
	);
	connect(
		pRoutingModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
		this, SLOT(onRoutingTableChanged())
	);
}

ChannelFxForm::~ChannelFxForm() {
	if (m_pAudioDevice) delete m_pAudioDevice;
}

void ChannelFxForm::onButtonClicked(QAbstractButton* button) {
	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	switch (m_ui.buttonBox->buttonRole(button)) {
		case QDialogButtonBox::AcceptRole:
		case QDialogButtonBox::ApplyRole:
			pModel->applyToSampler();
			// force a refresh of the parameter control elements
			onFxSendSelection(m_ui.SendsListView->currentIndex());
			break;
		case QDialogButtonBox::ResetRole:
			pModel->cleanRefresh();
			// force a refresh of the parameter control elements
			onFxSendSelection(m_ui.SendsListView->currentIndex());
			break;
		default: // to avoid gcc warnings
			break;
	}
}

void ChannelFxForm::onFxSendSelection(const QModelIndex& index) {
	const bool bValid = index.isValid();
	m_ui.destroyPushButton->setEnabled(bValid);
	m_ui.mainParametersGroupBox->setEnabled(bValid);
	m_ui.audioRoutingGroupBox->setEnabled(bValid);

	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	FxSend* pFxSend = pModel->fxSend(index);

	// clear routing model
	ChannelRoutingModel* pRoutingModel =
		(ChannelRoutingModel*) m_ui.audioRoutingTable->model();
	pRoutingModel->refresh(NULL, ChannelRoutingMap());
	pRoutingModel->routingMap().clear(); // Reset routing change map.
	if (m_pAudioDevice) {
		delete m_pAudioDevice;
		m_pAudioDevice = NULL;
	}

	if (!pFxSend) return;

	// update routing model
	if (m_pSamplerChannel->audioDevice() >= 0) {
		m_pAudioDevice =
			new Device(Device::Audio, m_pSamplerChannel->audioDevice());
		pRoutingModel->refresh(m_pAudioDevice, pFxSend->audioRouting());
	}

	m_ui.depthCtrlComboBox->setCurrentIndex(pFxSend->sendDepthMidiCtrl());
	m_ui.depthSpinBox->setValue(
		int(::round(pFxSend->currentDepth() * 100.0))
	);
}

void ChannelFxForm::onCreateFxSend() {
	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	pModel->addFxSend();
}

void ChannelFxForm::onDestroyFxSend() {
	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	pModel->removeFxSend(m_ui.SendsListView->currentIndex());
}

void ChannelFxForm::onDepthCtrlChanged(int iMidiCtrl) {
	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	const QModelIndex index = m_ui.SendsListView->currentIndex();
	FxSend* pFxSend = pModel->fxSend(index);
	if (!pFxSend) return;

	pFxSend->setSendDepthMidiCtrl(iMidiCtrl);
	pModel->onExternalModifiication(index);
}

void ChannelFxForm::onCurrentSendDepthChanged(int depthPercent) {
	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	const QModelIndex index = m_ui.SendsListView->currentIndex();
	FxSend* pFxSend = pModel->fxSend(index);
	if (!pFxSend) return;

	if (depthPercent == int( ::round(pFxSend->currentDepth() * 100.0) ))
		return; // nothing changed actually

	pFxSend->setCurrentDepth(double(depthPercent) / 100.0);
	pModel->onExternalModifiication(index);
}

void ChannelFxForm::onRoutingTableChanged() {
	ChannelRoutingModel* pRoutingModel =
		(ChannelRoutingModel*) m_ui.audioRoutingTable->model();
	if (pRoutingModel->routingMap().size() <= 0)
		return; // no changes

	FxSendsModel* pModel = (FxSendsModel*) m_ui.SendsListView->model();
	const QModelIndex index = m_ui.SendsListView->currentIndex();
	FxSend* pFxSend = pModel->fxSend(index);
	if (!pFxSend) {
		pRoutingModel->routingMap().clear(); // reset routing change map
		return;
	}

	ChannelRoutingMap routingMap = pRoutingModel->routingMap();
	for (
		ChannelRoutingMap::iterator iter = routingMap.begin();
		iter != routingMap.end(); ++iter
	) pFxSend->setAudioChannel(iter.key(), iter.value());

	pRoutingModel->routingMap().clear(); // reset routing change map

	pModel->onExternalModifiication(index);
}

void ChannelFxForm::updateTableCellRenderers() {
	ChannelRoutingModel* pRoutingModel =
		(ChannelRoutingModel*) m_ui.audioRoutingTable->model();
	const int rows = pRoutingModel->rowCount();
	const int cols = pRoutingModel->columnCount();
	updateTableCellRenderers(
		pRoutingModel->index(0, 0),
		pRoutingModel->index(rows - 1, cols - 1)
	);
}

void ChannelFxForm::updateTableCellRenderers (
	const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
	ChannelRoutingModel* pRoutingModel =
		(ChannelRoutingModel*) m_ui.audioRoutingTable->model();
	for (int r = topLeft.row(); r <= bottomRight.row(); r++) {
		for (int c = topLeft.column(); c <= bottomRight.column(); c++) {
			const QModelIndex index = pRoutingModel->index(r, c);
			m_ui.audioRoutingTable->openPersistentEditor(index);
		}
	}
}

} // namespace QSampler

// end of qsamplerChannelFxForm.cpp
