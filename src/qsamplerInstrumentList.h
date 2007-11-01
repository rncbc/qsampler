// qsamplerInstrumentList.h
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

#ifndef __qsamplerInstrumentList_h
#define __qsamplerInstrumentList_h

#include <QListWidget>
#include <QListWidgetItem>
//#include <qheader.h>
#include <QAbstractTableModel>
#include <QItemDelegate>

#include <lscp/client.h>

#include <string.h>

#include "qsamplerInstrument.h"

// Forward declarations.
//class qsamplerInstrument;
//class qsamplerInstrumentList;

class QAction;


//----------------------------------------------------------------------
// class qsamplerInstrumentGroup -- custom group list view item.
//

class qsamplerInstrumentGroup : public QListWidgetItem
{
public:

	// Constructors.
	qsamplerInstrumentGroup(QListWidget *pListView,
		const QString& sName, QListWidgetItem *pItemAfter = NULL);
	qsamplerInstrumentGroup(qsamplerInstrumentGroup *pGroupItem,
		const QString& sName);
	// Default destructor.
	virtual ~qsamplerInstrumentGroup();

	// Instance accessors.
	void setName(const QString& sName);
	QString name() const;

	QListWidget  *listView() const;
	qsamplerInstrumentGroup *groupItem() const;

	// To show up whether its open or not.
	virtual void setOpen(bool bOpen);

	// To virtually distinguish between list view items.
	virtual int rtti() const;
};


//----------------------------------------------------------------------
// class qsamplerInstrumentItem -- custom file list view item.
//

class qsamplerInstrumentItem : public qsamplerInstrumentGroup
{
public:

	// Constructors.
	qsamplerInstrumentItem(QListWidget *pListView,
		qsamplerInstrument *pInstrument,
		QListWidgetItem *pItemAfter = NULL);
	qsamplerInstrumentItem(qsamplerInstrumentGroup *pGroupItem,
		qsamplerInstrument *pInstrument);
	// Default destructor.
	virtual ~qsamplerInstrumentItem();

	// To virtually distinguish between list view items.
	virtual int rtti() const;

	// Payload accessor.
	qsamplerInstrument *instrument() const;

	// View refreshment.
	void update();

private:

	// File item full path.
	qsamplerInstrument *m_pInstrument;
};


//----------------------------------------------------------------------------
// qsamplerInstrumentList -- MIDI instrument list view.
//

#if 0
class qsamplerInstrumentList : public QListView
{
	Q_OBJECT

public:

	// Constructor.
	qsamplerInstrumentList(QWidget *pParent, const char *pszName = NULL);
	// Default destructor.
	~qsamplerInstrumentList();

	// QListViewItem::rtti() return values.
	enum ItemType { Group = 1001, Item = 1002 };

	// Add a new group/file item, optionally under a given group.
	qsamplerInstrumentGroup *addGroup(const QString& sName,
		qsamplerInstrumentGroup *pParentGroup = NULL);
	qsamplerInstrumentItem *addItem(
		qsamplerInstrument *pInstrument,
		qsamplerInstrumentGroup *pParentGroup = NULL);

	// Find a group/file item, given its name.
	qsamplerInstrumentGroup *findGroup(const QString& sName) const;
	qsamplerInstrumentItem  *findItem(
		qsamplerInstrument *pInstrument) const;

	// Map selector.
	void setMidiMap(int iMidiMap);
	int midiMap() const;

	// List actions accessors.
	QAction *newGroupAction() const;
	QAction *newItemAction() const;
	QAction *editItemAction() const;
	QAction *renameAction() const;
	QAction *deleteAction() const;
	QAction *refreshAction() const;

signals:

	// Instrument map/session change signal.
	void instrumentsChanged();

public slots:

	// General reloader.
	void refresh();

protected slots:

	// Add a new group item below the current one.
	void newGroupSlot();
	// Add a instrument item below the current one.
	void newItemSlot();
	// Change current instrument item.
	void editItemSlot();
	// Rename current group/item.
	void renameSlot();
	// Remove current group/item.
	void deleteSlot();

	// In-place selection slot.
	void selectionChangedSlot();

	// In-place activation slot.
	void activatedSlot(QListWidgetItem *pListItem);

	// In-place aliasing slot.
	void renamedSlot(QListWidgetItem *pItem);

protected:

	// Find and return the nearest group item...
	qsamplerInstrumentGroup *groupItem(QListWidgetItem *pListItem) const;

	// Context menu request event handler.
	void contextMenuEvent(QContextMenuEvent *pContextMenuEvent);

private:

	// List view actions.
	QAction *m_pNewGroupAction;
	QAction *m_pNewItemAction;
	QAction *m_pEditItemAction;
	QAction *m_pRenameAction;
	QAction *m_pDeleteAction;
	QAction *m_pRefreshAction;

	// Current map selection.
	int m_iMidiMap;
};
#endif

class MidiInstrumentsModel : public QAbstractTableModel {
Q_OBJECT
public:
    MidiInstrumentsModel(QObject* parent = 0);

    // overridden methods from subclass(es)
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    qsamplerInstrument* addInstrument(int iMap = 0, int iBank = -1, int iProg = -1);

    // Map selector.
    void setMidiMap(int iMidiMap);
    int midiMap() const;

signals:
    // Instrument map/session change signal.
    void instrumentsChanged();

public slots:
    // General reloader.
    void refresh();

private:
    typedef QMap<int, QList<qsamplerInstrument> > InstrumentsMap;

    // Current map selection.
    int m_iMidiMap;

    InstrumentsMap instruments;
};

class MidiInstrumentsDelegate : public QItemDelegate {
Q_OBJECT
public:
    MidiInstrumentsDelegate(QObject* parent = 0);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const;

    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const;

    void updateEditorGeometry(QWidget* editor,
        const QStyleOptionViewItem& option, const QModelIndex& index) const;
};


#endif  // __qsamplerInstrumentList_h


// end of qsamplerInstrumentList.h
