// qsamplerChannels.cpp
//
/****************************************************************************
   Copyright (C) 2003-2004, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qsamplerChannels.h"

#include <qtooltip.h>

#include "config.h"

//-------------------------------------------------------------------------
// qsamplerChannels - Channels child window.
//

// Constructor.
qsamplerChannels::qsamplerChannels ( QWidget *pParent, const char *pszName )
    : QVBox(pParent, pszName)
{
    // Surely a name is crucial (e.g.for storing geometry settings)
    if (pszName == 0)
        QVBox::setName("qsamplerChannels");
        
    // Finally set the default caption and tooltip.
    QString sCaption = tr("Channels");
    QToolTip::add(this, sCaption);
    QVBox::setCaption(sCaption);
}


// Destructor.
qsamplerChannels::~qsamplerChannels (void)
{
    // No need to delete child widgets, Qt does it all for us.
}


// end of qsamplerChannels.cpp
