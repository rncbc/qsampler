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
#include <qstatusbar.h>

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

    // All child forms are to be created later on setup.
    m_pMessages = NULL;

    // Make it an MDI workspace.
    m_pWorkspace = new QWorkspace(this);
    m_pWorkspace->setScrollBarsEnabled(true);
    setCentralWidget(m_pWorkspace);
}


// Kind of destructor.
void qsamplerMainForm::destroy (void)
{
    // Finally drop any widgets around...
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
    // Message window is forced to dock on the bottom.
    moveDockWindow(m_pMessages, Qt::DockBottom);
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

    // Initial decorations visibility state.
    viewMenubar(m_pOptions->bMenubar);
    viewToolbar(m_pOptions->bToolbar);
    viewStatusbar(m_pOptions->bStatusbar);

    // Try to restore old window positioning.
    m_pOptions->loadWidgetGeometry(m_pMessages);
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
            // And the child windows state.
            m_pOptions->saveWidgetGeometry(m_pMessages);
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
}


// Open an existing sampler session.
void qsamplerMainForm::fileOpen (void)
{
}


// Save current sampler session.
void qsamplerMainForm::fileSave (void)
{
}


// Save current sampler session with another name.
void qsamplerMainForm::fileSaveAs (void)
{
}


// Exit application program.
void qsamplerMainForm::fileExit (void)
{
    close();
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Edit Action slots.

// Add a new sampler channel.
void qsamplerMainForm::editAddChannel (void)
{
}


// Remove current sampler channel.
void qsamplerMainForm::editRemoveChannel (void)
{
}


// Reset current sampler channel.
void qsamplerMainForm::editResetChannel (void)
{
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- View Action slots.

// Show/hide the main program window menubar.
void qsamplerMainForm::viewMenubar ( bool bOn )
{
    if (bOn)
        MenuBar->show();
    else
        MenuBar->hide();
}


// Show/hide the main program window toolbar.
void qsamplerMainForm::viewToolbar ( bool bOn )
{
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
    if (bOn)
        statusBar()->show();
    else
        statusBar()->hide();
}


// Show/hide the messages window logger.
void qsamplerMainForm::viewMessages ( bool bOn )
{
    if (bOn)
        m_pMessages->show();
    else
        m_pMessages->hide();
}


// Show options dialog.
void qsamplerMainForm::viewOptions (void)
{
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
// qsamplerMainForm -- Help Action slots.

// Show information about the Qt toolkit.
void qsamplerMainForm::helpAboutQt (void)
{
    QMessageBox::aboutQt(this);
}


// Show information about application program.
void qsamplerMainForm::helpAbout (void)
{
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
    viewMessagesAction->setOn(m_pMessages && m_pMessages->isVisible());
}


//-------------------------------------------------------------------------
// qsamplerMainForm -- Messages window form handlers.


// Messages output methods.
void qsamplerMainForm::appendMessages( const QString& s )
{
    if (m_pMessages)
        m_pMessages->appendMessages(s);
}

void qsamplerMainForm::appendMessagesColor( const QString& s, const QString& c )
{
    if (m_pMessages)
        m_pMessages->appendMessagesColor(s, c);
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


// end of qsamplerMainForm.ui.h
