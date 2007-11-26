// qsamplerInstrumentForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2007, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007, Christian Schoenebeck

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

#include "qsamplerAbout.h"
#include "qsamplerInstrumentForm.h"

#include "qsamplerOptions.h"
#include "qsamplerChannel.h"
#include "qsamplerMainForm.h"

#include <QFileDialog>
#include <QMessageBox>

// Needed for lroundf()
#include <math.h>

#ifndef CONFIG_ROUND
static inline long lroundf ( float x )
{
	if (x >= 0.0f)
		return long(x + 0.5f);
	else
		return long(x - 0.5f);
}
#endif


namespace QSampler {

InstrumentForm::InstrumentForm ( QWidget* pParent )
	: QDialog(pParent)
{
	m_ui.setupUi(this);

	// Initialize locals.
	m_pInstrument = NULL;

	m_iDirtySetup = 0;
	m_iDirtyCount = 0;
	m_iDirtyName  = 0;

	// Try to restore normal window positioning.
	adjustSize();


	QObject::connect(m_ui.MapComboBox,
		SIGNAL(activated(int)),
		SLOT(changed()));
	QObject::connect(m_ui.BankSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(changed()));
	QObject::connect(m_ui.ProgSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(changed()));
	QObject::connect(m_ui.NameLineEdit,
		SIGNAL(textChanged(const QString&)),
		SLOT(nameChanged(const QString&)));
	QObject::connect(m_ui.EngineNameComboBox,
		SIGNAL(activated(int)),
		SLOT(changed()));
	QObject::connect(m_ui.InstrumentFileComboBox,
		SIGNAL(activated(const QString&)),
		SLOT(updateInstrumentName()));
	QObject::connect(m_ui.InstrumentFileToolButton,
		SIGNAL(clicked()),
		SLOT(openInstrumentFile()));
	QObject::connect(m_ui.InstrumentNrComboBox,
		SIGNAL(activated(int)),
		SLOT(instrumentNrChanged()));
	QObject::connect(m_ui.VolumeSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(changed()));
	QObject::connect(m_ui.LoadModeComboBox,
		SIGNAL(activated(int)),
		SLOT(changed()));
	QObject::connect(m_ui.OkPushButton,
		SIGNAL(clicked()),
		SLOT(accept()));
	QObject::connect(m_ui.CancelPushButton,
		SIGNAL(clicked()),
		SLOT(reject()));
}


InstrumentForm::~InstrumentForm (void)
{
}


// Channel dialog setup formal initializer.
void InstrumentForm::setup ( qsamplerInstrument *pInstrument )
{
	m_pInstrument = pInstrument;

	m_iDirtySetup = 0;
	m_iDirtyCount = 0;
	m_iDirtyName  = 0;

	if (m_pInstrument == NULL)
		return;

	// Check if we're up and connected.
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// It can be a brand new channel, remember?
	bool bNew = (m_pInstrument->bank() < 0 || m_pInstrument->prog() < 0);
	if (!bNew) {
		m_pInstrument->getInstrument();
		m_iDirtyName++;
	}

	// Avoid nested changes.
	m_iDirtySetup++;

	// Load combo box history...
	pOptions->loadComboBoxHistory(m_ui.InstrumentFileComboBox);

	// Populate maps list.
	m_ui.MapComboBox->clear();
	m_ui.MapComboBox->insertItems(0, qsamplerInstrument::getMapNames());

	// Populate Engines list.
	const char **ppszEngines
		= ::lscp_list_available_engines(pMainForm->client());
	if (ppszEngines) {
		m_ui.EngineNameComboBox->clear();
		for (int iEngine = 0; ppszEngines[iEngine]; iEngine++)
			m_ui.EngineNameComboBox->addItem(ppszEngines[iEngine]);
	}
	else pMainForm->appendMessagesClient("lscp_list_available_engines");

	// Read proper instrument information,
	// and populate the instrument form fields.

	// Instrument map name...
	int iMap = (bNew ? pOptions->iMidiMap : m_pInstrument->map());
	if (iMap < 0)
		iMap = 0;
	const QString& sMapName = qsamplerInstrument::getMapName(iMap);
	if (!sMapName.isEmpty()) {
		m_ui.MapComboBox->setCurrentIndex(
			m_ui.MapComboBox->findText(sMapName,
				Qt::MatchExactly | Qt::MatchCaseSensitive));
	}

	// It might be no maps around...
	bool bMapEnabled = (m_ui.MapComboBox->count() > 0);
	m_ui.MapTextLabel->setEnabled(bMapEnabled);
	m_ui.MapComboBox->setEnabled(bMapEnabled);

	// Instrument bank/program...
	int iBank = (bNew ? pOptions->iMidiBank : m_pInstrument->bank());
	int iProg = (bNew ? pOptions->iMidiProg : m_pInstrument->prog()) + 1;
	if (bNew && iProg > 128) {
		iProg = 1;
		iBank++;
	}
	m_ui.BankSpinBox->setValue(iBank);
	m_ui.ProgSpinBox->setValue(iProg);

	// Instrument name...
	m_ui.NameLineEdit->setText(m_pInstrument->name());

	// Engine name...
	QString sEngineName = m_pInstrument->engineName();
	if (sEngineName.isEmpty() || bNew)
		sEngineName = pOptions->sEngineName;
	if (sEngineName.isEmpty())
		sEngineName = qsamplerChannel::noEngineName();
	if (m_ui.EngineNameComboBox->findText(sEngineName,
			Qt::MatchExactly | Qt::MatchCaseSensitive) < 0) {
		m_ui.EngineNameComboBox->addItem(sEngineName);
	}
	m_ui.EngineNameComboBox->setCurrentIndex(
		m_ui.EngineNameComboBox->findText(sEngineName,
			Qt::MatchExactly | Qt::MatchCaseSensitive));

	// Instrument filename and index...
	QString sInstrumentFile = m_pInstrument->instrumentFile();
	if (sInstrumentFile.isEmpty())
		sInstrumentFile = qsamplerChannel::noInstrumentName();
	m_ui.InstrumentFileComboBox->setEditText(sInstrumentFile);
	m_ui.InstrumentNrComboBox->clear();
	m_ui.InstrumentNrComboBox->insertItems(0,
		qsamplerChannel::getInstrumentList(sInstrumentFile,
		pOptions->bInstrumentNames));
	m_ui.InstrumentNrComboBox->setCurrentIndex(m_pInstrument->instrumentNr());

	// Instrument volume....
	int iVolume = (bNew ? pOptions->iVolume :
		::lroundf(100.0f * m_pInstrument->volume()));
	m_ui.VolumeSpinBox->setValue(iVolume);

	// Instrument load mode...
	int iLoadMode = (bNew ? pOptions->iLoadMode :
		m_pInstrument->loadMode());
	m_ui.LoadModeComboBox->setCurrentIndex(iLoadMode);

	// Done.
	m_iDirtySetup--;
	stabilizeForm();

	// Done.
	m_iDirtySetup--;
	stabilizeForm();
}


// Special case for name change,
void InstrumentForm::nameChanged ( const QString& /* sName */ )
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtyName++;
	changed();
}


// Browse and open an instrument file.
void InstrumentForm::openInstrumentFile (void)
{
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// FIXME: the instrument file filters should be restricted,
	// depending on the current engine.
	QString sInstrumentFile = QFileDialog::getOpenFileName(this,
		QSAMPLER_TITLE ": " + tr("Instrument files"), // Caption.
		pOptions->sInstrumentDir,                 // Start here.
		tr("Instrument files") + " (*.gig *.dls)" // Filter (GIG and DLS files)
	);

	if (sInstrumentFile.isEmpty())
		return;

	m_ui.InstrumentFileComboBox->setEditText(sInstrumentFile);
	updateInstrumentName();
}


// Refresh the actual instrument name.
void InstrumentForm::updateInstrumentName (void)
{
	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	// TODO: this better idea would be to use libgig
	// to retrieve the REAL instrument names.
	m_ui.InstrumentNrComboBox->clear();
	m_ui.InstrumentNrComboBox->insertItems(0,
		qsamplerChannel::getInstrumentList(
			m_ui.InstrumentFileComboBox->currentText(),
			pOptions->bInstrumentNames)
	);

	instrumentNrChanged();
}


// Special case for instrumnet index change,
void InstrumentForm::instrumentNrChanged (void)
{
	if (m_iDirtySetup > 0)
		return;

	if (m_ui.NameLineEdit->text().isEmpty() || m_iDirtyName == 0) {
		m_ui.NameLineEdit->setText(m_ui.InstrumentNrComboBox->currentText());
		m_iDirtyName = 0;
	}

	changed();
}


// Accept settings (OK button slot).
void InstrumentForm::accept (void)
{
	if (m_pInstrument == NULL)
		return;

	MainForm* pMainForm = MainForm::getInstance();
	if (pMainForm == NULL)
		return;
	if (pMainForm->client() == NULL)
		return;

	qsamplerOptions *pOptions = pMainForm->options();
	if (pOptions == NULL)
		return;

	if (m_iDirtyCount > 0) {
		m_pInstrument->setMap(m_ui.MapComboBox->currentIndex());
		m_pInstrument->setBank(m_ui.BankSpinBox->value());
		m_pInstrument->setProg(m_ui.ProgSpinBox->value() - 1);
		m_pInstrument->setName(m_ui.NameLineEdit->text());
		m_pInstrument->setEngineName(m_ui.EngineNameComboBox->currentText());
		m_pInstrument->setInstrumentFile(m_ui.InstrumentFileComboBox->currentText());
		m_pInstrument->setInstrumentNr(m_ui.InstrumentNrComboBox->currentIndex());
		m_pInstrument->setVolume(0.01f * float(m_ui.VolumeSpinBox->value()));
		m_pInstrument->setLoadMode(m_ui.LoadModeComboBox->currentIndex());
	}

	// Save default engine name, instrument directory and history...
	pOptions->sInstrumentDir = QFileInfo(
		m_ui.InstrumentFileComboBox->currentText()).dir().absolutePath();
	pOptions->sEngineName = m_ui.EngineNameComboBox->currentText();
	pOptions->iMidiMap  = m_ui.MapComboBox->currentIndex();
	pOptions->iMidiBank = m_ui.BankSpinBox->value();
	pOptions->iMidiProg = m_ui.ProgSpinBox->value();
	pOptions->iVolume   = m_ui.VolumeSpinBox->value();
	pOptions->iLoadMode = m_ui.LoadModeComboBox->currentIndex();
	pOptions->saveComboBoxHistory(m_ui.InstrumentFileComboBox);

	// Just go with dialog acceptance.
	QDialog::accept();
}


// Reject settings (Cancel button slot).
void InstrumentForm::reject (void)
{
	bool bReject = true;

	// Check if there's any pending changes...
	if (m_iDirtyCount > 0 && m_ui.OkPushButton->isEnabled()) {
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
void InstrumentForm::changed (void)
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtyCount++;
	stabilizeForm();
}


// Stabilize current form state.
void InstrumentForm::stabilizeForm (void)
{
	bool bValid =
		!m_ui.NameLineEdit->text().isEmpty() &&
		m_ui.EngineNameComboBox->currentIndex() >= 0 &&
		m_ui.EngineNameComboBox->currentText() !=
		qsamplerChannel::noEngineName();

	const QString& sPath = m_ui.InstrumentFileComboBox->currentText();
	bValid = bValid && !sPath.isEmpty() && QFileInfo(sPath).exists();

	m_ui.OkPushButton->setEnabled(m_iDirtyCount > 0 && bValid);
}

} // namespace QSampler


// end of qsamplerInstrumentForm.cpp
