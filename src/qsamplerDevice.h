// qsamplerDevice.h
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

#ifndef __qsamplerDevice_h
#define __qsamplerDevice_h

#include <qtable.h>

#include <lscp/client.h>
#include <lscp/device.h>

#include "qsamplerOptions.h"


//-------------------------------------------------------------------------
// qsamplerDeviceParameterTable - Device parameter view table.
//

class qsamplerDeviceParameterTable : public QTable
{
    Q_OBJECT

public:

    // Constructor.
    qsamplerDeviceParameterTable(QWidget *pParent = 0, const char *pszName = 0);
    // Default destructor.
    ~qsamplerDeviceParameterTable();

    // LSCP client descriptor accessor.
    void setClient(lscp_client_t *pClient);
    lscp_client_t * client();

private:

    // LSCP client reference.
    lscp_client_t *m_pClient;
};


#endif  // __qsamplerDevice_h


// end of qsamplerDevice.h
