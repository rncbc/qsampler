// qsamplerOptions.h
//
/****************************************************************************
   Copyright (C) 2004-2010, rncbc aka Rui Nuno Capela. All rights reserved.
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

#ifndef __qsamplerOptions_h
#define __qsamplerOptions_h

#include <QSettings>
#include <QStringList>


class QWidget;
class QComboBox;

namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::Options - Prototype settings class.
//

class Options
{
public:

	// Constructor.
	Options();
	// Default destructor.
	~Options();

	// The settings object accessor.
	QSettings& settings();

	// explicit I/O methods.
	void loadOptions();
	void saveOptions();

	// Command line arguments parser.
	bool parse_args(const QStringList& args);
	// Command line usage helper.
	void print_usage(const QString& arg0);

	// Startup supplied session file.
	QString sSessionFile;

	// Server options...
	QString sServerHost;
	int     iServerPort;
	int     iServerTimeout;
	bool    bServerStart;
	QString sServerCmdLine;
	int     iStartDelay;

	// Logging options...
	bool    bMessagesLog;
	QString sMessagesLogPath;

	// Display options...
	QString sDisplayFont;
	bool    bDisplayEffect;
	bool    bAutoRefresh;
	int     iAutoRefreshTime;
	int     iMaxVolume;
	QString sMessagesFont;
	bool    bMessagesLimit;
	int     iMessagesLimitLines;
	bool    bConfirmRemove;
	bool    bKeepOnTop;
	bool    bStdoutCapture;
	bool    bCompletePath;
	bool    bInstrumentNames;
	int     iBaseFontSize;

	// View options...
	bool    bMenubar;
	bool    bToolbar;
	bool    bStatusbar;
	bool    bAutoArrange;

	// Default options...
	QString sSessionDir;
	QString sInstrumentDir;
	QString sEngineName;
	QString sAudioDriver;
	QString sMidiDriver;
	int     iMidiMap;
	int     iMidiBank;
	int     iMidiProg;
	int     iVolume;
	int     iLoadMode;

	// Recent file list.
	int     iMaxRecentFiles;
	QStringList recentFiles;

	// Widget geometry persistence helper prototypes.
	void saveWidgetGeometry(QWidget *pWidget, bool bVisible = false);
	void loadWidgetGeometry(QWidget *pWidget, bool bVisible = false);

	// Combo box history persistence helper prototypes.
	void loadComboBoxHistory(QComboBox *pComboBox, int iLimit = 8);
	void saveComboBoxHistory(QComboBox *pComboBox, int iLimit = 8);

	int  getMaxVoices();
	int  getEffectiveMaxVoices();
	void setMaxVoices(int iMaxVoices);

	int  getMaxStreams();
	int  getEffectiveMaxStreams();
	void setMaxStreams(int iMaxStreams);

	void sendFineTuningSettings();

private:

	// Settings member variables.
	QSettings m_settings;

	// Tuning
	int iMaxVoices;
	int iMaxStreams;
};

} // namespace QSampler


#endif  // __qsamplerOptions_h


// end of qsamplerOptions.h
