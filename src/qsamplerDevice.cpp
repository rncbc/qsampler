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
