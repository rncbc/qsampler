// qsamplerOptions.cpp
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

#include "qsamplerOptions.h"
#include "qsamplerAbout.h"

#include "lscp/client.h"

#include "config.h"


//-------------------------------------------------------------------------
// qsamplerOptions - Prototype settings structure.
//

// Constructor.
qsamplerOptions::qsamplerOptions (void)
{
    // Begin master key group.
    m_settings.beginGroup("/qsampler");

    // And go into general options group.
    m_settings.beginGroup("/Options");

    // Load server options...
    m_settings.beginGroup("/Server");
    sServerHost    = m_settings.readEntry("/ServerHost", "localhost");
    iServerPort    = m_settings.readNumEntry("/ServerPort", 8888);
    bServerStart   = m_settings.readBoolEntry("/ServerStart", true);
    sServerCmdLine = m_settings.readEntry("/ServerCmdLine", "linuxsampler");
    m_settings.endGroup();

    // Load display options...
    m_settings.beginGroup("/Display");
    sMessagesFont  = m_settings.readEntry("/MessagesFont", QString::null);
    bMessagesLimit = m_settings.readBoolEntry("/MessagesLimit", true);
    iMessagesLimitLines = m_settings.readNumEntry("/MessagesLimitLines", 1000);
    bQueryClose    = m_settings.readBoolEntry("/QueryClose",    true);
    bStdoutCapture = m_settings.readBoolEntry("/StdoutCapture", true);
    m_settings.endGroup();

    // And go into view options group.
    m_settings.beginGroup("/View");
    bMenubar       = m_settings.readBoolEntry("/Menubar",       true);
    bFileToolbar   = m_settings.readBoolEntry("/FileToolbar",   true);
    bEditToolbar   = m_settings.readBoolEntry("/EditToolbar",   true);
    bStatusbar     = m_settings.readBoolEntry("/Statusbar",     true);
    m_settings.endGroup();

    m_settings.endGroup();
}


// Default Destructor.
qsamplerOptions::~qsamplerOptions (void)
{
    // Make program version available in the future.
    m_settings.beginGroup("/Program");
    m_settings.writeEntry("/Version", QSAMPLER_VERSION);
    m_settings.endGroup();

    // And go into general options group.
    m_settings.beginGroup("/Options");

    // Save server options.
    m_settings.beginGroup("/Server");
    m_settings.writeEntry("/ServerHost", sServerHost);
    m_settings.writeEntry("/ServerPort", iServerPort);
    m_settings.writeEntry("/ServerStart", bServerStart);
    m_settings.writeEntry("/ServerCmdLine", sServerCmdLine);
    m_settings.endGroup();

    // Save display options.
    m_settings.beginGroup("/Display");
    m_settings.writeEntry("/MessagesFont",  sMessagesFont);
    m_settings.writeEntry("/MessagesLimit", bMessagesLimit);
    m_settings.writeEntry("/MessagesLimitLines", iMessagesLimitLines);
    m_settings.writeEntry("/QueryClose",    bQueryClose);
    m_settings.writeEntry("/StdoutCapture", bStdoutCapture);
    m_settings.endGroup();

    // View options group.
    m_settings.beginGroup("/View");
    m_settings.writeEntry("/Menubar",       bMenubar);
    m_settings.writeEntry("/FileToolbar",   bFileToolbar);
    m_settings.writeEntry("/EditToolbar",   bEditToolbar);
    m_settings.writeEntry("/Statusbar",     bStatusbar);
    m_settings.endGroup();

    m_settings.endGroup();

    m_settings.endGroup();
}


//-------------------------------------------------------------------------
// Command-line argument stuff.
//

// Help about command line options.
void qsamplerOptions::print_usage ( const char *arg0 )
{
    const QString sEot = "\n\t";
    const QString sEol = "\n\n";

    fprintf(stderr, QObject::tr("Usage") + ": %s [" + QObject::tr("options") + "] [" +
        QObject::tr("command-and-args") + "]" + sEol, arg0);
    fprintf(stderr, QSAMPLER_TITLE " - " + QObject::tr(QSAMPLER_SUBTITLE) + sEol);
    fprintf(stderr, QObject::tr("Options") + ":" + sEol);
    fprintf(stderr, "  -s, --start" + sEot +
        QObject::tr("Start linuxsampler server locally") + sEol);
    fprintf(stderr, "  -h, --hostname" + sEot +
        QObject::tr("Specify linuxsampler server hostname") + sEol);
    fprintf(stderr, "  -p, --port" + sEot +
        QObject::tr("Specify linuxsampler server port number") + sEol);
    fprintf(stderr, "  -?, --help" + sEot +
        QObject::tr("Show help about command line options") + sEol);
    fprintf(stderr, "  -v, --version" + sEot +
        QObject::tr("Show version information") + sEol);
}


// Parse command line arguments into m_settings.
bool qsamplerOptions::parse_args ( int argc, char **argv )
{
    const QString sEol = "\n\n";
    int iCmdArgs = 0;

    for (int i = 1; i < argc; i++) {

        if (iCmdArgs > 0) {
            sServerCmdLine += " ";
            sServerCmdLine += argv[i];
            iCmdArgs++;
            continue;
        }

        QString sArg = argv[i];
        QString sVal = QString::null;
        int iEqual = sArg.find("=");
        if (iEqual >= 0) {
            sVal = sArg.right(sArg.length() - iEqual - 1);
            sArg = sArg.left(iEqual);
        }
        else if (i < argc)
            sVal = argv[i + 1];

        if (sArg == "-s" || sArg == "--start") {
            bServerStart = true;
        }
        else if (sArg == "-h" || sArg == "--hostname") {
        	if (sVal.isNull()) {
                fprintf(stderr, QObject::tr("Option -h requires an argument (hostname).") + sEol);
                return false;
            }
            sServerHost = sVal;
            if (iEqual < 0)
                i++;
        }
        else if (sArg == "-p" || sArg == "--port") {
        	if (sVal.isNull()) {
                fprintf(stderr, QObject::tr("Option -p requires an argument (port).") + sEol);
                return false;
            }
            iServerPort = sVal.toInt();
            if (iEqual < 0)
                i++;
        }
        else if (sArg == "-?" || sArg == "--help") {
            print_usage(argv[0]);
            return false;
        }
        else if (sArg == "-v" || sArg == "--version") {
            fprintf(stderr, "Qt: %s\n", qVersion());
            fprintf(stderr, "liblscp: %s\n", ::lscp_client_version());
            fprintf(stderr, "qsampler: %s\n", QSAMPLER_VERSION);
            return false;
        }
        else {
            // Here starts the optional command line...
            sServerCmdLine += sArg;
            iCmdArgs++;
        }
    }

    // HACK: If there's a command line, it must be spawned on background...
    if (iCmdArgs > 0)
        sServerCmdLine += " &";

    // Alright with argument parsing.
    return true;
}


//---------------------------------------------------------------------------
// Widget geometry persistence helper methods.

void qsamplerOptions::loadWidgetGeometry ( QWidget *pWidget )
{
    // Try to restore old form window positioning.
    if (pWidget) {
        QPoint fpos;
        QSize  fsize;
        bool bVisible;
        m_settings.beginGroup("/Geometry/" + QString(pWidget->name()));
        fpos.setX(m_settings.readNumEntry("/x", -1));
        fpos.setY(m_settings.readNumEntry("/y", -1));
        fsize.setWidth(m_settings.readNumEntry("/width", -1));
        fsize.setHeight(m_settings.readNumEntry("/height", -1));
        bVisible = m_settings.readBoolEntry("/visible", false);
        m_settings.endGroup();
        if (fpos.x() > 0 && fpos.y() > 0)
            pWidget->move(fpos);
        if (fsize.width() > 0 && fsize.height() > 0)
            pWidget->resize(fsize);
        else
            pWidget->adjustSize();
        if (bVisible)
            pWidget->show();
    }
}


void qsamplerOptions::saveWidgetGeometry ( QWidget *pWidget )
{
    // Try to save form window position...
    // (due to X11 window managers ideossincrasies, we better
    // only save the form geometry while its up and visible)
    if (pWidget) {
        m_settings.beginGroup("/Geometry/" + QString(pWidget->name()));
        bool bVisible = pWidget->isVisible();
        if (bVisible) {
            QPoint fpos  = pWidget->pos();
            QSize  fsize = pWidget->size();
            m_settings.writeEntry("/x", fpos.x());
            m_settings.writeEntry("/y", fpos.y());
            m_settings.writeEntry("/width", fsize.width());
            m_settings.writeEntry("/height", fsize.height());
        }
        m_settings.writeEntry("/visible", bVisible);
        m_settings.endGroup();
    }
}


//---------------------------------------------------------------------------
// Combo box history persistence helper implementation.

void qsamplerOptions::add2ComboBoxHistory ( QComboBox *pComboBox, const QString& sNewText, int iLimit, int iIndex )
{
    int iCount = pComboBox->count();
    for (int i = 0; i < iCount; i++) {
        QString sText = pComboBox->text(i);
        if (sText == sNewText) {
            pComboBox->removeItem(i);
            iCount--;
            break;
        }
    }
    while (iCount >= iLimit)
        pComboBox->removeItem(--iCount);
    pComboBox->insertItem(sNewText, iIndex);
}


void qsamplerOptions::loadComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
    pComboBox->setUpdatesEnabled(false);
    pComboBox->setDuplicatesEnabled(false);

    m_settings.beginGroup("/History/" + QString(pComboBox->name()));
    for (int i = 0; i < iLimit; i++) {
        QString sText = m_settings.readEntry("/Item" + QString::number(i + 1), QString::null);
        if (sText.isEmpty())
            break;
        add2ComboBoxHistory(pComboBox, sText, iLimit);
    }
    m_settings.endGroup();

    pComboBox->setUpdatesEnabled(true);
}


void qsamplerOptions::saveComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
    add2ComboBoxHistory(pComboBox, pComboBox->currentText(), iLimit, 0);

    m_settings.beginGroup("/History/" + QString(pComboBox->name()));
    for (int i = 0; i < iLimit && i < pComboBox->count(); i++) {
        QString sText = pComboBox->text(i);
        if (sText.isEmpty())
            break;
        m_settings.writeEntry("/Item" + QString::number(i + 1), sText);
    }
    m_settings.endGroup();
}


// end of qsamplerOptions.cpp
