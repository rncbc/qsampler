// qsamplerMainForm.ui.h
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

#include <qmessagebox.h>
#include <qtextstream.h>
#include <qstatusbar.h>
#include <qlabel.h>
#include <qtimer.h>

#include "qsamplerAbout.h"

#include "lscp/client.h"

#include "config.h"

#if !defined(WIN32)
#include <unistd.h>
#endif

// Timer constant stuff.
#define QSAMPLER_TIMER_MSECS    500

#if defined(WIN32)
static WSADATA _wsaData;
#endif

//-------------------------------------------------------------------------
// qsamplerMainForm -- Main window form implementation.

// Kind of constructor.
void qsamplerMainForm::init (void)
{
    // Initialize some pointer references.
    m_pOptions = NULL;

    // All child forms are to be created later, not earlier than setup.
    m_pMessages = NULL;
    
    // We'll start clean.
    m_iDirtyCount = 0;

    m_pServer = NULL;
    m_pClient = NULL;

    m_iStartDelay = 0;
    m_iTimerDelay = 0;

    // Make it an MDI workspace.
    m_pWorkspace = new QWorkspace(this);
    m_pWorkspace->setScrollBarsEnabled(true);
    // Set the activation connection.
    QObject::connect(m_pWorkspace, SIGNAL(windowActivated(QWidget *)), this, SLOT(stabilizeForm()));
    // Make it shine :-)
    setCentralWidget(m_pWorkspace);
    
    // Create some statusbar labels...
    m_pStatusLine = new QLabel(this);
    m_pStatusFlag = new QLabel(tr("Modified"), this);
    m_pStatusFlag->setAlignment(Qt::AlignHCenter);
    m_pStatusFlag->setMinimumSize(m_pStatusFlag->sizeHint());
    // And make them there...
    statusBar()->addWidget(m_pStatusLine, 1);
    statusBar()->addWidget(m_pStatusFlag);

#if defined(WIN32)
    WSAStartup(MAKEWORD(1, 1), &_wsaData);
#endif
}


// Kind of destructor.
void qsamplerMainForm::destroy (void)
{
    // Stop client and/or server, if not already...
    stopServer();

    // Finally drop any widgets around...
    if (m_pStatusLine)
        delete m_pStatusLine;
    if (m_pStatusFlag)
        delete m_pStatusFlag;
    if (m_pMessages)
        delete m_pMessages;
    if (m_pWorkspace)
        delete m_pWorkspace;

#if defined(WIN32)
    WSACleanup();
#endif
}


// Make and set a proper setup options step.
void qsamplerMainForm::setup ( qsamplerOptions *pOptions )
{
    // We got options?
    m_pOptions = pOptions;

    // Some child forms are to be created right now.
    m_pMessages = new qsamplerMessages(this);
    // Set message defaults...
    updateMessagesFont();
    updateMessagesLimit();
    updateMessagesCapture();
    // Set the visibility signal.
    QObject::connect(m_pMessages, SIGNAL(visibilityChanged(bool)), this, SLOT(stabilizeForm()));

    // Initial decorations toggle state.
    viewMenubarAction->setOn(m_pOptions->bMenubar);
    viewToolbarAction->setOn(m_pOptions->bToolbar);
    viewStatusbarAction->setOn(m_pOptions->bStatusbar);
    channelsAutoArrangeAction->setOn(m_pOptions->bAutoArrange);

    // Initial decorations visibility state.
    viewMenubar(m_pOptions->bMenubar);
    viewToolbar(m_pOptions->bToolbar);
    viewStatusbar(m_pOptions->bStatusbar);

    // Restore whole dock windows state.
    QString sDockables = m_pOptions->settings().readEntry("/Layout/DockWindows" , QString::null);
    if (sDockables.isEmpty()) {
        // Message window is forced to dock on the bottom.
        moveDockWindow(m_pMessages, Qt::DockBottom);
    } else {
        // Make it as the last time.
        QTextIStream istr(&sDockables);
        istr >> *this;
    }
    // Try to restore old window positioning.
    m_pOptions->loadWidgetGeometry(this);

    // Final startup stabilization...
    stabilizeForm();
    
    // Make it ready :-)
    statusBar()->message(tr("Ready"), 3000);

    // We'll start scheduling...
    startSchedule(m_pOptions->iStartDelay);

    // Register the first timer slot.
    QTimer::singleShot(QSAMPLER_TIMER_MSECS, this, SLOT(timerSlot()));
}


// Window close event handlers.
bool qsamplerMainForm::queryClose (void)
{
    bool bQueryClose = true;

    // Try to save current general state...
    if (m_pOptions) {
        // Some windows default fonts is here on demand too.
        if (bQueryClose && m_pMessages)
            m_pOptions->sMessagesFont = m_pMessages->messagesFont().toString();
        // Try to save current positioning.
        if (bQueryClose) {
            // Save decorations state.
            m_pOptions->bMenubar = MenuBar->isVisible();
            m_pOptions->bToolbar = (fileToolbar->isVisible() || editToolbar->isVisible() || channelsToolbar->isVisible());
            m_pOptions->bStatusbar = statusBar()->isVisible();
            // Save the dock windows state.
            QString sDockables;
            QTextOStream ostr(&sDockables);
            ostr << *this;
            m_pOptions->settings().writeEntry("/Layout/DockWindows", sDockables);
            // And the main windows state.
            m_pOptions->saveWidgetGeometry(this);
            // Close child widgets.
            m_pMessages->close();
        }
    }
    
    return bQueryClose;
}


void qsamplerMainForm::closeEvent ( QCloseEvent *pCloseEvent )
{
    if (queryClose())
        pCloseEvent->accept();
    else
        pCloseEvent->ignore();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- File Action slots.

// Create a new sampler session.
void qsamplerMainForm::fileNew (void)
{
    appendMessages("qsamplerMainForm::fileNew()");

    // Of course we'll start clean new.
    m_iDirtyCount = 0;
    // Make things stable.
    stabilizeForm();
}


// Open an existing sampler session.
void qsamplerMainForm::fileOpen (void)
{
    appendMessages("qsamplerMainForm::fileOpen()");

    // Of course we'll start clean open.
    m_iDirtyCount = 0;
    // Make things stable.
    stabilizeForm();
}


// Save current sampler session.
void qsamplerMainForm::fileSave (void)
{
    appendMessages("qsamplerMainForm::fileSave()");
    
    // Maybe we're not dirty anymore.
    m_iDirtyCount = 0;
    // Make things stable.
    stabilizeForm();
}


// Save current sampler session with another name.
void qsamplerMainForm::fileSaveAs (void)
{
    appendMessages("qsamplerMainForm::fileSaveAs()");

    // Maybe we're not dirty anymore.
    m_iDirtyCount = 0;
    // Make things stable.
    stabilizeForm();
}


// Exit application program.
void qsamplerMainForm::fileExit (void)
{
    appendMessages("qsamplerMainForm::fileExit()");

    close();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Edit Action slots.

// Add a new sampler channel.
void qsamplerMainForm::editAddChannel (void)
{
    appendMessages("qsamplerMainForm::editAddChannel()");

    // Prepare for auto-arrange?
    qsamplerChannelStrip *pChannel = NULL;
    int y = 0;
    if (m_pOptions && m_pOptions->bAutoArrange) {
        QWidgetList wlist = m_pWorkspace->windowList();
        for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
            pChannel = (qsamplerChannelStrip *) wlist.at(iChannel);
        //  y += pChannel->height() + pChannel->parentWidget()->baseSize().height();
            y += pChannel->parentWidget()->frameGeometry().height();
        }
    }

    // FIXME: Arrange a proper method to grab a channel number.
    static int s_iChannel = 0;
    // Add a new channel itema...
    WFlags wflags = Qt::WStyle_Customize | Qt::WStyle_Tool | Qt::WStyle_Title | Qt::WStyle_NoBorder;
    pChannel = new qsamplerChannelStrip(m_pWorkspace, 0, wflags);
    pChannel->setCaption(tr("Channel") + " " + QString::number(++s_iChannel));
    // We'll need a display font.
    QFont font;
    if (m_pOptions && font.fromString(m_pOptions->sDisplayFont))
        pChannel->setDisplayFont(font);
    // Track channel setup changes.
    QObject::connect(pChannel, SIGNAL(channelChanged(qsamplerChannelStrip *)), this, SLOT(channelChanged(qsamplerChannelStrip *)));
    // Now we show up us to the world.
    pChannel->show();
    // Only then, we'll auto-arrange...
    if (m_pOptions && m_pOptions->bAutoArrange) {
        int iWidth  = m_pWorkspace->width();
    //  int iHeight = pChannel->height() + pChannel->parentWidget()->baseSize().height();
        int iHeight = pChannel->parentWidget()->frameGeometry().height();
        pChannel->parentWidget()->setGeometry(0, y, iWidth, iHeight);
    }

    // We'll be dirty, for sure...
    m_iDirtyCount++;
    // Stabilize form anyway.
    stabilizeForm();
}


// Remove current sampler channel.
void qsamplerMainForm::editRemoveChannel (void)
{
    appendMessages("qsamplerMainForm::editRemoveChannel()");
    
    qsamplerChannelStrip *pChannel = activeChannel();
    if (pChannel == NULL)
        return;

    // Prompt user if he/she's sure about this...
    if (m_pOptions && m_pOptions->bConfirmRemove) {
        if (QMessageBox::warning(this, tr("Warning"),
            tr("Remove channel:") + "\n\n" +
            pChannel->caption() + "\n\n" +
            tr("Are you sure?"),
            tr("OK"), tr("Cancel")) > 0)
            return;
    }
    
    // Just delete the channel strip.
    delete pChannel;
    
    // Do we auto-arrange?
    if (m_pOptions && m_pOptions->bAutoArrange)
        channelsArrange();

    // We'll be dirty, for sure...
    m_iDirtyCount++;
    stabilizeForm();
}


// Setup current sampler channel.
void qsamplerMainForm::editSetupChannel (void)
{
    appendMessages("qsamplerMainForm::editSetupChannel()");

    qsamplerChannelStrip *pChannel = activeChannel();
    if (pChannel == NULL)
        return;

    // Just invoque the channel strip procedure.
    pChannel->channelSetup();
}


// Reset current sampler channel.
void qsamplerMainForm::editResetChannel (void)
{
    appendMessages("qsamplerMainForm::editResetChannel()");

    qsamplerChannelStrip *pChannel = activeChannel();
    if (pChannel == NULL)
        return;

    // TODO: The real reset...
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- View Action slots.

// Show/hide the main program window menubar.
void qsamplerMainForm::viewMenubar ( bool bOn )
{
    appendMessages("qsamplerMainForm::viewMenubar(" + QString::number((int) bOn) + ")");
    
    if (bOn)
        MenuBar->show();
    else
        MenuBar->hide();
}


// Show/hide the main program window toolbar.
void qsamplerMainForm::viewToolbar ( bool bOn )
{
    appendMessages("qsamplerMainForm::viewToolbar(" + QString::number((int) bOn) + ")");

	if (bOn) {
        fileToolbar->show();
        editToolbar->show();
        channelsToolbar->show();
	} else {
        fileToolbar->hide();
        editToolbar->hide();
        channelsToolbar->hide();
	}
}


// Show/hide the main program window statusbar.
void qsamplerMainForm::viewStatusbar ( bool bOn )
{
    appendMessages("qsamplerMainForm::viewStatusbar(" + QString::number((int) bOn) + ")");

    if (bOn)
        statusBar()->show();
    else
        statusBar()->hide();
}


// Show/hide the messages window logger.
void qsamplerMainForm::viewMessages ( bool bOn )
{
    appendMessages("qsamplerMainForm::viewMessages(" + QString::number((int) bOn) + ")");

    if (bOn)
        m_pMessages->show();
    else
        m_pMessages->hide();
}


// Show options dialog.
void qsamplerMainForm::viewOptions (void)
{
    appendMessages("qsamplerMainForm::viewOptions()");

    if (m_pOptions == NULL)
        return;

    qsamplerOptionsForm *pOptionsForm = new qsamplerOptionsForm(this);
    if (pOptionsForm) {
        // Check out some initial nullities(tm)...
        qsamplerChannelStrip *pChannel = activeChannel();
        if (m_pOptions->sDisplayFont.isEmpty() && pChannel)
            m_pOptions->sDisplayFont = pChannel->displayFont().toString();
        if (m_pOptions->sMessagesFont.isEmpty() && m_pMessages)
            m_pOptions->sMessagesFont = m_pMessages->messagesFont().toString();
        // To track down deferred or immediate changes.
        QString sOldServerHost      = m_pOptions->sServerHost;
        int     iOldServerPort      = m_pOptions->iServerPort;
        bool    bOldServerStart     = m_pOptions->bServerStart;
        QString sOldServerCmdLine   = m_pOptions->sServerCmdLine;
        QString sOldDisplayFont     = m_pOptions->sDisplayFont;
        QString sOldMessagesFont    = m_pOptions->sMessagesFont;
        bool    bOldStdoutCapture   = m_pOptions->bStdoutCapture;
        int     bOldMessagesLimit   = m_pOptions->bMessagesLimit;
        int     iOldMessagesLimitLines = m_pOptions->iMessagesLimitLines;
        // Load the current setup settings.
        pOptionsForm->setup(m_pOptions);
        // Show the setup dialog...
        if (pOptionsForm->exec()) {
            // Warn if something will be only effective on next run.
            if (( bOldStdoutCapture && !m_pOptions->bStdoutCapture) ||
                (!bOldStdoutCapture &&  m_pOptions->bStdoutCapture))
                updateMessagesCapture();
            // Check wheather something immediate has changed.
            if (sOldDisplayFont != m_pOptions->sDisplayFont)
                updateDisplayFont();
            if (sOldMessagesFont != m_pOptions->sMessagesFont)
                updateMessagesFont();
            if (( bOldMessagesLimit && !m_pOptions->bMessagesLimit) ||
                (!bOldMessagesLimit &&  m_pOptions->bMessagesLimit) ||
                (iOldMessagesLimitLines !=  m_pOptions->iMessagesLimitLines))
                updateMessagesLimit();
            // And now the main thing, whether we'll do client/server recycling?
            if ((sOldServerHost != m_pOptions->sServerHost)      ||
                (iOldServerPort != m_pOptions->iServerPort)      ||
                ( bOldServerStart && !m_pOptions->bServerStart)  ||
                (!bOldServerStart &&  m_pOptions->bServerStart)  ||
                (sOldServerCmdLine != m_pOptions->sServerCmdLine && m_pOptions->bServerStart)) {
                // Ask user whether he/she want's a complete restart...
                if (QMessageBox::warning(this, tr("Warning"),
                    tr("New settings will be effective after\n"
                        "restarting the client/server connection.") + "\n\n" +
                    tr("Please note that this operation may cause\n"
                       "temporary MIDI and Audio disruption.") + "\n\n" +
                    tr("Do you want to restart the connection now?"),
                    tr("Yes"), tr("No")) == 0) {
                    // Stop server, it will force the client too.
                    stopServer();
                    // Reschedule a restart...
                    startSchedule(m_pOptions->iStartDelay);
                }
            }
        }
        // Done.
        delete pOptionsForm;
    }
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Channels Action slots.

// Arrange channel strips.
void qsamplerMainForm::channelsArrange (void)
{
    appendMessages("qsamplerMainForm::channelsArrange()");

    // Full width vertical tiling
    QWidgetList wlist = m_pWorkspace->windowList();
    if (wlist.isEmpty())
        return;

    m_pWorkspace->setUpdatesEnabled(false);
    
    int y = 0;
    for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
        qsamplerChannelStrip *pChannel = (qsamplerChannelStrip *) wlist.at(iChannel);
    /*  if (pChannel->testWState(WState_Maximized | WState_Minimized)) {
            // Prevent flicker...
            pChannel->hide();
            pChannel->showNormal();
        }   */
        pChannel->adjustSize();
        int iWidth  = m_pWorkspace->width();
        if (iWidth < pChannel->width())
            iWidth = pChannel->width();
    //  int iHeight = pChannel->height() + pChannel->parentWidget()->baseSize().height();
        int iHeight = pChannel->parentWidget()->frameGeometry().height();
        pChannel->parentWidget()->setGeometry(0, y, iWidth, iHeight);
        y += iHeight;
    }
    
    m_pWorkspace->setUpdatesEnabled(true);
}


// Auto-arrange channel strips.
void qsamplerMainForm::channelsAutoArrange ( bool bOn )
{
    appendMessages("qsamplerMainForm::channelsAutoArrange()");

    if (m_pOptions == NULL)
        return;

    // Toggle the auto-arrange flag.
    m_pOptions->bAutoArrange = bOn;

    // If on, update whole workspace...
    if (m_pOptions->bAutoArrange)
        channelsArrange();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Help Action slots.

// Show information about the Qt toolkit.
void qsamplerMainForm::helpAboutQt (void)
{
    appendMessages("qsamplerMainForm::helpAboutQt()");
    
    QMessageBox::aboutQt(this);
}


// Show information about application program.
void qsamplerMainForm::helpAbout (void)
{
    appendMessages("qsamplerMainForm::helpAbout()");

    // Stuff the about box text...
    QString sText = "<p>\n";
    sText += "<b>" QSAMPLER_TITLE " - " + tr(QSAMPLER_SUBTITLE) + "</b><br />\n";
    sText += "<br />\n";
    sText += tr("Version") + ": <b>" QSAMPLER_VERSION "</b><br />\n";
    sText += "<small>" + tr("Build") + ": " __DATE__ " " __TIME__ "</small><br />\n";
#ifdef CONFIG_DEBUG
    sText += "<small><font color=\"red\">";
    sText += tr("Debugging option enabled.");
    sText += "</font></small><br />";
#endif
    sText += "<br />\n";
    sText += tr("Using") + ": ";
    sText += ::lscp_client_package();
    sText += " ";
    sText += ::lscp_client_version();
    sText += "<br />\n";
    sText += "<br />\n";
    sText += tr("Website") + ": <a href=\"" QSAMPLER_WEBSITE "\">" QSAMPLER_WEBSITE "</a><br />\n";
    sText += "<br />\n";
    sText += "<small>";
    sText += QSAMPLER_COPYRIGHT "<br />\n";
    sText += "<br />\n";
    sText += tr("This program is free software; you can redistribute it and/or modify it") + "<br />\n";
    sText += tr("under the terms of the GNU General Public License version 2 or later.");
    sText += "</small>";
    sText += "</p>\n";
    
    QMessageBox::about(this, tr("About") + " " QSAMPLER_TITLE, sText);
}

//-------------------------------------------------------------------------
// qsamplerMainForm -- Main window stabilization.

void qsamplerMainForm::stabilizeForm (void)
{
    qsamplerChannelStrip *pChannel = activeChannel();

    fileSaveAction->setEnabled(m_iDirtyCount > 0);

    bool bHasChannel = (pChannel != 0);
    editAddChannelAction->setEnabled(true);
    editRemoveChannelAction->setEnabled(bHasChannel);
    editSetupChannelAction->setEnabled(bHasChannel);
    editResetChannelAction->setEnabled(bHasChannel);
    channelsArrangeAction->setEnabled(bHasChannel);

    viewMessagesAction->setOn(m_pMessages && m_pMessages->isVisible());


    if (bHasChannel)
        m_pStatusLine->setText(pChannel->caption());
    else
        m_pStatusLine->clear();

    if (m_iDirtyCount > 0)
        m_pStatusFlag->setText(tr("Modified"));
    else
        m_pStatusFlag->clear();
}


// Channel change receiver slot.
void qsamplerMainForm::channelChanged( qsamplerChannelStrip * )
{
    // Just mark the dirty form.
    m_iDirtyCount++;
    // and update the form status...
    stabilizeForm();
}


// Force update of the channels display font.
void qsamplerMainForm::updateDisplayFont (void)
{
    if (m_pOptions == NULL)
        return;

    // Check if display font is legal.
    if (m_pOptions->sDisplayFont.isEmpty())
        return;
    // Realize it.
    QFont font;
    if (!font.fromString(m_pOptions->sDisplayFont))
        return;

    // Full channel list update...
    QWidgetList wlist = m_pWorkspace->windowList();
    if (wlist.isEmpty())
        return;

    m_pWorkspace->setUpdatesEnabled(false);
    for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
        qsamplerChannelStrip *pChannel = (qsamplerChannelStrip *) wlist.at(iChannel);
        if (pChannel)
            pChannel->setDisplayFont(font);
    }
    m_pWorkspace->setUpdatesEnabled(true);
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Messages window form handlers.

// Messages output methods.
void qsamplerMainForm::appendMessages( const QString& s )
{
    if (m_pMessages)
        m_pMessages->appendMessages(s);

    statusBar()->message(s, 3000);
}

void qsamplerMainForm::appendMessagesColor( const QString& s, const QString& c )
{
    if (m_pMessages)
        m_pMessages->appendMessagesColor(s, c);

    statusBar()->message(s, 3000);
}

void qsamplerMainForm::appendMessagesText( const QString& s )
{
    if (m_pMessages)
        m_pMessages->appendMessagesText(s);
}

void qsamplerMainForm::appendMessagesError( const QString& s )
{
    if (m_pMessages)
        m_pMessages->show();

    appendMessagesColor(s, "#ff0000");

    QMessageBox::critical(this, tr("Error"), s, tr("Cancel"));
}


// Force update of the messages font.
void qsamplerMainForm::updateMessagesFont (void)
{
    if (m_pOptions == NULL)
        return;

    if (m_pMessages && !m_pOptions->sMessagesFont.isEmpty()) {
        QFont font;
        if (font.fromString(m_pOptions->sMessagesFont))
            m_pMessages->setMessagesFont(font);
    }
}


// Update messages window line limit.
void qsamplerMainForm::updateMessagesLimit (void)
{
    if (m_pOptions == NULL)
        return;

    if (m_pMessages) {
        if (m_pOptions->bMessagesLimit)
            m_pMessages->setMessagesLimit(m_pOptions->iMessagesLimitLines);
        else
            m_pMessages->setMessagesLimit(0);
    }
}


// Enablement of the messages capture feature.
void qsamplerMainForm::updateMessagesCapture (void)
{
    if (m_pOptions == NULL)
        return;

    if (m_pMessages)
        m_pMessages->setCaptureEnabled(m_pOptions->bStdoutCapture);
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- MDI channel strip management.

// Retrieve the active channel strip.
qsamplerChannelStrip *qsamplerMainForm::activeChannel (void)
{
    return (qsamplerChannelStrip *) m_pWorkspace->activeWindow();
}


// Retrieve a channel strip by index.
qsamplerChannelStrip *qsamplerMainForm::channelAt ( int iChannel )
{
    QWidgetList wlist = m_pWorkspace->windowList();
    if (wlist.isEmpty())
        return 0;
        
    return (qsamplerChannelStrip *) wlist.at(iChannel);
}


// Construct the windows menu.
void qsamplerMainForm::channelsMenuAboutToShow (void)
{
    channelsMenu->clear();
    channelsArrangeAction->addTo(channelsMenu);
    channelsAutoArrangeAction->addTo(channelsMenu);

    QWidgetList wlist = m_pWorkspace->windowList();
    if (!wlist.isEmpty()) {
        channelsMenu->insertSeparator();
        for (int iChannel = 0; iChannel < (int) wlist.count(); iChannel++) {
            qsamplerChannelStrip *pChannel = (qsamplerChannelStrip *) wlist.at(iChannel);
            int iItemID = channelsMenu->insertItem(pChannel->caption(), this, SLOT(channelsMenuActivated(int)));
            channelsMenu->setItemParameter(iItemID, iChannel);
            channelsMenu->setItemChecked(iItemID, activeChannel() == pChannel);
        }
    }
}


// Windows menu activation slot
void qsamplerMainForm::channelsMenuActivated ( int iChannel )
{
    appendMessages("qsamplerMainForm::channelsMenuActivated(" + QString::number(iChannel) + ")");
    
    qsamplerChannelStrip *pChannel = channelAt(iChannel);
    if (pChannel)
        pChannel->showNormal();
    pChannel->setFocus();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Timer stuff.

// Set the pseudo-timer delay schedule.
void qsamplerMainForm::startSchedule ( int iStartDelay )
{
    appendMessages("qsamplerMainForm::startSchedule(" + QString::number(iStartDelay) + ")");
    
    m_iStartDelay  = 1 + (iStartDelay * 1000);
    m_iTimerDelay  = 0;
}

// Suspend the pseudo-timer delay schedule.
void qsamplerMainForm::stopSchedule (void)
{
    appendMessages("qsamplerMainForm::stopSchedule()");
    
    m_iStartDelay  = 0;
    m_iTimerDelay  = 0;
}

// Timer slot funtion.
void qsamplerMainForm::timerSlot (void)
{
    if (m_pOptions == NULL)
        return;

    // Is it the first shot on server start after a few delay?
    if (m_iTimerDelay < m_iStartDelay) {
        m_iTimerDelay += QSAMPLER_TIMER_MSECS;
        if (m_iTimerDelay >= m_iStartDelay) {
            // If we cannot start it now, maybe a lil'mo'later ;)
            if (!startClient()) {
                m_iStartDelay += m_iTimerDelay;
                m_iTimerDelay  = 0;
            }
        }
    }

    // Register the next timer slot.
    QTimer::singleShot(QSAMPLER_TIMER_MSECS, this, SLOT(timerSlot()));
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Server stuff.

// Start linuxsampler server...
void qsamplerMainForm::startServer (void)
{
    appendMessages("qsamplerMainForm::startServer()");
    
    if (m_pOptions == NULL)
        return;

    // Aren't already a client, are we?
    if (!m_pOptions->bServerStart || m_pClient)
        return;

    // Is the server process instance still here?
    if (m_pServer) {
        switch (QMessageBox::warning(this, tr("Warning"),
            tr("Could not start the LinuxSampler server.") + "\n\n" +
            tr("Maybe it's already started."),
            tr("Stop"), tr("Kill"), tr("Cancel"))) {
          case 0:
            m_pServer->tryTerminate();
            break;
          case 1:
            m_pServer->kill();
            break;
        }
        return;
    }

    // Reset our timer counters...
    stopSchedule();

    // OK. Let's build the startup process...
    m_pServer = new QProcess(this);

    // Setup stdout/stderr capture...
    if (m_pOptions->bStdoutCapture) {
        m_pServer->setCommunication(QProcess::Stdout | QProcess::Stderr | QProcess::DupStderr);
        QObject::connect(m_pServer, SIGNAL(readyReadStdout()), this, SLOT(readServerStdout()));
        QObject::connect(m_pServer, SIGNAL(readyReadStderr()), this, SLOT(readServerStdout()));
    }
    // The unforgiveable signal communication...
    QObject::connect(m_pServer, SIGNAL(processExited()), this, SLOT(processServerExit()));

    // Build process arguments...
    m_pServer->setArguments(QStringList::split(' ', m_pOptions->sServerCmdLine));

    appendMessages(tr("Server is starting..."));
    appendMessagesColor(m_pOptions->sServerCmdLine, "#990099");

    // Go jack, go...
    if (!m_pServer->start()) {
        appendMessagesError(tr("Could not start server. Sorry."));
        processServerExit();
        return;
    }

    // Show startup results...
    appendMessages(tr("Server was started with PID=%1.").arg((long) m_pServer->processIdentifier()));

    // Reset (yet again) the timer counters...
    startSchedule(m_pOptions->iStartDelay);
}


// Stop linuxsampler server...
void qsamplerMainForm::stopServer (void)
{
    appendMessages("qsamplerMainForm::stopServer()");
    
    // Stop client code.
    stopClient();

    // And try to stop server.
    if (m_pServer) {
        appendMessages(tr("Server is stopping..."));
        if (m_pServer->isRunning())
            m_pServer->tryTerminate();
     }

     // Do final processing anyway.
     processServerExit();
}


// Stdout handler...
void qsamplerMainForm::readServerStdout (void)
{
    if (m_pMessages)
        m_pMessages->appendStdoutBuffer(m_pServer->readStdout());
}


// Linuxsampler server cleanup.
void qsamplerMainForm::processServerExit (void)
{
    appendMessages("qsamplerMainForm::processServerExit()");

    // Force client code cleanup.
    stopClient();

    // Flush anything that maybe pending...
    if (m_pMessages)
        m_pMessages->flushStdoutBuffer();

    if (m_pServer) {
        // Force final server shutdown...
        appendMessages(tr("Server was stopped with exit status %1.").arg(m_pServer->exitStatus()));
        if (!m_pServer->normalExit())
            m_pServer->kill();
        // Destroy it.
        delete m_pServer;
        m_pServer = NULL;
    }
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Client stuff.


// The LSCP client callback procedure.
lscp_status_t qsampler_client_callback ( lscp_client_t *pClient, const char *pchBuffer, int cchBuffer, void *pvData )
{
    qsamplerMainForm *pMainForm = (qsamplerMainForm *) pvData;
    if (pMainForm == NULL)
        return LSCP_FAILED;

    char *pszBuffer = (char *) malloc(cchBuffer + 1);
    if (pszBuffer == NULL)
        return LSCP_FAILED;

    memcpy(pszBuffer, pchBuffer, cchBuffer);
    pszBuffer[cchBuffer] = (char) 0;
    pMainForm->appendMessagesColor(pszBuffer, "#996699");
    free(pszBuffer);

    return LSCP_OK;
}


// Start our almighty client...
bool qsamplerMainForm::startClient (void)
{
    appendMessages("qsamplerMainForm::startClient()");
    
    // Have it a setup?
    if (m_pOptions == NULL)
        return false;

    // Aren't we already started, are we?
    if (m_pClient)
        return true;

    // We may stop scheduling around.
    stopSchedule();

    // Log prepare here.
    appendMessages(tr("Client connecting..."));

    // Create the client handle...
    m_pClient = ::lscp_client_create(m_pOptions->sServerHost.latin1(), m_pOptions->iServerPort, qsampler_client_callback, this);
    if (m_pClient == NULL) {
        // Is this the first try?
        // maybe we need to start a local server...
        if ((m_pServer && m_pServer->isRunning()) || !m_pOptions->bServerStart)
            appendMessagesError(tr("Could not connect to server as client."));
        else
            startServer();
        // This is always a failure.
        return false;
    }

    // Log success here.
    appendMessages(tr("Client connected."));

    // OK, we're at it!
    return true;
}


// Stop client...
void qsamplerMainForm::stopClient (void)
{
    appendMessages("qsamplerMainForm::stopClient()");

    if (m_pClient == NULL)
        return;

    // Clear timer counters...
    stopSchedule();

    // Log prepare here.
    appendMessages(tr("Client disconnecting..."));

    // Close us as a client...
    lscp_client_destroy(m_pClient);
    m_pClient = NULL;

    // Log final here.
    appendMessages(tr("Client disconnected."));
}


// end of qsamplerMainForm.ui.h
