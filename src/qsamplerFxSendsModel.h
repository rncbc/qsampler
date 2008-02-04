// qsamplerFxSendList.h
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

#ifndef __qsamplerFxSendList_h
#define __qsamplerFxSendList_h

#include <QAbstractListModel>
#include <lscp/client.h>

#include "qsamplerFxSend.h"

namespace QSampler {

class FxSendsModel : public QAbstractListModel
{
	Q_OBJECT
public:
	FxSendsModel(int SamplerChannelID, QObject* pParent = NULL);

	// Overridden methods from subclass(es)
	int rowCount(const QModelIndex& parent) const;
	QVariant data(const QModelIndex& index, int role) const;
	bool setData(const QModelIndex& index,
		const QVariant& value, int role = Qt::EditRole);
	QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex& index) const;

	// Make the following method public
	QAbstractListModel::reset;

	// Own methods
	FxSend* addFxSend();
	FxSend* fxSend(const QModelIndex& index);
	void removeFxSend(const QModelIndex& index);

signals:
	void fxSendsDirtyChanged(bool);

public slots:
	void cleanRefresh();
	void applyToSampler();

private:
	typedef QList<FxSend> FxSendsList;

	int         m_SamplerChannelID;
	FxSendsList m_FxSends;
};

} // namespace QSampler

#endif // __qsamplerFxSendList_h

// end of qsamplerFxSendList.h
