// qsamplerInstrumentList.cpp
//
/****************************************************************************
   Copyright (C) 2003-2007, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qsamplerOptions.h"
#include "qsamplerMainForm.h"

#include <qmessagebox.h>
#include <qaction.h>
#include <qfileinfo.h>
#include <qpopupmenu.h>

// Needed for lroundf()
#include <math.h>

#ifndef CONFIG_ROUND
static inline long lroundf ( float x )
{
	if (x >= 0.0f)
		return long(x + 0.5f);
	else
		return long(x - 0.5f);
}
#endif


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
		setText(1, QString::number(m_pInstrument->map()));
		setText(2, QString::number(m_pInstrument->bank()));
		setText(3, QString::number(m_pInstrument->prog() + 1));
		setText(4, m_pInstrument->engineName());
		setText(5, QFileInfo(m_pInstrument->instrumentFile()).fileName());
		setText(6, QString::number(m_pInstrument->instrumentNr()));
		setText(7, QString::number(::lroundf(100.0f * m_pInstrument->volume())));
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
		setText(8, sLoadMode);
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
	m_iMidiMap = LSCP_MIDI_MAP_ALL;

//  QListView::setRootIsDecorated(true);
	QListView::setAllColumnsShowFocus(true);
	QListView::setResizeMode(QListView::NoColumn);
//	QListView::setAcceptDrops(true);
	QListView::setDragAutoScroll(true);
	QListView::setSizePolicy(
		QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
//	QListView::setShowToolTips(false);
	QListView::setSortColumn(-1);

	QListView::addColumn(tr("Name"));
	QListView::addColumn(tr("Map"));
	QListView::addColumn(tr("Bank"));
	QListView::addColumn(tr("Prog"));
	QListView::addColumn(tr("Engine"));
	QListView::addColumn(tr("File"));
	QListView::addColumn(tr("Nr"));
	QListView::addColumn(tr("Vol"));
	QListView::addColumn(tr("Mode"));

	QListView::setColumnAlignment(1, Qt::AlignHCenter);	// Map
	QListView::setColumnAlignment(2, Qt::AlignHCenter);	// Bank
	QListView::setColumnAlignment(3, Qt::AlignHCenter);	// Prog
	QListView::setColumnAlignment(6, Qt::AlignHCenter);	// Nr
	QListView::setColumnAlignment(7, Qt::AlignHCenter);	// Vol

	QListView::setColumnWidth(0, 120);	// Name
	QListView::setColumnWidth(5, 240);	// File

	m_pNewGroupAction = new QAction(
		QIconSet(QPixmap::fromMimeSource("itemGroupNew.png")),
		tr("New &Group"), tr("Ctrl+G"), this);
	m_pNewItemAction  = new QAction(
		QIconSet(QPixmap::fromMimeSource("itemNew.png")),
		tr("New &Instrument..."), tr("Ins"), this);
	m_pEditItemAction = new QAction(
		QIconSet(QPixmap::fromMimeSource("formEdit.png")),
		tr("&Edit..."), tr("Enter"), this);
	m_pRenameAction   = new QAction(tr("&Rename"), tr("F2"), this);
	m_pDeleteAction   = new QAction(
		QIconSet(QPixmap::fromMimeSource("formRemove.png")),
		tr("&Delete"), tr("Del"), this);
	m_pRefreshAction  = new QAction(
		QIconSet(QPixmap::fromMimeSource("formRefresh.png")),
		tr("Re&fresh"), tr("F5"), this);

	m_pNewGroupAction->setToolTip(tr("New Group"));
	m_pNewItemAction->setToolTip(tr("New Instrument"));
	m_pEditItemAction->setToolTip(tr("Edit"));
	m_pRenameAction->setToolTip(tr("Rename"));
	m_pDeleteAction->setToolTip(tr("Delete"));
	m_pRefreshAction->setToolTip(tr("Refresh"));

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
	// Check it there's already one instrument item
	// with the very same key (bank, program);
	// if yes, just remove it without prejudice...
	qsamplerInstrumentItem *pItem = findItem(pInstrument);
	if (pItem)
		delete pItem;

	// Add the new item under proper group one, if any...
	if (pParentGroup) {
		pParentGroup->setOpen(true);
		pItem = new qsamplerInstrumentItem(pParentGroup, pInstrument);
	} else {
		pItem = new qsamplerInstrumentItem(this, pInstrument);
	}

	// Set it as current selection...
	QListView::setSelected(pItem, true);

	return pItem;
}


// Add a new instrument group, optionally under another group.
qsamplerInstrumentGroup *qsamplerInstrumentList::addGroup (
	const QString& sName, qsamplerInstrumentGroup *pParentGroup )
{
	qsamplerInstrumentGroup *pGroup = findGroup(sName);
	if (pGroup == NULL) {
		if (pParentGroup) {
			pParentGroup->setOpen(true);
			pGroup = new qsamplerInstrumentGroup(pParentGroup, sName);
		} else {
			pGroup = new qsamplerInstrumentGroup(this, sName);
		}
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
				&& pItem->instrument()->map()  == pInstrument->map()
				&& pItem->instrument()->bank() == pInstrument->bank()
				&& pItem->instrument()->prog() == pInstrument->prog())
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
	qsamplerInstrumentGroup *pNewGroup
		= addGroup(tr("New Group"), groupItem(QListView::selectedItem()));
	if (pNewGroup)
		pNewGroup->startRename(0);

	selectionChangedSlot();
}


// Map selector.
void qsamplerInstrumentList::setMidiMap ( int iMidiMap )
{
	if (iMidiMap < 0)
		iMidiMap = LSCP_MIDI_MAP_ALL;

	m_iMidiMap = iMidiMap;
}

int qsamplerInstrumentList::midiMap (void) const
{
	return m_iMidiMap;
}


// List actions accessors.
QAction *qsamplerInstrumentList::newGroupAction (void) const
{
	return m_pNewGroupAction;
}

QAction *qsamplerInstrumentList::newItemAction (void) const
{
	return m_pNewItemAction;
}

QAction *qsamplerInstrumentList::editItemAction (void) const
{
	return m_pEditItemAction;
}

QAction *qsamplerInstrumentList::renameAction (void) const
{
	return m_pRenameAction;
}

QAction *qsamplerInstrumentList::deleteAction (void) const
{
	return m_pDeleteAction;
}

QAction *qsamplerInstrumentList::refreshAction (void) const
{
	return m_pRefreshAction;
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

	// Commit...
	pInstrument->mapInstrument();
	// add new item to the tree...
	addItem(pInstrument, groupItem(QListView::selectedItem()));
	// Notify we've changes...
	emit instrumentsChanged();

	selectionChangedSlot();
}


// Edit current item below the current one.
void qsamplerInstrumentList::editItemSlot (void)
{
	QListViewItem *pListItem = QListView::selectedItem();
	if (pListItem == NULL)
		return;
	if (pListItem->rtti() == Item) {
		qsamplerInstrument *pInstrument = NULL;
		qsamplerInstrumentItem *pItem
			= static_cast<qsamplerInstrumentItem *> (pListItem);
		if (pItem)
			pInstrument = pItem->instrument();
		if (pInstrument) {
			// Save current key values...
			int iMap  = pInstrument->map();
			int iBank = pInstrument->bank();
			int iProg = pInstrument->prog();
			// Do the edit dance...
			qsamplerInstrumentForm form(this);
			form.setup(pInstrument);
			if (form.exec()) {
				// Commit...
				pInstrument->mapInstrument();
				// Check whether we changed instrument key...
				if (iMap  == pInstrument->map()  &&
					iBank == pInstrument->bank() &&
					iProg == pInstrument->prog()) {
					// just update tree item...
					pItem->update();
				} else {
					// Change item tree, whether applicable...
					if (m_iMidiMap < 0 || m_iMidiMap == pInstrument->map()) {
						// Add new brand item into view...
						addItem(pInstrument, groupItem(pListItem));
					} else {
						// Just remove/hide old one.
						delete pItem;
					}
				}
				// Notify we've changes...
				emit instrumentsChanged();
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
	if (pListItem == NULL)
		return;

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	// Prompt user if this is for real...
	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions && pOptions->bConfirmRemove) {
		if (QMessageBox::warning(this,
			QSAMPLER_TITLE ": " + tr("Warning"),
			tr("Delete %1:\n\n"
			"%2\n\n"
			"Are you sure?")
			.arg(pListItem->rtti() == Item ? tr("instrument") : tr("group"))
			.arg(pListItem->text(0)),
			tr("OK"), tr("Cancel")) > 0)
			return;
	}

	// Unmap instrument entry...
	if (pListItem->rtti() == Item) {
		qsamplerInstrumentItem *pItem
			= static_cast<qsamplerInstrumentItem *> (pListItem);
		if (pItem && pItem->instrument()) {
			pItem->instrument()->unmapInstrument();
			emit instrumentsChanged();
		}
	}

	// Do it for real...
	delete pListItem;

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
			pItem->instrument()->mapInstrument();
			emit instrumentsChanged();
			pItem->update();
		}
	}
}


// Context menu request event handler.
void qsamplerInstrumentList::contextMenuEvent (
	QContextMenuEvent *pContextMenuEvent )
{
	if (!m_pNewItemAction->isEnabled())
		return;

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
		= ::lscp_list_midi_instruments(pMainForm->client(), m_iMidiMap);
	for (int iInstr = 0; pInstrs && pInstrs[iInstr].map >= 0; ++iInstr) {
		int iMap  = pInstrs[iInstr].map;
		int iBank = pInstrs[iInstr].bank;
		int iProg = pInstrs[iInstr].prog;
		qsamplerInstrument *pInstrument
			= new qsamplerInstrument(iMap, iBank, iProg);
		if (pInstrument->getInstrument())
			pItem = new qsamplerInstrumentItem(this, pInstrument, pItem);
	}

	if (pInstrs == NULL && ::lscp_client_get_errno(pMainForm->client())) {
		pMainForm->appendMessagesClient("lscp_list_midi_instruments");
		pMainForm->appendMessagesError(tr("Could not get current list of MIDI instrument mappings.\n\nSorry."));
	}

	selectionChangedSlot();
}


// end of qsamplerInstrumentList.cpp

