// qsamplerInstrument.cpp
//
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

#include "qsamplerMainForm.h"


//-------------------------------------------------------------------------
// qsamplerInstrument - MIDI instrument map structure.
//

// Constructor.
qsamplerInstrument::qsamplerInstrument ( int iBank, int iProgram )
{
	m_iBank         = iBank;
	m_iProgram      = iProgram;
	m_iInstrumentNr = 0;;
	m_fVolume       = 1.0f;
	m_iLoadMode     = 0;
}

// Default destructor.
qsamplerInstrument::~qsamplerInstrument (void)
{
}


// Instrument accessors.
void qsamplerInstrument::setBank ( int iBank )
{
	m_iBank = iBank;
}

int qsamplerInstrument::bank (void) const
{
	return m_iBank;
}


void qsamplerInstrument::setProgram ( int iProgram )
{
	m_iProgram = iProgram;
}

int qsamplerInstrument::program (void) const
{
	return m_iProgram;
}


void qsamplerInstrument::setName ( const QString& sName )
{
	m_sName = sName;
}

const QString& qsamplerInstrument::name (void) const
{
	return m_sName;
}


void qsamplerInstrument::setEngineName ( const QString& sEngineName )
{
	m_sEngineName = sEngineName;
}

const QString& qsamplerInstrument::engineName (void) const
{
	return m_sEngineName;
}


void qsamplerInstrument::setInstrumentFile ( const QString& sInstrumentFile )
{
	m_sInstrumentFile = sInstrumentFile;
}

const QString& qsamplerInstrument::instrumentFile (void) const
{
	return m_sInstrumentFile;
}


const QString& qsamplerInstrument::instrumentName (void) const
{
	return m_sInstrumentName;
}


void qsamplerInstrument::setInstrumentNr ( int iInstrumentNr )
{
	m_iInstrumentNr = iInstrumentNr;
}

int qsamplerInstrument::instrumentNr (void) const
{
	return m_iInstrumentNr;
}


void qsamplerInstrument::setVolume ( float fVolume )
{
	m_fVolume = fVolume;
}

float qsamplerInstrument::volume (void) const
{
	return m_fVolume;
}


void qsamplerInstrument::setLoadMode ( int iLoadMode )
{
	m_iLoadMode = iLoadMode;
}

int qsamplerInstrument::loadMode (void) const
{
	return m_iLoadMode;
}


// Sync methods.
bool qsamplerInstrument::map (void)
{
#ifdef CONFIG_MIDI_INSTRUMENT

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	if (m_iBank < 0 || m_iProgram < 0)
		return false;

	lscp_midi_instrument_t instr;

	instr.bank_msb = (m_iBank & 0x3f80) >> 7;
	instr.bank_lsb = (m_iBank & 0x7f);
	instr.program  = (m_iProgram & 0x7f);

	lscp_load_mode_t load_mode;
	switch (m_iLoadMode) {
		case 3:
			load_mode = LSCP_LOAD_PERSISTENT;
			break;
		case 2:
			load_mode = LSCP_LOAD_ON_DEMAND_HOLD;
			break;
		case 1:
			load_mode = LSCP_LOAD_ON_DEMAND;
			break;
		case 0:
		default:
			load_mode = LSCP_LOAD_DEFAULT;
			break;
	}

	if (::lscp_map_midi_instrument(pMainForm->client(), &instr,
		m_sEngineName.latin1(),
		m_sInstrumentFile.latin1(),
		m_iInstrumentNr,
		m_fVolume,
		load_mode,
		m_sName.latin1()) != LSCP_OK) {
		pMainForm->appendMessagesClient("lscp_map_midi_instrument");
		return false;
	}

	return true;

#else

	return false;

#endif
}


bool qsamplerInstrument::unmap (void)
{
#ifdef CONFIG_MIDI_INSTRUMENT

	if (m_iBank < 0 || m_iProgram < 0)
		return false;

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	lscp_midi_instrument_t instr;

	instr.bank_msb = (m_iBank & 0x3f80) >> 7;
	instr.bank_lsb = (m_iBank & 0x7f);
	instr.program  = (m_iProgram & 0x7f);

	if (::lscp_unmap_midi_instrument(pMainForm->client(), &instr) != LSCP_OK) {
		pMainForm->appendMessagesClient("lscp_unmap_midi_instrument");
		return false;
	}

	return true;

#else

	return false;

#endif
}


bool qsamplerInstrument::get (void)
{
#ifdef CONFIG_MIDI_INSTRUMENT

	if (m_iBank < 0 || m_iProgram < 0)
		return false;

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	lscp_midi_instrument_t instr;

	instr.bank_msb = (m_iBank & 0x3f80) >> 7;
	instr.bank_lsb = (m_iBank & 0x7f);
	instr.program  = (m_iProgram & 0x7f);

	lscp_midi_instrument_info_t *pInstrInfo
		= ::lscp_get_midi_instrument_info(pMainForm->client(), &instr);
	if (pInstrInfo == NULL) {
		pMainForm->appendMessagesClient("lscp_get_midi_instrument_info");
		return false;
	}

	m_sName           = pInstrInfo->name;
	m_sEngineName     = pInstrInfo->engine_name;
	m_sInstrumentName = pInstrInfo->instrument_name;
	m_sInstrumentFile = pInstrInfo->instrument_file;
	m_iInstrumentNr   = pInstrInfo->instrument_nr;
	m_fVolume         = pInstrInfo->volume;

	switch (pInstrInfo->load_mode) {
		case LSCP_LOAD_PERSISTENT:
			m_iLoadMode = 3;
			break;
		case LSCP_LOAD_ON_DEMAND_HOLD:
			m_iLoadMode = 2;
			break;
		case LSCP_LOAD_ON_DEMAND:
			m_iLoadMode = 1;
			break;
		case LSCP_LOAD_DEFAULT:
		default:
			m_iLoadMode = 0;
			break;
	}

	// Fix something.
	if (m_sName.isEmpty())
		m_sName = m_sInstrumentName;

	return true;

#else

	return false;

#endif
}


// end of qsamplerInstrument.cpp
