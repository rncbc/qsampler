// qsamplerChannels.h
//
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

#ifndef __qsamplerChannels_h
#define __qsamplerChannels_h

#include <qscrollview.h>
#include <qframe.h>
#include <qptrlist.h>

class QVBoxLayout;
class QVBox;

class qsamplerChannelForm;


//-------------------------------------------------------------------------
// qsamplerChannelStrip - Channel strip window.
//

class qsamplerChannelStrip : public QFrame
{
    Q_OBJECT

public:

    // Constructor.
    qsamplerChannelStrip(QWidget *pParent, const char *pszName = 0);
    // Destructor.
    ~qsamplerChannelStrip();

    // The form accessor.
    qsamplerChannelForm *form();

private:

    // The layout enforcer;
    QVBoxLayout *m_pVBoxLayout;

    // The child widgets;
    qsamplerChannelForm *m_pChannelForm;
};


//-------------------------------------------------------------------------
// qsamplerChannels - Channels child window.
//

class qsamplerChannels : public QScrollView
{
    Q_OBJECT

public:

    // Constructor.
    qsamplerChannels(QWidget *pParent, const char *pszName = 0);
    // Destructor.
    ~qsamplerChannels();

    // Channel list management.
    qsamplerChannelStrip *addChannel();
    void removeChannel(qsamplerChannelStrip *pChannel);

    // Retrive channel form by index.
    qsamplerChannelStrip *channelAt(int iChannel);

private:

    // The vertical layout enforcer.
    QVBox *m_pVBox;

    // The official channel list.
    QPtrList<qsamplerChannelStrip> m_channels;
};


#endif  // __qsamplerChannels_h


// end of qsamplerChannels.h
