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

#include "qsamplerAbout.h"

#include "lscp/client.h"

#include "config.h"

#if !defined(WIN32)
#include <unistd.h>
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

    // Make it an MDI workspace.
    m_pWorkspace = new QWorkspace(this);
    m_pWorkspace->setScrollBarsEnabled(true);
    // Set the activation connection.
    QObject::connect(m_pWorkspace, SIGNAL(windowActivated(QWidget *)), this, SLOT(stabilizeForm()));
    // Make it shine :-)
    setCentralWidget(m_pWorkspace);
    
    // Create some statusbar labels...
    m_pStatusLine = new QLabel(this);
    m_pStatusFlag = new QLabel(tr("MOD"), this);
    m_pStatusFlag->setAlignment(Qt::AlignHCenter);
    m_pStatusFlag->setMinimumSize(m_pStatusFlag->sizeHint());
    // And make them there...
    statusBar()->addWidget(m_pStatusLine, 1);
    statusBar()->addWidget(m_pStatusFlag);
}


// Kind of destructor.
void qsamplerMainForm::destroy (void)
{
    // Finally drop any widgets around...
    if (m_pStatusLine)
        delete m_pStatusLine;
    if (m_pStatusFlag)
        delete m_pStatusFlag;
    if (m_pMessages)
        delete m_pMessages;
    if (m_pWorkspace)
        delete m_pWorkspace;
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
            m_pOptions->bToolbar = (fileToolbar->isVisible() || editToolbar->isVisible());
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
    appendMessages("fileNew()");

    // Of course we'll start clean new.
    m_iDirtyCount = 0;
    // Make things stable.
    stabilizeForm();
}


// Open an existing sampler session.
void qsamplerMainForm::fileOpen (void)
{
    appendMessages("fileOpen()");

    // Of course we'll start clean open.
    m_iDirtyCount = 0;
    // Make things stable.
    stabilizeForm();
}


// Save current sampler session.
void qsamplerMainForm::fileSave (void)
{
    appendMessages("fileSave()");
    
    // Maybe we're not dirty anymore.
    m_iDirtyCount = 0;
    // Make things stable.
    stabilizeForm();
}


// Save current sampler session with another name.
void qsamplerMainForm::fileSaveAs (void)
{
    appendMessages("fileSaveAs()");

    // Maybe we're not dirty anymore.
    m_iDirtyCount = 0;
    // Make things stable.
    stabilizeForm();
}


// Exit application program.
void qsamplerMainForm::fileExit (void)
{
    appendMessages("fileExit()");

    close();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Edit Action slots.

// Add a new sampler channel.
void qsamplerMainForm::editAddChannel (void)
{
    appendMessages("editAddChannel()");

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
    appendMessages("editRemoveChannel()");
    
    qsamplerChannelStrip *pChannel = activeChannel();
    if (pChannel == NULL)
        return;

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
    appendMessages("editSetupChannel()");

    qsamplerChannelStrip *pChannel = activeChannel();
    if (pChannel == NULL)
        return;

    // Just invoque the channel strip procedure.
    pChannel->channelSetup();
}


// Reset current sampler channel.
void qsamplerMainForm::editResetChannel (void)
{
    appendMessages("editResetChannel()");

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
    appendMessages("viewMenubar(" + QString::number((int) bOn) + ")");
    
    if (bOn)
        MenuBar->show();
    else
        MenuBar->hide();
}


// Show/hide the main program window toolbar.
void qsamplerMainForm::viewToolbar ( bool bOn )
{
    appendMessages("viewToolbar(" + QString::number((int) bOn) + ")");

	if (bOn) {
        fileToolbar->show();
        editToolbar->show();
	} else {
        fileToolbar->hide();
        editToolbar->hide();
	}
}


// Show/hide the main program window statusbar.
void qsamplerMainForm::viewStatusbar ( bool bOn )
{
    appendMessages("viewStatusbar(" + QString::number((int) bOn) + ")");

    if (bOn)
        statusBar()->show();
    else
        statusBar()->hide();
}


// Show/hide the messages window logger.
void qsamplerMainForm::viewMessages ( bool bOn )
{
    appendMessages("viewMessages(" + QString::number((int) bOn) + ")");

    if (bOn)
        m_pMessages->show();
    else
        m_pMessages->hide();
}


// Show options dialog.
void qsamplerMainForm::viewOptions (void)
{
    appendMessages("viewOptions()");

    if (m_pOptions == NULL)
        return;

    qsamplerOptionsForm *pOptionsForm = new qsamplerOptionsForm(this);
    if (pOptionsForm) {
        // Check out some initial nullities(tm)...
        if (m_pOptions->sMessagesFont.isEmpty() && m_pMessages)
            m_pOptions->sMessagesFont = m_pMessages->messagesFont().toString();
        // To track down deferred or immediate changes.
        QString sOldMessagesFont    = m_pOptions->sMessagesFont;
        bool    bStdoutCapture      = m_pOptions->bStdoutCapture;
        int     bMessagesLimit      = m_pOptions->bMessagesLimit;
        int     iMessagesLimitLines = m_pOptions->iMessagesLimitLines;
        // Load the current setup settings.
        pOptionsForm->setup(m_pOptions);
        // Show the setup dialog...
        if (pOptionsForm->exec()) {
            // Warn if something will be only effective on next run.
            if (( bStdoutCapture && !m_pOptions->bStdoutCapture) ||
                (!bStdoutCapture &&  m_pOptions->bStdoutCapture))
                updateMessagesCapture();
            // Check wheather something immediate has changed.
            if (sOldMessagesFont != m_pOptions->sMessagesFont)
                updateMessagesFont();
            if (( bMessagesLimit && !m_pOptions->bMessagesLimit) ||
                (!bMessagesLimit &&  m_pOptions->bMessagesLimit) ||
                (iMessagesLimitLines !=  m_pOptions->iMessagesLimitLines))
                updateMessagesLimit();
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
    appendMessages("channelsArrange()");

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
    appendMessages("channelsAutoArrange()");

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
    appendMessages("helpAboutQt()");
    
    QMessageBox::aboutQt(this);
}


// Show information about application program.
void qsamplerMainForm::helpAbout (void)
{
    appendMessages("helpAbout()");

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
        m_pStatusFlag->setText(tr("MOD"));
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
    appendMessages("channelsMenuActivated(" + QString::number(iChannel) + ")");
    
    qsamplerChannelStrip *pChannel = channelAt(iChannel);
    if (pChannel)
        pChannel->showNormal();
    pChannel->setFocus();
}


// end of qsamplerMainForm.ui.h
