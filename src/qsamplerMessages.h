// qsamplerMessages.h
//
/****************************************************************************
   Copyright (C) 2004-2013, rncbc aka Rui Nuno Capela. All rights reserved.
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

#ifndef __qsamplerMessages_h
#define __qsamplerMessages_h

#include <QDockWidget>

class QSocketNotifier;
class QTextEdit;
class QFile;

namespace QSampler {

//-------------------------------------------------------------------------
// QSampler::Messages - Messages log dockable window.
//

class Messages : public QDockWidget
{
	Q_OBJECT

public:

	// Constructor.
	Messages(QWidget *pParent);
	// Destructor.
	~Messages();

	// Stdout/stderr capture accessors.
	bool isCaptureEnabled();
	void setCaptureEnabled(bool bCapture);

	// Message font accessors.
	QFont messagesFont();
	void setMessagesFont(const QFont & font);

	// Maximum number of message lines accessors.
	int messagesLimit();
	void setMessagesLimit(int iMessagesLimit);

	// Logging settings.
	bool isLogging() const;
	void setLogging(bool bEnabled, const QString& sFilename = QString());

	// The main utility methods.
	void appendMessages(const QString& s);
	void appendMessagesColor(const QString& s, const QString &c);
	void appendMessagesText(const QString& s);

	// Stdout capture functions.
	void appendStdoutBuffer(const QString& s);
	void flushStdoutBuffer();

	// History reset.
	void clear();

#if QT_VERSION < 0x040300
signals:

	void visibilityChanged(bool bVisible);

#endif

protected:

	// Message executives.
	void appendMessagesLine(const QString& s);
	void appendMessagesLog(const QString& s);

#if QT_VERSION < 0x040300

	// Overridden method of QWidget
	void showEvent(QShowEvent *pEvent);

#endif

protected slots:

	// Stdout capture slot.
	void stdoutNotify(int fd);

private:

	// The maximum number of message lines.
	int m_iMessagesLines;
	int m_iMessagesLimit;
	int m_iMessagesHigh;

	// The textview main widget.
	QTextEdit *m_pMessagesTextView;

	// Stdout capture variables.
	QSocketNotifier *m_pStdoutNotifier;
	QString          m_sStdoutBuffer;
	int              m_fdStdout[2];

	// Logging stuff.
	QFile *m_pMessagesLog;	
};

} // namespace QSampler

#endif  // __qsamplerMessages_h


// end of qsamplerMessages.h
