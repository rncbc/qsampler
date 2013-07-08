// qsamplerMessages.cpp
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

#include "qsamplerAbout.h"
#include "qsamplerMessages.h"

#include <QSocketNotifier>

#include <QFile>
#include <QTextEdit>
#include <QTextCursor>
#include <QTextStream>
#include <QTextBlock>
#include <QScrollBar>
#include <QDateTime>
#include <QIcon>

#if !defined(WIN32)
#include <unistd.h>
#endif


namespace QSampler {

// The default maximum number of message lines.
#define QSAMPLER_MESSAGES_MAXLINES  1000

// Notification pipe descriptors
#define QSAMPLER_MESSAGES_FDNIL    -1
#define QSAMPLER_MESSAGES_FDREAD    0
#define QSAMPLER_MESSAGES_FDWRITE   1

//-------------------------------------------------------------------------
// QSampler::Messages - Messages log dockable window.
//

// Constructor.
Messages::Messages ( QWidget *pParent )
	: QDockWidget(pParent)
{
	// Surely a name is crucial (e.g.for storing geometry settings)
	QDockWidget::setObjectName("qsamplerMessages");

	// Intialize stdout capture stuff.
	m_pStdoutNotifier = NULL;
	m_fdStdout[QSAMPLER_MESSAGES_FDREAD]  = QSAMPLER_MESSAGES_FDNIL;
	m_fdStdout[QSAMPLER_MESSAGES_FDWRITE] = QSAMPLER_MESSAGES_FDNIL;

	// Create local text view widget.
	m_pMessagesTextView = new QTextEdit(this);
//  QFont font(m_pMessagesTextView->font());
//  font.setFamily("Fixed");
//  m_pMessagesTextView->setFont(font);
	m_pMessagesTextView->setLineWrapMode(QTextEdit::NoWrap);
	m_pMessagesTextView->setReadOnly(true);
	m_pMessagesTextView->setUndoRedoEnabled(false);
//	m_pMessagesTextView->setTextFormat(Qt::LogText);

	// Initialize default message limit.
	m_iMessagesLines = 0;
	setMessagesLimit(QSAMPLER_MESSAGES_MAXLINES);

	m_pMessagesLog = NULL;

	// Prepare the dockable window stuff.
	QDockWidget::setWidget(m_pMessagesTextView);
	QDockWidget::setFeatures(QDockWidget::AllDockWidgetFeatures);
	QDockWidget::setAllowedAreas(
		Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	// Some specialties to this kind of dock window...
	QDockWidget::setMinimumHeight(120);

	// Finally set the default caption and tooltip.
	const QString& sCaption = tr("Messages");
	QDockWidget::setWindowTitle(sCaption);
//	QDockWidget::setWindowIcon(QIcon(":/icons/qsamplerMessages.png"));
	QDockWidget::setToolTip(sCaption);
}


// Destructor.
Messages::~Messages (void)
{
	// Turn off and close logging.
	setLogging(false);

	// No more notifications.
	if (m_pStdoutNotifier)
		delete m_pStdoutNotifier;

	// No need to delete child widgets, Qt does it all for us.
}


// Own stdout/stderr socket notifier slot.
void Messages::stdoutNotify ( int fd )
{
#if !defined(WIN32)
	char achBuffer[1024];
	int  cchBuffer = ::read(fd, achBuffer, sizeof(achBuffer) - 1);
	if (cchBuffer > 0) {
		achBuffer[cchBuffer] = (char) 0;
		appendStdoutBuffer(achBuffer);
	}
#endif
}


// Stdout buffer handler -- now splitted by complete new-lines...
void Messages::appendStdoutBuffer ( const QString& s )
{
	m_sStdoutBuffer.append(s);

	int iLength = m_sStdoutBuffer.lastIndexOf('\n');
	if (iLength > 0) {
		QString sTemp = m_sStdoutBuffer.left(iLength);
		m_sStdoutBuffer.remove(0, iLength + 1);
		QStringList list = sTemp.split('\n');
		QStringListIterator iter(list);
		while (iter.hasNext())
			appendMessagesText(iter.next());
	}
}


// Stdout flusher -- show up any unfinished line...
void Messages::flushStdoutBuffer (void)
{
	if (!m_sStdoutBuffer.isEmpty()) {
		appendMessagesText(m_sStdoutBuffer);
		m_sStdoutBuffer.truncate(0);
	}
}


// Stdout capture accessors.
bool Messages::isCaptureEnabled (void)
{
	return (bool) (m_pStdoutNotifier != NULL);
}

void Messages::setCaptureEnabled ( bool bCapture )
{
	// Flush current buffer.
	flushStdoutBuffer();

#if !defined(WIN32)
	// Destroy if already enabled.
	if (!bCapture && m_pStdoutNotifier) {
		delete m_pStdoutNotifier;
		m_pStdoutNotifier = NULL;
		// Close the notification pipes.
		if (m_fdStdout[QSAMPLER_MESSAGES_FDREAD] != QSAMPLER_MESSAGES_FDNIL) {
			::close(m_fdStdout[QSAMPLER_MESSAGES_FDREAD]);
			m_fdStdout[QSAMPLER_MESSAGES_FDREAD]  = QSAMPLER_MESSAGES_FDNIL;
		}
		if (m_fdStdout[QSAMPLER_MESSAGES_FDREAD] != QSAMPLER_MESSAGES_FDNIL) {
			::close(m_fdStdout[QSAMPLER_MESSAGES_FDREAD]);
			m_fdStdout[QSAMPLER_MESSAGES_FDREAD]  = QSAMPLER_MESSAGES_FDNIL;
		}
	}
	// Are we going to make up the capture?
	if (bCapture && m_pStdoutNotifier == NULL && ::pipe(m_fdStdout) == 0) {
		::dup2(m_fdStdout[QSAMPLER_MESSAGES_FDWRITE], STDOUT_FILENO);
		::dup2(m_fdStdout[QSAMPLER_MESSAGES_FDWRITE], STDERR_FILENO);
		m_pStdoutNotifier = new QSocketNotifier(
			m_fdStdout[QSAMPLER_MESSAGES_FDREAD], QSocketNotifier::Read, this);
		QObject::connect(m_pStdoutNotifier,
			SIGNAL(activated(int)),
			SLOT(stdoutNotify(int)));
	}
#endif
}


// Message font accessors.
QFont Messages::messagesFont (void)
{
	return m_pMessagesTextView->font();
}

void Messages::setMessagesFont ( const QFont& font )
{
	m_pMessagesTextView->setFont(font);
}


// Maximum number of message lines accessors.
int Messages::messagesLimit (void)
{
	return m_iMessagesLimit;
}

void Messages::setMessagesLimit ( int iMessagesLimit )
{
	m_iMessagesLimit = iMessagesLimit;
	m_iMessagesHigh  = iMessagesLimit + (iMessagesLimit / 3);
}

// Messages logging stuff.
bool Messages::isLogging (void) const
{
	return (m_pMessagesLog != NULL);
}

void Messages::setLogging ( bool bEnabled, const QString& sFilename )
{
	if (m_pMessagesLog) {
		appendMessages(tr("Logging stopped --- %1 ---")
			.arg(QDateTime::currentDateTime().toString()));
		m_pMessagesLog->close();
		delete m_pMessagesLog;
		m_pMessagesLog = NULL;
	}

	if (bEnabled) {
		m_pMessagesLog = new QFile(sFilename);
		if (m_pMessagesLog->open(QIODevice::Text | QIODevice::Append)) {
			appendMessages(tr("Logging started --- %1 ---")
				.arg(QDateTime::currentDateTime().toString()));
		} else {
			delete m_pMessagesLog;
			m_pMessagesLog = NULL;
		}
	}
}


// Messages log output method.
void Messages::appendMessagesLog ( const QString& s )
{
	if (m_pMessagesLog) {
		QTextStream(m_pMessagesLog) << s << endl;
		m_pMessagesLog->flush();
	}
}

// Messages widget output method.
void Messages::appendMessagesLine ( const QString& s )
{
	// Check for message line limit...
	if (m_iMessagesLines > m_iMessagesHigh) {
		m_pMessagesTextView->setUpdatesEnabled(false);
		QTextCursor textCursor(m_pMessagesTextView->document()->begin());
		while (m_iMessagesLines > m_iMessagesLimit) {
			// Move cursor extending selection
			// from start to next line-block...
			textCursor.movePosition(
				QTextCursor::NextBlock, QTextCursor::KeepAnchor);
			m_iMessagesLines--;
		}
		// Remove the excessive line-blocks...
		textCursor.removeSelectedText();
		m_pMessagesTextView->setUpdatesEnabled(true);
	}

	m_pMessagesTextView->append(s);
	m_iMessagesLines++;
}


// The main utility methods.
void Messages::appendMessages ( const QString& s )
{
	appendMessagesColor(s, "#999999");
}

void Messages::appendMessagesColor ( const QString& s, const QString &c )
{
	QString sText = QTime::currentTime().toString("hh:mm:ss.zzz") + ' ' + s;
	appendMessagesLine("<font color=\"" + c + "\">" + sText + "</font>");
	appendMessagesLog(sText);
}

void Messages::appendMessagesText ( const QString& s )
{
	appendMessagesLine(s);
	appendMessagesLog(s);
}


// History reset.
void Messages::clear (void)
{
	m_iMessagesLines = 0;
	m_pMessagesTextView->clear();
}

} // namespace QSampler


// end of qsamplerMessages.cpp
