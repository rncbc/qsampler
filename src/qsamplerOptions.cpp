// qsamplerOptions.cpp
//
/****************************************************************************
   Copyright (C) 2004-2007, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007, Christian Schoenebeck

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qsamplerAbout.h"
#include "qsamplerOptions.h"

#include <qcombobox.h>

#include <lscp/client.h>

#ifdef CONFIG_LIBGIG
#include <gig.h>
#endif


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
    iServerTimeout = m_settings.readNumEntry("/ServerTimeout", 1000);
    bServerStart   = m_settings.readBoolEntry("/ServerStart", true);
    sServerCmdLine = m_settings.readEntry("/ServerCmdLine", "linuxsampler");
    iStartDelay    = m_settings.readNumEntry("/StartDelay", 3);
    m_settings.endGroup();

    // Load display options...
    m_settings.beginGroup("/Display");
    sDisplayFont     = m_settings.readEntry("/DisplayFont", QString::null);
    bDisplayEffect   = m_settings.readBoolEntry("/DisplayEffect", true);
    bAutoRefresh     = m_settings.readBoolEntry("/AutoRefresh", true);
    iAutoRefreshTime = m_settings.readNumEntry("/AutoRefreshTime", 1000);
    iMaxVolume       = m_settings.readNumEntry("/MaxVolume", 100);
    sMessagesFont    = m_settings.readEntry("/MessagesFont", QString::null);
    bMessagesLimit   = m_settings.readBoolEntry("/MessagesLimit", true);
    iMessagesLimitLines = m_settings.readNumEntry("/MessagesLimitLines", 1000);
    bConfirmRemove   = m_settings.readBoolEntry("/ConfirmRemove", true);
    bKeepOnTop       = m_settings.readBoolEntry("/KeepOnTop", true);
    bStdoutCapture   = m_settings.readBoolEntry("/StdoutCapture", true);
    bCompletePath    = m_settings.readBoolEntry("/CompletePath", true);
    iMaxRecentFiles  = m_settings.readNumEntry("/MaxRecentFiles", 5);
    bInstrumentNames = m_settings.readBoolEntry("/InstrumentNames", false);
    m_settings.endGroup();

    // And go into view options group.
    m_settings.beginGroup("/View");
    bMenubar     = m_settings.readBoolEntry("/Menubar", true);
    bToolbar     = m_settings.readBoolEntry("/Toolbar", true);
    bStatusbar   = m_settings.readBoolEntry("/Statusbar", true);
    bAutoArrange = m_settings.readBoolEntry("/AutoArrange", true);
    m_settings.endGroup();

    m_settings.endGroup(); // Options group.

    // Recent file list.
    m_settings.beginGroup("/RecentFiles");
    recentFiles.clear();
    for (int i = 0; i < iMaxRecentFiles; i++) {
        QString sFilename = m_settings.readEntry("/File" + QString::number(i + 1), QString::null);
        if (!sFilename.isEmpty())
            recentFiles.append(sFilename);
    }
    m_settings.endGroup();

    // Last but not least, get the default directories.
    m_settings.beginGroup("/Default");
    sSessionDir    = m_settings.readEntry("/SessionDir", QString::null);
    sInstrumentDir = m_settings.readEntry("/InstrumentDir", QString::null);
    sEngineName    = m_settings.readEntry("/EngineName", QString::null);
    sAudioDriver   = m_settings.readEntry("/AudioDriver", QString::null);
    sMidiDriver    = m_settings.readEntry("/MidiDriver", QString::null);
    iMidiMap       = m_settings.readNumEntry("/MidiMap", 0);
    iMidiBank      = m_settings.readNumEntry("/MidiBank", 0);
    iMidiProg      = m_settings.readNumEntry("/MidiProg", 0);
    iVolume        = m_settings.readNumEntry("/Volume", 100);
    iLoadMode      = m_settings.readNumEntry("/Loadmode", 0);
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
    m_settings.writeEntry("/ServerTimeout", iServerTimeout);
    m_settings.writeEntry("/ServerStart", bServerStart);
    m_settings.writeEntry("/ServerCmdLine", sServerCmdLine);
    m_settings.writeEntry("/StartDelay", iStartDelay);
    m_settings.endGroup();

    // Save display options.
    m_settings.beginGroup("/Display");
    m_settings.writeEntry("/DisplayFont", sDisplayFont);
    m_settings.writeEntry("/DisplayEffect", bDisplayEffect);
    m_settings.writeEntry("/AutoRefresh", bAutoRefresh);
    m_settings.writeEntry("/AutoRefreshTime", iAutoRefreshTime);
    m_settings.writeEntry("/MaxVolume", iMaxVolume);
    m_settings.writeEntry("/MessagesFont", sMessagesFont);
    m_settings.writeEntry("/MessagesLimit", bMessagesLimit);
    m_settings.writeEntry("/MessagesLimitLines", iMessagesLimitLines);
    m_settings.writeEntry("/ConfirmRemove", bConfirmRemove);
    m_settings.writeEntry("/KeepOnTop", bKeepOnTop);
    m_settings.writeEntry("/StdoutCapture", bStdoutCapture);
    m_settings.writeEntry("/CompletePath", bCompletePath);
    m_settings.writeEntry("/MaxRecentFiles", iMaxRecentFiles);
    m_settings.writeEntry("/InstrumentNames", bInstrumentNames);
    m_settings.endGroup();

    // View options group.
    m_settings.beginGroup("/View");
    m_settings.writeEntry("/Menubar", bMenubar);
    m_settings.writeEntry("/Toolbar", bToolbar);
    m_settings.writeEntry("/Statusbar", bStatusbar);
    m_settings.writeEntry("/AutoArrange", bAutoArrange);
    m_settings.endGroup();

    m_settings.endGroup(); // Options group.

    // Recent file list.
    m_settings.beginGroup("/RecentFiles");
    for (int i = 0; i < (int) recentFiles.count(); i++)
        m_settings.writeEntry("/File" + QString::number(i + 1), recentFiles[i]);
    m_settings.endGroup();

    // Default directories.
    m_settings.beginGroup("/Default");
    m_settings.writeEntry("/SessionDir", sSessionDir);
    m_settings.writeEntry("/InstrumentDir", sInstrumentDir);
    m_settings.writeEntry("/EngineName", sEngineName);
    m_settings.writeEntry("/AudioDriver", sAudioDriver);
    m_settings.writeEntry("/MidiDriver", sMidiDriver);
    m_settings.writeEntry("/MidiMap", iMidiMap);
    m_settings.writeEntry("/MidiBank", iMidiBank);
    m_settings.writeEntry("/MidiProg", iMidiProg);
    m_settings.writeEntry("/Volume", iVolume);
    m_settings.writeEntry("/Loadmode", iLoadMode);
    m_settings.endGroup();

    m_settings.endGroup();
}

//-------------------------------------------------------------------------
// Settings accessor.
//

QSettings& qsamplerOptions::settings (void)
{
    return m_settings;
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
        QObject::tr("session-file") + "]" + sEol, arg0);
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
            sSessionFile += " ";
            sSessionFile += argv[i];
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
#ifdef CONFIG_LIBGIG
			fprintf(stderr, "%s: %s\n", gig::libraryName().c_str(), gig::libraryVersion().c_str());
#endif            
            fprintf(stderr, "%s: %s\n", ::lscp_client_package(), ::lscp_client_version());
            fprintf(stderr, "qsampler: %s\n", QSAMPLER_VERSION);
            return false;
        }
        else {
            // If we don't have one by now,
            // this will be the startup sesion file...
            sSessionFile += sArg;
            iCmdArgs++;
        }
    }

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
        else
            pWidget->hide();
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
