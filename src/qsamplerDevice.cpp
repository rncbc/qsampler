// qsamplerDevice.cpp
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

#include "qsamplerDevice.h"

#include "qsamplerMainForm.h"
#include "qsamplerDeviceForm.h"

#include "config.h"


//-------------------------------------------------------------------------
// qsamplerDeviceParameterTable - Device parameter view table.
//

// Constructor.
qsamplerDeviceParameterTable::qsamplerDeviceParameterTable ( QWidget *pParent, const char *pszName )
	: QTable(pParent, pszName)
{
	m_pClient = NULL;

	// Set fixed number of columns.
	QTable::setNumCols(5);
	QTable::setShowGrid(false);
	QTable::setSorting(false);
	QTable::setFocusStyle(QTable::FollowStyle);
	QTable::setSelectionMode(QTable::NoSelection);
	// No vertical header.
	QTable::verticalHeader()->hide();
	QTable::setLeftMargin(0);
	// Initialize the fixed table column headings.
	QHeader *pHeader = QTable::horizontalHeader();
	pHeader->setLabel(0, tr("Name"));
	pHeader->setLabel(1, tr("Description"));
	pHeader->setLabel(2, tr("Type"));
	pHeader->setLabel(3, tr("Value"));
	pHeader->setLabel(4, tr("Default"));
	// Set read-onlyness of each column
	QTable::setColumnReadOnly(0, true);
	QTable::setColumnReadOnly(1, true);
	QTable::setColumnReadOnly(2, true);
/*  QTable::setColumnReadOnly(2, true); -- of course not. */
	QTable::setColumnReadOnly(4, true);
}

// Default destructor.
qsamplerDeviceParameterTable::~qsamplerDeviceParameterTable (void)
{
}


// The client descriptor property accessors.
void qsamplerDeviceParameterTable::setClient ( lscp_client_t *pClient )
{
    m_pClient = pClient;
}

lscp_client_t *qsamplerDeviceParameterTable::client (void)
{
    return m_pClient;
}


// end of qsamplerDevice.cpp
