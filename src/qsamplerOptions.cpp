// qsamplerOptions.cpp
//
/****************************************************************************
   Copyright (C) 2004-2025, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007,2008,2015 Christian Schoenebeck

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

#include <QFileInfo>
#include <QTextStream>

#include <QComboBox>

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QCommandLineParser>
#include <QCommandLineOption>
#if defined(Q_OS_WINDOWS)
#include <QMessageBox>
#endif
#endif

#include <QApplication>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#endif

#include <lscp/client.h>
#ifdef CONFIG_LIBGIG
#if defined(Q_CC_GNU) || defined(Q_CC_MINGW)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include <gig.h>
#if defined(Q_CC_GNU) || defined(Q_CC_MINGW)
#pragma GCC diagnostic pop
#endif
#endif


namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::Options - Prototype settings structure.
//

// Constructor.
Options::Options (void)
	: m_settings(QSAMPLER_DOMAIN, QSAMPLER_TITLE)
{
	loadOptions();
}


// Default Destructor.
Options::~Options (void)
{
	saveOptions();
}


// Explicit load method.
void Options::loadOptions (void)
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
#if defined(__APPLE__)
	sServerCmdLine = m_settings.value("/ServerCmdLine", "/usr/local/bin/linuxsampler").toString();
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
	bConfirmReset    = m_settings.value("/ConfirmReset", true).toBool();
	bConfirmRestart  = m_settings.value("/ConfirmRestart", true).toBool();
	bConfirmError    = m_settings.value("/ConfirmError", true).toBool();
	bKeepOnTop       = m_settings.value("/KeepOnTop", true).toBool();
	bStdoutCapture   = m_settings.value("/StdoutCapture", true).toBool();
	bCompletePath    = m_settings.value("/CompletePath", true).toBool();
	iMaxRecentFiles  = m_settings.value("/MaxRecentFiles", 5).toInt();
	iBaseFontSize    = m_settings.value("/BaseFontSize", 0).toInt();
// if libgig provides a fast way to retrieve instrument names even for large
// .gig files, then we enable this feature by default
#ifdef CONFIG_LIBGIG_SETAUTOLOAD
	bInstrumentNames = m_settings.value("/InstrumentNames", true).toBool();
#else
	bInstrumentNames = m_settings.value("/InstrumentNames", false).toBool();
#endif
	m_settings.endGroup();

	// Load custom options...
	m_settings.beginGroup("/Custom");
	sCustomColorTheme = m_settings.value("/ColorTheme").toString();
	sCustomStyleTheme = m_settings.value("/StyleTheme").toString();
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


// Explicit save method.
void Options::saveOptions (void)
{
	// Make program version available in the future.
	m_settings.beginGroup("/Program");
	m_settings.setValue("/Version", PROJECT_VERSION);
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
	m_settings.setValue("/ConfirmReset", bConfirmReset);
	m_settings.setValue("/ConfirmRestart", bConfirmRestart);
	m_settings.setValue("/ConfirmError", bConfirmError);
	m_settings.setValue("/KeepOnTop", bKeepOnTop);
	m_settings.setValue("/StdoutCapture", bStdoutCapture);
	m_settings.setValue("/CompletePath", bCompletePath);
	m_settings.setValue("/MaxRecentFiles", iMaxRecentFiles);
	m_settings.setValue("/BaseFontSize", iBaseFontSize);
	m_settings.setValue("/InstrumentNames", bInstrumentNames);
	m_settings.endGroup();

	// Save custom options...
	m_settings.beginGroup("/Custom");
	m_settings.setValue("/ColorTheme", sCustomColorTheme);
	m_settings.setValue("/StyleTheme", sCustomStyleTheme);
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

	// Save/commit to disk.
	m_settings.sync();
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)

void Options::show_error( const QString& msg )
{
#if defined(Q_OS_WINDOWS)
	QMessageBox::information(nullptr, QApplication::applicationName(), msg);
#else
	const QByteArray tmp = msg.toUtf8() + '\n';
	::fputs(tmp.constData(), stderr);
#endif
}

#else

// Help about command line options.
void Options::print_usage ( const QString& arg0 )
{
	QTextStream out(stderr);
	const QString sEot = "\n\t";
	const QString sEol = "\n\n";

	out << QObject::tr("Usage: %1 [options] [session-file]").arg(arg0) + sEol;
	out << QSAMPLER_TITLE " - " + QObject::tr(QSAMPLER_SUBTITLE) + sEol;
	out << QObject::tr("Options:") + sEol;
	out << "  -s, --start" + sEot +
		QObject::tr("Start linuxsampler server locally.") + sEol;
	out << "  -n, --hostname" + sEot +
		QObject::tr("Specify linuxsampler server hostname (default = localhost)") + sEol;
	out << "  -p, --port" + sEot +
		QObject::tr("Specify linuxsampler server port number (default = 8888)") + sEol;
	out << "  -h, --help" + sEot +
		QObject::tr("Show help about command line options.") + sEol;
	out << "  -v, --version" + sEot +
		QObject::tr("Show version information") + sEol;
}

#endif


// Parse command line arguments into m_settings.
bool Options::parse_args ( const QStringList& args )
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)

	QCommandLineParser parser;
	parser.setApplicationDescription(
		QSAMPLER_TITLE " - " + QObject::tr(QSAMPLER_SUBTITLE));

	parser.addOption({{"s", "start"},
		QObject::tr("Start linuxsampler server locally.")});
	parser.addOption({{"n", "hostname"},
		QObject::tr("Specify linuxsampler server hostname (default = localhost)"), "name"});
	parser.addOption({{"p", "port"},
		QObject::tr("Specify linuxsampler server port number (default = 8888)"), "num"});
	const QCommandLineOption& helpOption = parser.addHelpOption();
	const QCommandLineOption& versionOption = parser.addVersionOption();
	parser.addPositionalArgument("session-file",
		QObject::tr("Session file (.lscp)"),
		QObject::tr("[session-file]"));

	if (!parser.parse(args)) {
		show_error(parser.errorText());
		return false;
	}

	if (parser.isSet(helpOption)) {
		show_error(parser.helpText());
		return false;
	}

	if (parser.isSet(versionOption)) {
		QString sVersion = QString("%1 v%2")
			.arg(QSAMPLER_TITLE)
			.arg(QCoreApplication::applicationVersion());
		sVersion += '\n';
		sVersion += QString("Qt: %1").arg(qVersion());
	#if defined(QT_STATIC)
		sVersion += "-static";
	#endif
		sVersion += '\n';
	#ifdef CONFIG_LIBGIG
		sVersion += QString("%1: %2")
			.arg(gig::libraryName().c_str())
			.arg(gig::libraryVersion().c_str());
		sVersion += '\n';
	#endif
		sVersion += QString("%1: %2")
			.arg(::lscp_client_package())
			.arg(::lscp_client_version());
		show_error(sVersion);
		return false;
	}

	if (parser.isSet("start")) {
		bServerStart = true;
	}

	if (parser.isSet("hostname")) {
		const QString& sVal = parser.value("hostname");
		if (sVal.isEmpty()) {
			show_error(QObject::tr("Option -n requires an argument (hostname)."));
			return false;
		}
		sServerHost = sVal;
	}

	if (parser.isSet("port")) {
		bool bOK = false;
		const int iVal = parser.value("port").toInt(&bOK);
		if (!bOK) {
			show_error(QObject::tr("Option -p requires an argument (port)."));
			return false;
		}
		iServerPort = iVal;
	}

	foreach (const QString& sArg, parser.positionalArguments()) {
		sessionFiles.append(QFileInfo(sArg).absoluteFilePath());
	}

#else

	int iCmdArgs = 0;

	QTextStream out(stderr);
	const QString sEol = "\n\n";
	const int argc = args.count();

	for (int i = 1; i < argc; ++i) {

		QString sArg = args.at(i);

		if (iCmdArgs > 0) {
			sessionFiles.append(QFileInfo(sArg).absoluteFilePath());
			++iCmdArgs;
			continue;
		}

		QString sVal;
		const int iEqual = sArg.indexOf("=");
		if (iEqual >= 0) {
			sVal = sArg.right(sArg.length() - iEqual - 1);
			sArg = sArg.left(iEqual);
		}
		else if (i < argc - 1) {
			sVal = args.at(i + 1);
			if (sVal[0] == '-')
				sVal.clear();
		}

		if (sArg == "-s" || sArg == "--start") {
			bServerStart = true;
		}
		else if (sArg == "-n" || sArg == "--hostname") {
			if (sVal.isNull()) {
				out << QObject::tr("Option -n requires an argument (hostname).") + sEol;
				return false;
			}
			sServerHost = sVal;
			if (iEqual < 0)
				++i;
		}
		else if (sArg == "-p" || sArg == "--port") {
			if (sVal.isNull()) {
				out << QObject::tr("Option -p requires an argument (port).") + sEol;
				return false;
			}
			iServerPort = sVal.toInt();
			if (iEqual < 0)
				++i;
		}
		else if (sArg == "-h" || sArg == "--help") {
			print_usage(args.at(0));
			return false;
		}
		else if (sArg == "-v" || sArg == "--version") {
			out << QString("%1 v%2\n")
				.arg(QSAMPLER_TITLE)
				.arg(PROJECT_VERSION);
			out << QString("Qt: %1").arg(qVersion());
		#if defined(QT_STATIC)
			out << "-static";
		#endif
			out << '\n';
		#ifdef CONFIG_LIBGIG
			out << QString("%1: %2\n")
				.arg(gig::libraryName().c_str())
				.arg(gig::libraryVersion().c_str());
		#endif
			out << QString("%1: %2\n")
				.arg(::lscp_client_package())
				.arg(::lscp_client_version());
			return false;
		} else {
			// If we don't have one by now,
			// this will be the startup sesion file...
			sessionFiles.append(QFileInfo(sArg).absoluteFilePath());
			++iCmdArgs;
		}
	}

#endif

	// Alright with argument parsing.
	return true;
}


//---------------------------------------------------------------------------
// Widget geometry persistence helper methods.

void Options::loadWidgetGeometry ( QWidget *pWidget, bool bVisible )
{
	// Try to restore old form window positioning.
	if (pWidget) {
	//	if (bVisible) pWidget->show(); -- force initial exposure?
		m_settings.beginGroup("/Geometry/" + pWidget->objectName());
	#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		const QByteArray& geometry
			= m_settings.value("/geometry").toByteArray();
		if (geometry.isEmpty()) {
			QWidget *pParent = pWidget->parentWidget();
			if (pParent)
				pParent = pParent->window();
		#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			if (pParent == nullptr)
				pParent = QApplication::desktop();
		#endif
			if (pParent) {
				QRect wrect(pWidget->geometry());
				wrect.moveCenter(pParent->geometry().center());
				pWidget->move(wrect.topLeft());
			}
		} else {
			pWidget->restoreGeometry(geometry);
		}
	#else//--LOAD_OLD_GEOMETRY
		QPoint wpos;
		QSize  wsize;
		wpos.setX(m_settings.value("/x", -1).toInt());
		wpos.setY(m_settings.value("/y", -1).toInt());
		wsize.setWidth(m_settings.value("/width", -1).toInt());
		wsize.setHeight(m_settings.value("/height", -1).toInt());
		if (wpos.x() > 0 && wpos.y() > 0)
			pWidget->move(wpos);
		if (wsize.width() > 0 && wsize.height() > 0)
			pWidget->resize(wsize);
	#endif
	//	else
	//	pWidget->adjustSize();
		if (!bVisible)
			bVisible = m_settings.value("/visible", false).toBool();
		if (bVisible)
			pWidget->show();
		else
			pWidget->hide();
		m_settings.endGroup();
	}
}


void Options::saveWidgetGeometry ( QWidget *pWidget, bool bVisible )
{
	// Try to save form window position...
	// (due to X11 window managers ideossincrasies, we better
	// only save the form geometry while its up and visible)
	if (pWidget) {
		m_settings.beginGroup("/Geometry/" + pWidget->objectName());
	#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		m_settings.setValue("/geometry", pWidget->saveGeometry());
	#else//--SAVE_OLD_GEOMETRY
		const QPoint& wpos  = pWidget->pos();
		const QSize&  wsize = pWidget->size();
		m_settings.setValue("/x", wpos.x());
		m_settings.setValue("/y", wpos.y());
		m_settings.setValue("/width", wsize.width());
		m_settings.setValue("/height", wsize.height());
	#endif
		if (!bVisible) bVisible = pWidget->isVisible();
		m_settings.setValue("/visible", bVisible);
		m_settings.endGroup();
	}
}


//---------------------------------------------------------------------------
// Combo box history persistence helper implementation.

void Options::loadComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
	const bool bBlockSignals = pComboBox->blockSignals(true);

	// Load combobox list from configuration settings file...
	m_settings.beginGroup("/History/" + pComboBox->objectName());

	if (m_settings.childKeys().count() > 0) {
		pComboBox->setUpdatesEnabled(false);
		pComboBox->setDuplicatesEnabled(false);
		pComboBox->clear();
		for (int i = 0; i < iLimit; ++i) {
			const QString& sText = m_settings.value(
				"/Item" + QString::number(i + 1)).toString();
			if (sText.isEmpty())
				break;
			pComboBox->addItem(sText);
		}
		pComboBox->setUpdatesEnabled(true);
	}

	m_settings.endGroup();

	pComboBox->blockSignals(bBlockSignals);
}


void Options::saveComboBoxHistory ( QComboBox *pComboBox, int iLimit )
{
	const bool bBlockSignals = pComboBox->blockSignals(true);

	int iCount = pComboBox->count();
 
	// Add current text as latest item (if not blank)...
	const QString& sCurrentText = pComboBox->currentText();
	if (!sCurrentText.isEmpty()) {
		for (int i = 0; i < iCount; ++i) {
			const QString& sText = pComboBox->itemText(i);
			if (sText == sCurrentText) {
				pComboBox->removeItem(i);
				--iCount;
				break;
			}
		}
		pComboBox->insertItem(0, sCurrentText);
		pComboBox->setCurrentIndex(0);
		++iCount;
	}

	while (iCount >= iLimit)
		pComboBox->removeItem(--iCount);

	// Save combobox list to configuration settings file...
	m_settings.beginGroup("/History/" + pComboBox->objectName());
	for (int i = 0; i < iCount; ++i) {
		const QString& sText = pComboBox->itemText(i);
		if (sText.isEmpty())
			break;
		m_settings.setValue("/Item" + QString::number(i + 1), sText);
	}
	m_settings.endGroup();

	pComboBox->blockSignals(bBlockSignals);
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

