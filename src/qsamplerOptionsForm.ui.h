// qsamplerOptionsForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2004-2005, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <qvalidator.h>
#include <qmessagebox.h>
#include <qfontdialog.h>

#include "qsamplerOptions.h"

#include "config.h"


// Kind of constructor.
void qsamplerOptionsForm::init (void)
{
    // No settings descriptor initially (the caller will set it).
    m_pOptions = NULL;

    // Initialize dirty control state.
    m_iDirtySetup = 0;
    m_iDirtyCount = 0;

    // Set dialog validators...
    ServerPortComboBox->setValidator(new QIntValidator(ServerPortComboBox));

    // Try to restore old window positioning.
    adjustSize();
}


// Kind of destructor.
void qsamplerOptionsForm::destroy (void)
{
}


// Populate (setup) dialog controls from settings descriptors.
void qsamplerOptionsForm::setup ( qsamplerOptions *pOptions )
{
    // Set reference descriptor.
    m_pOptions = pOptions;

    // Start clean.
    m_iDirtyCount = 0;
    // Avoid nested changes.
    m_iDirtySetup++;

    // Load combo box history...
    m_pOptions->loadComboBoxHistory(ServerHostComboBox);
    m_pOptions->loadComboBoxHistory(ServerPortComboBox);
    m_pOptions->loadComboBoxHistory(ServerCmdLineComboBox);

    // Load Server settings...
    ServerHostComboBox->setCurrentText(m_pOptions->sServerHost);
    ServerPortComboBox->setCurrentText(QString::number(m_pOptions->iServerPort));
    ServerTimeoutSpinBox->setValue(m_pOptions->iServerTimeout);
    ServerStartCheckBox->setChecked(m_pOptions->bServerStart);
    ServerCmdLineComboBox->setCurrentText(m_pOptions->sServerCmdLine);
    StartDelaySpinBox->setValue(m_pOptions->iStartDelay);

    // Load Display options...
    QFont font;
    
    // Display font.
    if (m_pOptions->sDisplayFont.isEmpty() || !font.fromString(m_pOptions->sDisplayFont))
        font = QFont("Sans Serif", 8);
    DisplayFontTextLabel->setFont(font);
    DisplayFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    // Display effect.
    DisplayEffectCheckBox->setChecked(m_pOptions->bDisplayEffect);
    toggleDisplayEffect(m_pOptions->bDisplayEffect);

    // Auto-refresh and maximum volume options.
    AutoRefreshCheckBox->setChecked(m_pOptions->bAutoRefresh);
    AutoRefreshTimeSpinBox->setValue(m_pOptions->iAutoRefreshTime);
    MaxVolumeSpinBox->setValue(m_pOptions->iMaxVolume);

    // Messages font.
    if (m_pOptions->sMessagesFont.isEmpty() || !font.fromString(m_pOptions->sMessagesFont))
        font = QFont("Fixed", 8);
    MessagesFontTextLabel->setFont(font);
    MessagesFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));

    // Messages limit option.
    MessagesLimitCheckBox->setChecked(m_pOptions->bMessagesLimit);
    MessagesLimitLinesSpinBox->setValue(m_pOptions->iMessagesLimitLines);

    // Other options finally.
    ConfirmRemoveCheckBox->setChecked(m_pOptions->bConfirmRemove);
    StdoutCaptureCheckBox->setChecked(m_pOptions->bStdoutCapture);
    CompletePathCheckBox->setChecked(m_pOptions->bCompletePath);
    InstrumentNamesCheckBox->setChecked(m_pOptions->bInstrumentNames);
    MaxRecentFilesSpinBox->setValue(m_pOptions->iMaxRecentFiles);

#ifndef CONFIG_LIBGIG
    InstrumentNamesCheckBox->setEnabled(false);
#endif

    // Done.
    m_iDirtySetup--;
    stabilizeForm();
}


// Accept settings (OK button slot).
void qsamplerOptionsForm::accept (void)
{
    // Save options...
    if (m_iDirtyCount > 0) {
        // Server settings....
        m_pOptions->sServerHost          = ServerHostComboBox->currentText().stripWhiteSpace();
        m_pOptions->iServerPort          = ServerPortComboBox->currentText().toInt();
        m_pOptions->iServerTimeout       = ServerTimeoutSpinBox->value();
        m_pOptions->bServerStart         = ServerStartCheckBox->isChecked();
        m_pOptions->sServerCmdLine       = ServerCmdLineComboBox->currentText().simplifyWhiteSpace();
        m_pOptions->iStartDelay          = StartDelaySpinBox->value();
        // Channels options...
        m_pOptions->sDisplayFont         = DisplayFontTextLabel->font().toString();
        m_pOptions->bDisplayEffect       = DisplayEffectCheckBox->isChecked();
        m_pOptions->bAutoRefresh         = AutoRefreshCheckBox->isChecked();
        m_pOptions->iAutoRefreshTime     = AutoRefreshTimeSpinBox->value();
        m_pOptions->iMaxVolume           = MaxVolumeSpinBox->value();
        // Messages options...
        m_pOptions->sMessagesFont        = MessagesFontTextLabel->font().toString();
        m_pOptions->bMessagesLimit       = MessagesLimitCheckBox->isChecked();
        m_pOptions->iMessagesLimitLines  = MessagesLimitLinesSpinBox->value();
        // Other options...
        m_pOptions->bConfirmRemove       = ConfirmRemoveCheckBox->isChecked();
        m_pOptions->bStdoutCapture       = StdoutCaptureCheckBox->isChecked();
        m_pOptions->bCompletePath        = CompletePathCheckBox->isChecked();
        m_pOptions->bInstrumentNames     = InstrumentNamesCheckBox->isChecked();
        m_pOptions->iMaxRecentFiles      = MaxRecentFilesSpinBox->value();
        // Reset dirty flag.
        m_iDirtyCount = 0;
    }

    // Save combobox history...
    m_pOptions->saveComboBoxHistory(ServerHostComboBox);
    m_pOptions->saveComboBoxHistory(ServerPortComboBox);
    m_pOptions->saveComboBoxHistory(ServerCmdLineComboBox);

    // Just go with dialog acceptance.
    QDialog::accept();
}


// Reject settings (Cancel button slot).
void qsamplerOptionsForm::reject (void)
{
    bool bReject = true;

    // Check if there's any pending changes...
    if (m_iDirtyCount > 0) {
        switch (QMessageBox::warning(this, tr("Warning"),
            tr("Some settings have been changed.\n\n"
               "Do you want to apply the changes?"),
            tr("Apply"), tr("Discard"), tr("Cancel"))) {
        case 0:     // Apply...
            accept();
            return;
        case 1:     // Discard
            break;
        default:    // Cancel.
            bReject = false;
        }
    }

    if (bReject)
        QDialog::reject();
}


// Dirty up settings.
void qsamplerOptionsForm::optionsChanged (void)
{
    if (m_iDirtySetup > 0)
        return;

    m_iDirtyCount++;
    stabilizeForm();
}


// Stabilize current form state.
void qsamplerOptionsForm::stabilizeForm (void)
{
    bool bEnabled = ServerStartCheckBox->isChecked();
    ServerCmdLineTextLabel->setEnabled(bEnabled);
    ServerCmdLineComboBox->setEnabled(bEnabled);
    StartDelayTextLabel->setEnabled(bEnabled);
    StartDelaySpinBox->setEnabled(bEnabled);

    AutoRefreshTimeSpinBox->setEnabled(AutoRefreshCheckBox->isChecked());
    MessagesLimitLinesSpinBox->setEnabled(MessagesLimitCheckBox->isChecked());

    OkPushButton->setEnabled(m_iDirtyCount > 0);
}


// The channel display font selection dialog.
void qsamplerOptionsForm::chooseDisplayFont (void)
{
    bool  bOk  = false;
    QFont font = QFontDialog::getFont(&bOk, DisplayFontTextLabel->font(), this);
    if (bOk) {
        DisplayFontTextLabel->setFont(font);
        DisplayFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));
        optionsChanged();
    }
}


// The messages font selection dialog.
void qsamplerOptionsForm::chooseMessagesFont (void)
{
    bool  bOk  = false;
    QFont font = QFontDialog::getFont(&bOk, MessagesFontTextLabel->font(), this);
    if (bOk) {
        MessagesFontTextLabel->setFont(font);
        MessagesFontTextLabel->setText(font.family() + " " + QString::number(font.pointSize()));
        optionsChanged();
    }
}


// The channel display effect demo changer.
void qsamplerOptionsForm::toggleDisplayEffect ( bool bOn )
{
    QPixmap pm;
    if (bOn)
        pm = QPixmap::fromMimeSource("displaybg1.png");
    DisplayFontTextLabel->setPaletteBackgroundPixmap(pm);

    optionsChanged();
}


// end of qsamplerOptionsForm.ui.h

