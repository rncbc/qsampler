// qsamplerFxSendList.cpp
//
/****************************************************************************
   Copyright (C) 2010-2012, rncbc aka Rui Nuno Capela. All rights reserved.
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
#include "qsamplerFxSendsModel.h"
#include "qsamplerFxSend.h"

#include <QBrush>
#include <QIcon>
#include <QFont>

namespace QSampler {

FxSendsModel::FxSendsModel(int SamplerChannelID, QObject* pParent) : QAbstractListModel(pParent) {
	m_SamplerChannelID = SamplerChannelID;
	cleanRefresh();
}

int FxSendsModel::rowCount(const QModelIndex& /*parent*/) const {
	return m_FxSends.size();
}

QVariant FxSendsModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return QVariant();

	switch (role) {
		case Qt::DisplayRole:
			return m_FxSends[index.row()].name();
			break;
		case Qt::ToolTipRole:
			if (m_FxSends[index.row()].deletion())
				return QString(
					"Scheduled for deletion. Click on 'Apply' to actually "
					"destroy FX Send."
				);
			return (m_FxSends[index.row()].isNew()) ?
						QString(
							"New FX send. Click on 'Apply' to actually "
							"perform creation."
						) :
						QString("FX Send ID ") +
						QString::number(m_FxSends[index.row()].id());
			break;
		case Qt::ForegroundRole:
			if (m_FxSends[index.row()].deletion())
				return QBrush(Qt::red);
			if (m_FxSends[index.row()].isNew())
				return QBrush(Qt::green);
			break;
		case Qt::DecorationRole:
			if (m_FxSends[index.row()].deletion())
				return QIcon(":/images/formRemove.png");
			if (m_FxSends[index.row()].isNew())
				return QIcon(":/images/itemNew.png");
			if (m_FxSends[index.row()].isModified())
				return QIcon(":/images/formEdit.png");
			return QIcon(":/images/itemFile.png");
		case Qt::FontRole: {
			if (m_FxSends[index.row()].isModified()) {
				QFont font;
				font.setBold(true);
				return font;
			}
			break;
		}
		default:
			return QVariant();
	}
	return QVariant();
}

bool FxSendsModel::setData(
	const QModelIndex& index, const QVariant& value, int /*role*/)
{
	if (!index.isValid())
		return false;

	m_FxSends[index.row()].setName(value.toString());
	emit dataChanged(index, index);
	emit fxSendsDirtyChanged(true);

	return true;
}

QVariant FxSendsModel::headerData(int section, Qt::Orientation /*orientation*/,
	int role) const
{
	if (role == Qt::DisplayRole && section == 0)
		return QString("FX Send Name");
	else
		return QVariant();
}

Qt::ItemFlags FxSendsModel::flags(const QModelIndex& /*index*/) const {
	return Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

FxSend* FxSendsModel::addFxSend() {
#if CONFIG_FXSEND
	FxSend fxSend(m_SamplerChannelID);
	fxSend.setName("New FX Send");
	m_FxSends.push_back(fxSend);
	QModelIndex index = createIndex(m_FxSends.size() - 1, 0);
#if QT_VERSION < 0x050000
	QAbstractListModel::reset();
#else
	QAbstractListModel::beginResetModel();
	QAbstractListModel::endResetModel();
#endif
	emit fxSendsDirtyChanged(true);
	return &m_FxSends.last();
#else // CONFIG_FXSEND
	return NULL;
#endif // CONFIG_FXSEND
}

FxSend* FxSendsModel::fxSend(const QModelIndex& index) {
	if (!index.isValid())
		return NULL;

	return &m_FxSends[index.row()];
}

void FxSendsModel::removeFxSend(const QModelIndex& index) {
	FxSend* pFxSend = fxSend(index);
	if (!pFxSend) return;
	pFxSend->setDeletion(true);
#if QT_VERSION < 0x050000
	QAbstractListModel::reset();
#else
	QAbstractListModel::beginResetModel();
	QAbstractListModel::endResetModel();
#endif
	emit fxSendsDirtyChanged(true);
}

void FxSendsModel::cleanRefresh() {
	m_FxSends.clear();
	QList<int> sends = FxSend::allFxSendsOfSamplerChannel(m_SamplerChannelID);
	for (int i = 0; i < sends.size(); ++i) {
		const int iFxSendId = sends[i]; 
		FxSend fxSend(m_SamplerChannelID, iFxSendId);
		fxSend.getFromSampler();
		m_FxSends.push_back(fxSend);
	}
#if QT_VERSION < 0x050000
	QAbstractListModel::reset();
#else
	QAbstractListModel::beginResetModel();
	QAbstractListModel::endResetModel();
#endif
	emit fxSendsDirtyChanged(false);
}

void FxSendsModel::onExternalModifiication(const QModelIndex& index) {
	if (!index.isValid()) return;
	emit dataChanged(index, index);
	emit fxSendsDirtyChanged(true);
}

void FxSendsModel::applyToSampler() {
	for (int i = 0; i < m_FxSends.size(); ++i)
		m_FxSends[i].applyToSampler();

	// make a clean refresh
	// (throws out all FxSend objects marked for deletion)
	cleanRefresh();
}

} // namespace QSampler

// end of qsamplerFxSendList.cpp
