// qsamplerOptions.h
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

#ifndef __qsamplerOptions_h
#define __qsamplerOptions_h

#include <qsettings.h>
#include <qcombobox.h>


//-------------------------------------------------------------------------
// qsamplerOptions - Prototype settings class.
//

class qsamplerOptions
{
public:

    // Constructor.
    qsamplerOptions();
    // Default destructor.
    ~qsamplerOptions();

    // Command line arguments parser.
    bool parse_args(int argc, char **argv);
    // Command line usage helper.
    void print_usage(const char *arg0);

    // Server options...
    QString sServerHost;
    int     iServerPort;
    bool    bServerStart;
    QString sServerCmdLine;
    
    // Display options...
    QString sMessagesFont;
    bool    bMessagesLimit;
    int     iMessagesLimitLines;
    bool    bQueryClose;
    bool    bStdoutCapture;

    // View options...
    bool    bMenubar;
    bool    bFileToolbar;
    bool    bEditToolbar;
    bool    bStatusbar;

    // Widget geometry persistence helper prototypes.
    void saveWidgetGeometry(QWidget *pWidget);
    void loadWidgetGeometry(QWidget *pWidget);

    // Combo box history persistence helper prototypes.
    void add2ComboBoxHistory(QComboBox *pComboBox, const QString& sNewText, int iLimit = 8, int iIndex = -1);
    void loadComboBoxHistory(QComboBox *pComboBox, int iLimit = 8);
    void saveComboBoxHistory(QComboBox *pComboBox, int iLimit = 8);

private:

    // Settings member variables.
    QSettings m_settings;
};


#endif  // __qsamplerOptions_h


// end of qsamplerOptions.h
