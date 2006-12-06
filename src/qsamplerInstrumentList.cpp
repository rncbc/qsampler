// qsamplerInstrumentList.cpp
//
/****************************************************************************
   Copyright (C) 2003-2005, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qsamplerInstrumentList.h"

#include "qsamplerInstrument.h"
#include "qsamplerInstrumentForm.h"

#include "qsamplerMainForm.h"

#include <qaction.h>
#include <qfileinfo.h>
#include <qpopupmenu.h>

// Needed for lroundf()
#include <math.h>


//----------------------------------------------------------------------
// class qsamplerInstrumentGroup -- custom group list view item.
//

// Constructors.
qsamplerInstrumentGroup::qsamplerInstrumentGroup (
	qsamplerInstrumentList *pListView, const QString& sName,
	QListViewItem *pItemAfter )
	: QListViewItem(pListView, pItemAfter ? pItemAfter : pListView->lastItem())
{
	QListViewItem::setRenameEnabled(0, true);

	QListViewItem::setPixmap(0, QPixmap::fromMimeSource("itemGroup.png"));
	QListViewItem::setText(0, sName);
}


qsamplerInstrumentGroup::qsamplerInstrumentGroup (
	qsamplerInstrumentGroup *pGroupItem, const QString& sName )
	: QListViewItem(pGroupItem, sName)
{
	QListViewItem::setRenameEnabled(0, true);

	QListViewItem::setPixmap(0, QPixmap::fromMimeSource("itemGroup.png"));
}


// Default destructor.
qsamplerInstrumentGroup::~qsamplerInstrumentGroup (void)
{
}


// Instance accessors.
void qsamplerInstrumentGroup::setName ( const QString& sName )
{
	QListViewItem::setText(0, sName);
}


QString qsamplerInstrumentGroup::name (void) const
{
	return QListViewItem::text(0);
}


qsamplerInstrumentGroup *qsamplerInstrumentGroup::groupItem (void) const
{
	QListViewItem *pParent = QListViewItem::parent();
	while (pParent && pParent->rtti() != qsamplerInstrumentList::Group)
		pParent = pParent->parent();
	return static_cast<qsamplerInstrumentGroup *> (pParent);
}


qsamplerInstrumentList *qsamplerInstrumentGroup::listView (void) const
{
	return static_cast<qsamplerInstrumentList *> (QListViewItem::listView());
}


// To show up whether its open or not.
void qsamplerInstrumentGroup::setOpen ( bool bOpen )
{
	// Set the proper pixmap of this...
	if (rtti() == qsamplerInstrumentList::Group) {
		QListViewItem::setPixmap(0, QPixmap::fromMimeSource(
			bOpen ? "itemGroupOpen.png" : "itemGroup.png"));
	}
	// Open it up...
	QListViewItem::setOpen(bOpen);

	// All ancestors should be also visible.
	if (bOpen && QListViewItem::parent())
		QListViewItem::parent()->setOpen(true);
}


// To virtually distinguish between list view items.
int qsamplerInstrumentGroup::rtti (void) const
{
	return qsamplerInstrumentList::Group;
}


//----------------------------------------------------------------------
// class qsamplerInstrumentItem -- custom file list view item.
//

// Constructors.
qsamplerInstrumentItem::qsamplerInstrumentItem (
	qsamplerInstrumentList *pListView,
	qsamplerInstrument *pInstrument,
	QListViewItem *pItemAfter )
	: qsamplerInstrumentGroup(pListView, pInstrument->name(), pItemAfter)
{
	m_pInstrument = pInstrument;

	update();
}

qsamplerInstrumentItem::qsamplerInstrumentItem (
	qsamplerInstrumentGroup *pGroupItem,
	qsamplerInstrument *pInstrument )
	: qsamplerInstrumentGroup(pGroupItem, pInstrument->name())
{
	m_pInstrument = pInstrument;

	update();
}


// Default destructor.
qsamplerInstrumentItem::~qsamplerInstrumentItem (void)
{
	if (m_pInstrument)
		delete m_pInstrument;
}


// To virtually distinguish between list view items.
int qsamplerInstrumentItem::rtti (void) const
{
	return qsamplerInstrumentList::Item;
}


// Payload accessor.
qsamplerInstrument *qsamplerInstrumentItem::instrument (void) const
{
	return m_pInstrument;
}


// Item refreshment.
void qsamplerInstrumentItem::update (void)
{
	QListViewItem::setPixmap(0, QPixmap::fromMimeSource("itemFile.png"));

	const QString s = "-";
	if (m_pInstrument) {
		setText(0, m_pInstrument->name());
		setText(1, QString::number(m_pInstrument->bank()));
		setText(2, QString::number(m_pInstrument->program()));
		setText(3, m_pInstrument->engineName());
		setText(4, QFileInfo(m_pInstrument->instrumentFile()).fileName());
		setText(5, QString::number(m_pInstrument->instrumentNr()));
		setText(6, QString::number(::lroundf(100.0f * m_pInstrument->volume())));
		QString sLoadMode = s;
		switch (m_pInstrument->loadMode()) {
		case 3:
			sLoadMode = QObject::tr("Persistent");
			break;
		case 2:
			sLoadMode = QObject::tr("On Demand Hold");
			break;
		case 1:
			sLoadMode = QObject::tr("On Demand");
			break;
		}
		setText(7, sLoadMode);
	} else {
		for (int i = 0; i < listView()->columns(); i++)
			setText(i, s);
	}
}


//----------------------------------------------------------------------------
// qsamplerInstrumentList -- MIDI instrument list view.
//

// Constructor.
qsamplerInstrumentList::qsamplerInstrumentList (
	QWidget *pParent, const char *pszName )
	: QListView(pParent, pszName)
{
//  QListView::setRootIsDecorated(true);
	QListView::setResizeMode(QListView::NoColumn);
//	QListView::setAcceptDrops(true);
	QListView::setDragAutoScroll(true);
	QListView::setSizePolicy(
		QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
//	QListView::setShowToolTips(false);
	QListView::setSortColumn(-1);

	QListView::addColumn(tr("Name"));
	QListView::addColumn(tr("Bank"));
	QListView::addColumn(tr("Prog"));
	QListView::addColumn(tr("Engine"));
	QListView::addColumn(tr("File"));
	QListView::addColumn(tr("Nr"));
	QListView::addColumn(tr("Vol"));
	QListView::addColumn(tr("Mode"));

	QListView::setColumnAlignment(1, Qt::AlignHCenter);	// Bank
	QListView::setColumnAlignment(2, Qt::AlignHCenter);	// Prog
	QListView::setColumnAlignment(5, Qt::AlignHCenter);	// Nr
	QListView::setColumnAlignment(6, Qt::AlignHCenter);	// Vol

	QListView::setColumnWidth(0, 60);	// Name
	QListView::setColumnWidth(0, 120);	// File

	m_pNewGroupAction = new QAction(tr("New &Group"), tr("Ctrl+G"), this);
	m_pNewItemAction  = new QAction(tr("New &Instrument..."), tr("Ctrl+I"), this);
	m_pEditItemAction = new QAction(tr("&Edit..."), tr("Ctrl+E"), this);
	m_pRenameAction   = new QAction(tr("&Rename"), tr("Ctrl+R"), this);
	m_pDeleteAction   = new QAction(tr("&Delete"), tr("Ctrl+D"), this);
	m_pRefreshAction  = new QAction(tr("Re&fresh"), tr("Ctrl+F"), this);

	QObject::connect(m_pNewGroupAction,
		SIGNAL(activated()),
		SLOT(newGroupSlot()));
	QObject::connect(m_pNewItemAction,
		SIGNAL(activated()),
		SLOT(newItemSlot()));
	QObject::connect(m_pEditItemAction,
		SIGNAL(activated()),
		SLOT(editItemSlot()));
	QObject::connect(m_pRenameAction,
		SIGNAL(activated()),
		SLOT(renameSlot()));
	QObject::connect(m_pDeleteAction,
		SIGNAL(activated()),
		SLOT(deleteSlot()));
	QObject::connect(m_pRefreshAction,
		SIGNAL(activated()),
		SLOT(refresh()));

	QObject::connect(this,
		SIGNAL(selectionChanged()),
		SLOT(selectionChangedSlot()));
	QObject::connect(this,
		SIGNAL(doubleClicked(QListViewItem*, const QPoint&, int)),
		SLOT(activatedSlot(QListViewItem*)));
	QObject::connect(this,
		SIGNAL(returnPressed(QListViewItem*)),
		SLOT(activatedSlot(QListViewItem*)));
	QObject::connect(this,
		SIGNAL(itemRenamed(QListViewItem*,int)),
		SLOT(renamedSlot(QListViewItem*)));

	selectionChangedSlot();
}


// Default destructor.
qsamplerInstrumentList::~qsamplerInstrumentList (void)
{
	delete m_pNewGroupAction;
	delete m_pNewItemAction;
	delete m_pEditItemAction;
	delete m_pRenameAction;
	delete m_pDeleteAction;
}


// Add a new instrument item, optionally under a given group.
qsamplerInstrumentItem *qsamplerInstrumentList::addItem (
	qsamplerInstrument *pInstrument,
	qsamplerInstrumentGroup *pParentGroup )
{
	qsamplerInstrumentItem *pItem = findItem(pInstrument);
	if (pItem == NULL) {
		if (pParentGroup)
			pItem = new qsamplerInstrumentItem(pParentGroup, pInstrument);
		else
			pItem = new qsamplerInstrumentItem(this, pInstrument);
	}
	QListView::setSelected(pItem, true);
	return pItem;
}


// Add a new instrument group, optionally under another group.
qsamplerInstrumentGroup *qsamplerInstrumentList::addGroup (
	const QString& sName, qsamplerInstrumentGroup *pParentGroup )
{
	qsamplerInstrumentGroup *pGroup = findGroup(sName);
	if (pGroup == NULL) {
		if (pParentGroup)
			pGroup = new qsamplerInstrumentGroup(pParentGroup, sName);
		else
			pGroup = new qsamplerInstrumentGroup(this, sName);
	}
	QListView::setSelected(pGroup, true);
	return pGroup;
}


// Find a group item, given its name.
qsamplerInstrumentGroup *qsamplerInstrumentList::findGroup (
	const QString& sName ) const
{
	// Iterate all over the place to search for the group.
	QListViewItemIterator iter((QListView *) this);
	while (iter.current()) {
		QListViewItem *pItem = iter.current();
		if (pItem->rtti() == Group && pItem->text(0) == sName)
			return static_cast<qsamplerInstrumentGroup *> (pItem);
		++iter;
	}
	// Not found.
	return NULL;
}


// Find a file item, given its name.
qsamplerInstrumentItem *qsamplerInstrumentList::findItem (
	qsamplerInstrument *pInstrument ) const
{
	if (pInstrument == NULL)
		return NULL;

	// Iterate all over the place to search for the group.
	QListViewItemIterator iter((QListView *) this);
	while (iter.current()) {
		QListViewItem *pListItem = iter.current();
		if (pListItem->rtti() == Item) {
			qsamplerInstrumentItem *pItem
				= static_cast<qsamplerInstrumentItem *> (pListItem);
			if (pItem && pItem->instrument()
				&& pItem->instrument()->bank() == pInstrument->bank()
				&& pItem->instrument()->program() == pInstrument->program())
				return pItem;
		}
		++iter;
	}
	// Not found.
	return NULL;
}


// Find and return the nearest group item...
qsamplerInstrumentGroup *qsamplerInstrumentList::groupItem (
	QListViewItem *pItem ) const
{
	while (pItem && pItem->rtti() != Group)
		pItem = pItem->parent();
	return static_cast<qsamplerInstrumentGroup *> (pItem);
}


// Add a new group item below the current one.
void qsamplerInstrumentList::newGroupSlot (void)
{
	qsamplerInstrumentGroup *pParentGroup
		= groupItem(QListView::selectedItem());
	qsamplerInstrumentGroup *pNewGroup
		= addGroup(tr("New Group"), pParentGroup);
	if (pParentGroup)
		pParentGroup->setOpen(true);
	if (pNewGroup)
		pNewGroup->startRename(0);

	selectionChangedSlot();
}


// Add a new instrument item below the current one.
void qsamplerInstrumentList::newItemSlot (void)
{
	qsamplerInstrument *pInstrument = new qsamplerInstrument();

	qsamplerInstrumentForm form(this);
	form.setup(pInstrument);
	if (!form.exec()) {
		delete pInstrument;
		return;
	}

	// Check it there's already one for the same key (bank, program)
	qsamplerInstrumentItem *pItem = findItem(pInstrument);
	if (pItem)
		delete pItem;

	pInstrument->map();
	emit instrumentsChanged();

	qsamplerInstrumentGroup *pParentGroup
		= groupItem(QListView::selectedItem());
	addItem(pInstrument, pParentGroup);
	if (pParentGroup)
		pParentGroup->setOpen(true);

	selectionChangedSlot();
}


// Edit current item below the current one.
void qsamplerInstrumentList::editItemSlot (void)
{
	QListViewItem *pListItem = QListView::selectedItem();
	if (pListItem == NULL)
		return;
	if (pListItem->rtti() == Item) {
		qsamplerInstrumentItem *pItem
			= static_cast<qsamplerInstrumentItem *> (pListItem);
		if (pItem && pItem->instrument()) {
			qsamplerInstrumentForm form(this);
			form.setup(pItem->instrument());
			if (form.exec()) {
				pItem->instrument()->map();
				emit instrumentsChanged();
				pItem->update();
			}
		}
	}

	selectionChangedSlot();
}


// Rename current group/item.
void qsamplerInstrumentList::renameSlot (void)
{
	QListViewItem *pListItem = QListView::selectedItem();
	if (pListItem)
		pListItem->startRename(0);

	selectionChangedSlot();
}


// Remove current group/item.
void qsamplerInstrumentList::deleteSlot (void)
{
	QListViewItem *pListItem = QListView::selectedItem();
	if (pListItem) {
		if (pListItem->rtti() == Item) {
			qsamplerInstrumentItem *pItem
				= static_cast<qsamplerInstrumentItem *> (pListItem);
			if (pItem && pItem->instrument()) {
				pItem->instrument()->unmap();
				emit instrumentsChanged();
			}
		}
		delete pListItem;
	}

	selectionChangedSlot();
}


// In-place selection slot.
void qsamplerInstrumentList::selectionChangedSlot (void)
{
	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	QListViewItem *pListItem = QListView::selectedItem();
	bool bEnabled = (pMainForm && pMainForm->client());
	m_pNewItemAction->setEnabled(bEnabled);
	bEnabled = (bEnabled && pListItem != NULL);
	m_pEditItemAction->setEnabled(bEnabled && pListItem->rtti() == Item);
	m_pRenameAction->setEnabled(bEnabled);
	m_pDeleteAction->setEnabled(bEnabled);
}


// In-place activation slot.
void qsamplerInstrumentList::activatedSlot ( QListViewItem *pListItem )
{
	// FIXME: Hope the list view item is the one selected.
	if (pListItem->rtti() == Item)
		editItemSlot();
}


// In-place aliasing slot.
void qsamplerInstrumentList::renamedSlot ( QListViewItem *pListItem )
{
	if (pListItem->rtti() == Item) {
		qsamplerInstrumentItem *pItem
			= static_cast<qsamplerInstrumentItem *> (pListItem);
		if (pItem && pItem->instrument()) {
			pItem->instrument()->setName(pListItem->text(0));
			pItem->instrument()->map();
			emit instrumentsChanged();
			pItem->update();
		}
	}
}


// Context menu request event handler.
void qsamplerInstrumentList::contextMenuEvent (
	QContextMenuEvent *pContextMenuEvent )
{
	QPopupMenu menu(this);

	// Construct context menu.
	m_pNewItemAction->addTo(&menu);
//	m_pNewGroupAction->addTo(&menu);
	menu.insertSeparator();
	m_pEditItemAction->addTo(&menu);
	m_pRenameAction->addTo(&menu);
	m_pDeleteAction->addTo(&menu);
	menu.insertSeparator();
	m_pRefreshAction->addTo(&menu);

	menu.exec(pContextMenuEvent->globalPos());
}


// General reloader.
void qsamplerInstrumentList::refresh (void)
{
	clear();

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	qsamplerInstrumentItem *pItem = NULL;
	lscp_midi_instrument_t *pInstrs
		= ::lscp_list_midi_instruments(pMainForm->client());
	for (int iInstr = 0; pInstrs && pInstrs[iInstr].program >= 0; ++iInstr) {
		int iBank = (pInstrs[iInstr].bank_msb << 7) | pInstrs[iInstr].bank_lsb;
		int iProgram = pInstrs[iInstr].program;
		qsamplerInstrument *pInstrument
			= new qsamplerInstrument(iBank, iProgram);
		if (pInstrument->get())
			pItem = new qsamplerInstrumentItem(this, pInstrument, pItem);
	}

	if (pInstrs == NULL && ::lscp_client_get_errno(pMainForm->client())) {
		pMainForm->appendMessagesClient("lscp_list_midi_instruments");
		pMainForm->appendMessagesError(tr("Could not get current list of MIDI instrument mappings.\n\nSorry."));
	}

	selectionChangedSlot();
}


// end of qsamplerInstrumentList.cpp

