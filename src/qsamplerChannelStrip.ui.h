// qsamplerChannelStrip.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2004, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <qvalidator.h>
#include <qmessagebox.h>
#include <qfiledialog.h>

#include "qsamplerChannelForm.h"

#include "config.h"


// Kind of constructor.
void qsamplerChannelStrip::init (void)
{
    // Try to restore normal window positioning.
    adjustSize();
}


// Kind of destructor.
void qsamplerChannelStrip::destroy (void)
{
}


// Channel setup dialog.
void qsamplerChannelStrip::channelSetup (void)
{
    qsamplerChannelForm *pChannelForm = new qsamplerChannelForm(this);
    if (pChannelForm) {
        if (pChannelForm->exec())
            emit channelChanged(this);
        delete pChannelForm;
    }
}


// end of qsamplerChannelStrip.ui.h

