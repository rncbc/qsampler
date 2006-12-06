// qsamplerInstrumentForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2004-2006, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qsamplerInstrument.h"

#include "qsamplerOptions.h"
#include "qsamplerChannel.h"

#include "qsamplerMainForm.h"

#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qlistbox.h>

// Needed for lroundf()
#include <math.h>


// Kind of constructor.
void qsamplerInstrumentForm::init (void)
{
	// Initialize locals.
	m_pInstrument = NULL;

	m_iDirtySetup = 0;
	m_iDirtyCount = 0;
	m_iDirtyName  = 0;

	// Try to restore normal window positioning.
	adjustSize();
}


// Kind of destructor.
void qsamplerInstrumentForm::destroy (void)
{
}


// Channel dialog setup formal initializer.
void qsamplerInstrumentForm::setup ( qsamplerInstrument *pInstrument )
{
	m_pInstrument = pInstrument;

	m_iDirtySetup = 0;
	m_iDirtyCount = 0;
	m_iDirtyName  = 0;

	if (m_pInstrument == NULL)
		return;

	// Check if we're up and connected.
	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// It can be a brand new channel, remember?
	bool bNew = (m_pInstrument->bank() < 0 || m_pInstrument->program() < 0);
	if (!bNew) {
		m_pInstrument->get();
		m_iDirtyName++;
	}

	// Avoid nested changes.
	m_iDirtySetup++;

	// Load combo box history...
	pOptions->loadComboBoxHistory(InstrumentFileComboBox);

	// Populate Engines list.
	const char **ppszEngines = ::lscp_list_available_engines(pMainForm->client());
	if (ppszEngines) {
		EngineNameComboBox->clear();
		for (int iEngine = 0; ppszEngines[iEngine]; iEngine++)
			EngineNameComboBox->insertItem(ppszEngines[iEngine]);
	}
	else pMainForm->appendMessagesClient("lscp_list_available_engines");

	// Read proper instrument information,
	// and populate the instrument form fields.

	// Instrument bank/program...
	int iBank = m_pInstrument->bank();
	int iProgram = m_pInstrument->program();
	BankSpinBox->setValue(iBank);
	ProgramSpinBox->setValue(iProgram);
	// Spacial hack to avoid changes on the key...
	if (!bNew) {
		BankSpinBox->setRange(iBank, iBank);
		ProgramSpinBox->setRange(iProgram, iProgram);
	}

	// Instrument name...
	NameLineEdit->setText(m_pInstrument->name());

	// Engine name...
	QString sEngineName = m_pInstrument->engineName();
	if (sEngineName.isEmpty() || bNew)
		sEngineName = pOptions->sEngineName;
	if (sEngineName.isEmpty())
		sEngineName = qsamplerChannel::noEngineName();
	if (EngineNameComboBox->listBox()->findItem(sEngineName,
			Qt::ExactMatch | Qt::CaseSensitive) == NULL) {
		EngineNameComboBox->insertItem(sEngineName);
	}
	EngineNameComboBox->setCurrentText(sEngineName);
	// Instrument filename and index...
	QString sInstrumentFile = m_pInstrument->instrumentFile();
	if (sInstrumentFile.isEmpty())
		sInstrumentFile = qsamplerChannel::noInstrumentName();
	InstrumentFileComboBox->setCurrentText(sInstrumentFile);
	InstrumentNrComboBox->clear();
	InstrumentNrComboBox->insertStringList(
		qsamplerChannel::getInstrumentList(sInstrumentFile,
		pOptions->bInstrumentNames));
	InstrumentNrComboBox->setCurrentItem(m_pInstrument->instrumentNr());

	// Instrument volume....
	VolumeSpinBox->setValue(::lroundf(100.0f * m_pInstrument->volume()));

	// Instrument load mode...
	LoadModeComboBox->setCurrentItem(m_pInstrument->loadMode());

	// Done.
	m_iDirtySetup--;
	stabilizeForm();

	// Done.
	m_iDirtySetup--;
	stabilizeForm();
}


// Special case for name change,
void qsamplerInstrumentForm::nameChanged ( const QString& /* sName */ )
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtyName++;
	changed();
}


// Browse and open an instrument file.
void qsamplerInstrumentForm::openInstrumentFile (void)
{
	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// FIXME: the instrument file filters should be restricted,
	// depending on the current engine.
	QString sInstrumentFile = QFileDialog::getOpenFileName(
		pOptions->sInstrumentDir,                   // Start here.
		tr("Instrument files") + " (*.gig *.dls)",  // Filter (GIG and DLS files)
		this, 0,                                    // Parent and name (none)
		QSAMPLER_TITLE ": " + tr("Instrument files")// Caption.
	);

	if (sInstrumentFile.isEmpty())
		return;

	InstrumentFileComboBox->setCurrentText(sInstrumentFile);
	updateInstrumentName();
}


// Refresh the actual instrument name.
void qsamplerInstrumentForm::updateInstrumentName (void)
{
	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// TODO: this better idea would be to use libgig
	// to retrieve the REAL instrument names.
	InstrumentNrComboBox->clear();
	InstrumentNrComboBox->insertStringList(
		qsamplerChannel::getInstrumentList(
			InstrumentFileComboBox->currentText(),
			pOptions->bInstrumentNames)
	);

	instrumentNrChanged();
}


// Special case for instrumnet index change,
void qsamplerInstrumentForm::instrumentNrChanged (void)
{
	if (m_iDirtySetup > 0)
		return;

	if (NameLineEdit->text().isEmpty() || m_iDirtyName == 0) {
		NameLineEdit->setText(InstrumentNrComboBox->currentText());
		m_iDirtyName = 0;
	}

	changed();
}


// Accept settings (OK button slot).
void qsamplerInstrumentForm::accept (void)
{
	if (m_pInstrument == NULL)
		return;

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	if (m_iDirtyCount > 0) {
		m_pInstrument->setBank(BankSpinBox->value());
		m_pInstrument->setProgram(ProgramSpinBox->value());
		m_pInstrument->setName(NameLineEdit->text());
		m_pInstrument->setEngineName(EngineNameComboBox->currentText());
		m_pInstrument->setInstrumentFile(InstrumentFileComboBox->currentText());
		m_pInstrument->setInstrumentNr(InstrumentNrComboBox->currentItem());
		m_pInstrument->setVolume(0.01f * float(VolumeSpinBox->value()));
		m_pInstrument->setLoadMode(LoadModeComboBox->currentItem());
	}

	// Save default engine name, instrument directory and history...
	pOptions->sInstrumentDir = QFileInfo(InstrumentFileComboBox->currentText()).dirPath(true);
	pOptions->sEngineName = EngineNameComboBox->currentText();
	pOptions->saveComboBoxHistory(InstrumentFileComboBox);

	// Just go with dialog acceptance.
	QDialog::accept();
}


// Reject settings (Cancel button slot).
void qsamplerInstrumentForm::reject (void)
{
	bool bReject = true;

	// Check if there's any pending changes...
	if (m_iDirtyCount > 0 && OkPushButton->isEnabled()) {
		switch (QMessageBox::warning(this,
			QSAMPLER_TITLE ": " + tr("Warning"),
			tr("Some channel settings have been changed.\n\n"
			"Do you want to apply the changes?"),
			tr("Apply"), tr("Discard"), tr("Cancel"))) {
		case 0:     // Apply...
			accept();
			return;
		case 1:     // Discard
			break;
		default:    // Cancel.
			bReject = false;
			break;
		}
	}

	if (bReject)
		QDialog::reject();
}


// Dirty up settings.
void qsamplerInstrumentForm::changed (void)
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtyCount++;
	stabilizeForm();
}


// Stabilize current form state.
void qsamplerInstrumentForm::stabilizeForm (void)
{
	bool bValid =!NameLineEdit->text().isEmpty();

	const QString& sPath = InstrumentFileComboBox->currentText();
	bValid = bValid && !sPath.isEmpty() && QFileInfo(sPath).exists();

	OkPushButton->setEnabled(m_iDirtyCount > 0 && bValid);
}


// end of qsamplerInstrumentForm.ui.h
