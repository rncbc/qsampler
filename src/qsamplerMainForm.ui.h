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


//-------------------------------------------------------------------------
// qsamplerMainForm -- Main window form implementation.

// Kind of constructor.
void qsamplerMainForm::init (void)
{
    m_pOptions = NULL;
}


// Kind of destructor.
void qsamplerMainForm::destroy (void)
{
}


// Make and set a proper setup options step.
void qsamplerMainForm::setup ( qsamplerOptions *pOptions )
{
    // We got options?
    m_pOptions = pOptions;

    // Initial decorations toggle state.
    viewMenubarAction->setOn(m_pOptions->bMenubar);
    viewFileToolbarAction->setOn(m_pOptions->bFileToolbar);
    viewEditToolbarAction->setOn(m_pOptions->bEditToolbar);
    viewStatusbarAction->setOn(m_pOptions->bStatusbar);

    // Initial decorations visibility state.
    viewMenubar(m_pOptions->bMenubar);
    viewFileToolbar(m_pOptions->bFileToolbar);
    viewEditToolbar(m_pOptions->bEditToolbar);
    viewStatusbar(m_pOptions->bStatusbar);

    // Try to restore old window positioning.
    m_pOptions->loadWidgetGeometry(this);
}


// Window close event handlers.
bool qsamplerMainForm::queryClose (void)
{
    bool bQueryClose = true;

    // Ty to save current positioning.
    if (m_pOptions && bQueryClose) {
        // Save decorations state.
        m_pOptions->bMenubar = MenuBar->isVisible();
        m_pOptions->bFileToolbar = fileToolbar->isVisible();
        m_pOptions->bEditToolbar = editToolbar->isVisible();
        m_pOptions->bStatusbar = statusBar()->isVisible();
        // And main window form...
        m_pOptions->saveWidgetGeometry(this);
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

// Show/hide the main program menubar.
void qsamplerMainForm::viewMenubar ( bool bOn )
{
    if (bOn)
        MenuBar->show();
    else
        MenuBar->hide();
}


// Show/hide the file actions toolbar.
void qsamplerMainForm::viewFileToolbar ( bool bOn )
{
    if (bOn)
        fileToolbar->show();
    else
        fileToolbar->hide();
}


// Show/hide the edit actions toolbar.
void qsamplerMainForm::viewEditToolbar ( bool bOn )
{
    if (bOn)
        editToolbar->show();
    else
        editToolbar->hide();
}


// Show/hide the main program statusbar.
void qsamplerMainForm::viewStatusbar ( bool bOn )
{
    if (bOn)
        statusBar()->show();
    else
        statusBar()->hide();
}


// Show options dialog.
void qsamplerMainForm::viewOptions (void)
{
    if (m_pOptions == NULL)
        return;

    qsamplerOptionsForm *pOptionsForm = new qsamplerOptionsForm(this);
    if (pOptionsForm) {
        // Load the current setup settings.
        pOptionsForm->setup(m_pOptions);
        // Show the setup dialog...
        pOptionsForm->exec();
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


// end of qsamplerMainForm.ui.h
