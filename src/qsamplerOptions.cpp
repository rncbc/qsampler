// qsamplerOptions.cpp
//
/****************************************************************************
   Copyright (C) 2004-2009, rncbc aka Rui Nuno Capela. All rights reserved.
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
#include "qsamplerMainForm.h"

#include <QTextStream>
#include <QComboBox>

#include <lscp/client.h>

#ifdef CONFIG_LIBGIG
#include <gig.h>
#endif


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::Options - Prototype settings structure.
//

// Constructor.
Options::Options (void)
	: m_settings(QSAMPLER_DOMAIN, QSAMPLER_TITLE)
{
	// Begin into general options group.
	m_settings.beginGroup("/Options");

	// Load server options...
	m_settings.beginGroup("/Server");
	sServerHost    = m_settings.value("/ServerHost", "localhost").toString();
	iServerPort    = m_settings.value("/ServerPort", 8888).toInt();
	#if defined(__APPLE__)  //  Toshi Nagata 20080105
	//  TODO: Should this be a configure option?
	iServerTimeout = m_settings.value("/ServerTimeout", 10000).toInt();
	#else
	iServerTimeout = m_settings.value("/ServerTimeout", 1000).toInt();
	#endif
	bServerStart   = m_settings.value("/ServerStart", true).toBool();
	#if defined(__APPLE__)  //  Toshi Nagata 20080113
	sServerCmdLine = m_settings.value("/ServerCmdLine", "linuxsampler.starter").toString();
	#else
	sServerCmdLine = m_settings.value("/ServerCmdLine", "linuxsampler").toString();
	#endif
	iStartDelay    = m_settings.value("/StartDelay", 3).toInt();
	m_settings.endGroup();

	// Load logging options...
	m_settings.beginGroup("/Logging");
	bMessagesLog     = m_settings.value("/MessagesLog", false).toBool();
	sMessagesLogPath = m_settings.value("/MessagesLogPath", "qsampler.log").toString();
	m_settings.endGroup();

	// Load display options...
	m_settings.beginGroup("/Display");
	sDisplayFont     = m_settings.value("/DisplayFont").toString();
	bDisplayEffect   = m_settings.value("/DisplayEffect", true).toBool();
	bAutoRefresh     = m_settings.value("/AutoRefresh", true).toBool();
	iAutoRefreshTime = m_settings.value("/AutoRefreshTime", 1000).toInt();
	iMaxVolume       = m_settings.value("/MaxVolume", 100).toInt();
	sMessagesFont    = m_settings.value("/MessagesFont").toString();
	bMessagesLimit   = m_settings.value("/MessagesLimit", true).toBool();
	iMessagesLimitLines = m_settings.value("/MessagesLimitLines", 1000).toInt();
	bConfirmRemove   = m_settings.value("/ConfirmRemove", true).toBool();
	bKeepOnTop       = m_settings.value("/KeepOnTop", true).toBool();
	bStdoutCapture   = m_settings.value("/StdoutCapture", true).toBool();
	bCompletePath    = m_settings.value("/CompletePath", true).toBool();
	iMaxRecentFiles  = m_settings.value("/MaxRecentFiles", 5).toInt();
	iBaseFontSize    = m_settings.value("/BaseFontSize", 0).toInt();
// if libgig provides a fast way to retrieve instrument names even for large
// .gig files, then we enable this feature by default
#if HAVE_LIBGIG_SETAUTOLOAD
	bInstrumentNames = m_settings.value("/InstrumentNames", true).toBool();
#else
	bInstrumentNames = m_settings.value("/InstrumentNames", false).toBool();
#endif
	m_settings.endGroup();

	// And go into view options group.
	m_settings.beginGroup("/View");
	bMenubar     = m_settings.value("/Menubar", true).toBool();
	bToolbar     = m_settings.value("/Toolbar", true).toBool();
	bStatusbar   = m_settings.value("/Statusbar", true).toBool();
	bAutoArrange = m_settings.value("/AutoArrange", true).toBool();
	m_settings.endGroup();

	m_settings.endGroup(); // Options group.

	// Recent file list.
	recentFiles.clear();
	m_settings.beginGroup("/RecentFiles");
	for (int iFile = 0; iFile < iMaxRecentFiles; iFile++) {
		QString sFilename = m_settings.value(
			"/File" + QString::number(iFile + 1)).toString();
		if (!sFilename.isEmpty())
			recentFiles.append(sFilename);
	}
	m_settings.endGroup();

	// Sampler fine tuning settings.
	m_settings.beginGroup("/Tuning");
	iMaxVoices  = m_settings.value("/MaxVoices",  -1).toInt();
	iMaxStreams = m_settings.value("/MaxStreams",  -1).toInt();
	m_settings.endGroup();

	// Last but not least, get the default directories.
	m_settings.beginGroup("/Default");
	sSessionDir    = m_settings.value("/SessionDir").toString();
	sInstrumentDir = m_settings.value("/InstrumentDir").toString();
	sEngineName    = m_settings.value("/EngineName").toString();
	sAudioDriver   = m_settings.value("/AudioDriver").toString();
	sMidiDriver    = m_settings.value("/MidiDriver").toString();
	iMidiMap       = m_settings.value("/MidiMap",  0).toInt();
	iMidiBank      = m_settings.value("/MidiBank", 0).toInt();
	iMidiProg      = m_settings.value("/MidiProg", 0).toInt();
	iVolume        = m_settings.value("/Volume", 100).toInt();
	iLoadMode      = m_settings.value("/Loadmode", 0).toInt();
	m_settings.endGroup();
}


// Default Destructor.
Options::~Options (void)
{
	// Make program version available in the future.
	m_settings.beginGroup("/Program");
	m_settings.setValue("/Version", QSAMPLER_VERSION);
	m_settings.endGroup();

	// And go into general options group.
	m_settings.beginGroup("/Options");

	// Save server options.
	m_settings.beginGroup("/Server");
	m_settings.setValue("/ServerHost", sServerHost);
	m_settings.setValue("/ServerPort", iServerPort);
	m_settings.setValue("/ServerTimeout", iServerTimeout);
	m_settings.setValue("/ServerStart", bServerStart);
	m_settings.setValue("/ServerCmdLine", sServerCmdLine);
	m_settings.setValue("/StartDelay", iStartDelay);
	m_settings.endGroup();

	// Save logging options...
	m_settings.beginGroup("/Logging");
	m_settings.setValue("/MessagesLog", bMessagesLog);
	m_settings.setValue("/MessagesLogPath", sMessagesLogPath);
	m_settings.endGroup();

	// Save display options.
	m_settings.beginGroup("/Display");
	m_settings.setValue("/DisplayFont", sDisplayFont);
	m_settings.setValue("/DisplayEffect", bDisplayEffect);
	m_settings.setValue("/AutoRefresh", bAutoRefresh);
	m_settings.setValue("/AutoRefreshTime", iAutoRefreshTime);
	m_settings.setValue("/MaxVolume", iMaxVolume);
	m_settings.setValue("/MessagesFont", sMessagesFont);
	m_settings.setValue("/MessagesLimit", bMessagesLimit);
	m_settings.setValue("/MessagesLimitLines", iMessagesLimitLines);
	m_settings.setValue("/ConfirmRemove", bConfirmRemove);
	m_settings.setValue("/KeepOnTop", bKeepOnTop);
	m_settings.setValue("/StdoutCapture", bStdoutCapture);
	m_settings.setValue("/CompletePath", bCompletePath);
	m_settings.setValue("/MaxRecentFiles", iMaxRecentFiles);
	m_settings.setValue("/BaseFontSize", iBaseFontSize);
	m_settings.setValue("/InstrumentNames", bInstrumentNames);
	m_settings.endGroup();

	// View options group.
	m_settings.beginGroup("/View");
	m_settings.setValue("/Menubar", bMenubar);
	m_settings.setValue("/Toolbar", bToolbar);
	m_settings.setValue("/Statusbar", bStatusbar);
	m_settings.setValue("/AutoArrange", bAutoArrange);
	m_settings.endGroup();

	m_settings.endGroup(); // Options group.

	// Recent file list.
	int iFile = 0;
	m_settings.beginGroup("/RecentFiles");
	QStringListIterator iter(recentFiles);
	while (iter.hasNext())
		m_settings.setValue("/File" + QString::number(++iFile), iter.next());
	m_settings.endGroup();

	// Sampler fine tuning settings.
	m_settings.beginGroup("/Tuning");
	if (iMaxVoices > 0)
		m_settings.setValue("/MaxVoices", iMaxVoices);
	if (iMaxStreams >= 0)
		m_settings.setValue("/MaxStreams", iMaxStreams);
	m_settings.endGroup();

	// Default directories.
	m_settings.beginGroup("/Default");
	m_settings.setValue("/SessionDir", sSessionDir);
	m_settings.setValue("/InstrumentDir", sInstrumentDir);
	m_settings.setValue("/EngineName", sEngineName);
	m_settings.setValue("/AudioDriver", sAudioDriver);
	m_settings.setValue("/MidiDriver", sMidiDriver);
	m_settings.setValue("/MidiMap", iMidiMap);
	m_settings.setValue("/MidiBank", iMidiBank);
	m_settings.setValue("/MidiProg", iMidiProg);
	m_settings.setValue("/Volume", iVolume);
	m_settings.setValue("/Loadmode", iLoadMode);
	m_settings.endGroup();
}

//-------------------------------------------------------------------------
// Settings accessor.
//

QSettings& Options::settings (void)
{
	return m_settings;
}


//-------------------------------------------------------------------------
// Command-line argument stuff.
//

// Help about command line options.
void Options::print_usage ( const QString& arg0 )
{
	QTextStream out(stderr);
	out << QObject::tr("Usage: %1 [options] [session-file]\n\n"
		QSAMPLER_TITLE " - " QSAMPLER_SUBTITLE "\n\n"
		"Options:\n\n"
		"  -s, --start\n\tStart linuxsampler server locally\n\n"
		"  -h, --hostname\n\tSpecify linuxsampler server hostname\n\n"
		"  -p, --port\n\tSpecify linuxsampler server port number\n\n"
		"  -?, --help\n\tShow help about command line options\n\n"
		"  -v, --version\n\tShow version information\n\n")
		.arg(arg0);
}


// Parse command line arguments into m_settings.
bool Options::parse_args ( const QStringList& args )
{
	QTextStream out(stderr);
	const QString sEol = "\n\n";
	int iCmdArgs = 0;
	int argc = args.count();

	for (int i = 1; i < argc; i++) {

		if (iCmdArgs > 0) {
			sSessionFile += " ";
			sSessionFile += args.at(i);
			iCmdArgs++;
			continue;
		}

		QString sArg = args.at(i);
		QString sVal = QString::null;
		int iEqual = sArg.indexOf("=");
		if (iEqual >= 0) {
			sVal = sArg.right(sArg.length() - iEqual - 1);
			sArg = sArg.left(iEqual);
		}
		else if (i < argc - 1)
			sVal = args.at(i + 1);

		if (sArg == "-s" || sArg == "--start") {
			bServerStart = true;
		}
		else if (sArg == "-h" || sArg == "--hostname") {
			if (sVal.isNull()) {
				out << QObject::tr("Option -h requires an argument (hostname).") + sEol;
				return false;
			}
			sServerHost = sVal;
			if (iEqual < 0)
				i++;
		}
		else if (sArg == "-p" || sArg == "--port") {
			if (sVal.isNull()) {
				out << QObject::tr("Option -p requires an argument (port).") + sEol;
				return false;
			}
			iServerPort = sVal.toInt();
			if (iEqual < 0)
				i++;
		}
		else if (sArg == "-?" || sArg == "--help") {
			print_usage(args.at(0));
			return false;
		}
		else if (sArg == "-v" || sArg == "--version") {
			out << QObject::tr("Qt: %1\n").arg(qVersion());
#ifdef CONFIG_LIBGIG
			out << QString("%1: %2\n")
				.arg(gig::libraryName().c_str())
				.arg(gig::libraryVersion().c_str());
#endif
			out << QString("%1: %2\n")
				.arg(::lscp_client_package())
				.arg(::lscp_client_version());
			out << QObject::tr(QSAMPLER_TITLE ": %1\n").arg(QSAMPLER_VERSION);
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

void Options::loadWidgetGeometry ( QWidget *pWidget )
{
	// Try to restore old form window positioning.
	if (pWidget) {
		QPoint fpos;
		QSize  fsize;
		bool bVisible;
		m_settings.beginGroup("/Geometry/" + pWidget->objectName());
		fpos.setX(m_settings.value("/x", -1).toInt());
		fpos.setY(m_settings.value("/y", -1).toInt());
		fsize.setWidth(m_settings.value("/width", -1).toInt());
		fsize.setHeight(m_settings.value("/height", -1).toInt());
		bVisible = m_settings.value("/visible", false).toBool();
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


void Options::saveWidgetGeometry ( QWidget *pWidget )
{
	// Try to save form window position...
	// (due to X11 window managers ideossincrasies, we better
	// only save the form geometry while its up and visible)
	if (pWidget) {
		m_settings.beginGroup("/Geometry/" + pWidget->objectName());
		bool bVisible = pWidget->isVisible();
		const QPoint& fpos  = pWidget->pos();
		const QSize&  fsize = pWidget->size();
		m_settings.setValue("/x", fpos.x());
		m_settings.setValue("/y", fpos.y());
		m_settings.setValue("/width", fsize.width());
		m_settings.setValue("/height", fsize.height());
		m_settings.setValue("/visible", bVisible);
		m_settings.endGroup();
	}
}


//---------------------------------------------------------------------------
// Combo box history persistence helper implementation.

void Options::loadComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
	// Load combobox list from configuration settings file...
	m_settings.beginGroup("/History/" + pComboBox->objectName());

	if (m_settings.childKeys().count() > 0) {
		pComboBox->setUpdatesEnabled(false);
		pComboBox->setDuplicatesEnabled(false);
		pComboBox->clear();
		for (int i = 0; i < iLimit; i++) {
			const QString& sText = m_settings.value(
				"/Item" + QString::number(i + 1)).toString();
			if (sText.isEmpty())
				break;
			pComboBox->addItem(sText);
		}
		pComboBox->setUpdatesEnabled(true);
	}

	m_settings.endGroup();
}


void Options::saveComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
	// Add current text as latest item...
	const QString& sCurrentText = pComboBox->currentText();
	int iCount = pComboBox->count();
	for (int i = 0; i < iCount; i++) {
		const QString& sText = pComboBox->itemText(i);
		if (sText == sCurrentText) {
			pComboBox->removeItem(i);
			iCount--;
			break;
		}
	}
	while (iCount >= iLimit)
		pComboBox->removeItem(--iCount);
	pComboBox->insertItem(0, sCurrentText);
	iCount++;

	// Save combobox list to configuration settings file...
	m_settings.beginGroup("/History/" + pComboBox->objectName());
	for (int i = 0; i < iCount; i++) {
		const QString& sText = pComboBox->itemText(i);
		if (sText.isEmpty())
			break;
		m_settings.setValue("/Item" + QString::number(i + 1), sText);
	}
	m_settings.endGroup();
}

int Options::getMaxVoices() {
#ifndef CONFIG_MAX_VOICES
	return -1;
#else
	if (iMaxVoices > 0) return iMaxVoices;
	return getEffectiveMaxVoices();
#endif // CONFIG_MAX_VOICES
}

int Options::getEffectiveMaxVoices() {
#ifndef CONFIG_MAX_VOICES
	return -1;
#else
	MainForm *pMainForm = MainForm::getInstance();
	if (!pMainForm || !pMainForm->client())
		return -1;

	return ::lscp_get_voices(pMainForm->client());
#endif // CONFIG_MAX_VOICES
}

void Options::setMaxVoices(int iMaxVoices) {
#ifdef CONFIG_MAX_VOICES
	if (iMaxVoices < 1) return;

	MainForm *pMainForm = MainForm::getInstance();
	if (!pMainForm || !pMainForm->client())
		return;

	lscp_status_t result =
		::lscp_set_voices(pMainForm->client(), iMaxVoices);

	if (result != LSCP_OK) {
		pMainForm->appendMessagesClient("lscp_set_voices");
		return;
	}

	this->iMaxVoices = iMaxVoices;
#endif // CONFIG_MAX_VOICES
}

int Options::getMaxStreams() {
#ifndef CONFIG_MAX_VOICES
	return -1;
#else
	if (iMaxStreams > 0) return iMaxStreams;
	return getEffectiveMaxStreams();
#endif // CONFIG_MAX_VOICES
}

int Options::getEffectiveMaxStreams() {
#ifndef CONFIG_MAX_VOICES
	return -1;
#else
	MainForm *pMainForm = MainForm::getInstance();
	if (!pMainForm || !pMainForm->client())
		return -1;

	return ::lscp_get_streams(pMainForm->client());
#endif // CONFIG_MAX_VOICES
}

void Options::setMaxStreams(int iMaxStreams) {
#ifdef CONFIG_MAX_VOICES
	if (iMaxStreams < 0) return;

	MainForm *pMainForm = MainForm::getInstance();
	if (!pMainForm || !pMainForm->client())
		return;

	lscp_status_t result =
		::lscp_set_streams(pMainForm->client(), iMaxStreams);

	if (result != LSCP_OK) {
		pMainForm->appendMessagesClient("lscp_set_streams");
		return;
	}

	this->iMaxStreams = iMaxStreams;
#endif // CONFIG_MAX_VOICES
}

void Options::sendFineTuningSettings() {
	setMaxVoices(iMaxVoices);
	setMaxStreams(iMaxStreams);

	MainForm *pMainForm = MainForm::getInstance();
	if (!pMainForm || !pMainForm->client())
		return;

	pMainForm->appendMessages(QObject::tr("Sent fine tuning settings."));
}

} // namespace QSampler


// end of qsamplerOptions.cpp
