// qsamplerInstrument.cpp
//
/****************************************************************************
   Copyright (C) 2004-2007, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qsamplerUtilities.h"
#include "qsamplerAbout.h"
#include "qsamplerInstrument.h"

#include "qsamplerMainForm.h"


//-------------------------------------------------------------------------
// qsamplerInstrument - MIDI instrument map structure.
//

// Constructor.
qsamplerInstrument::qsamplerInstrument ( int iMap, int iBank, int iProg )
{
	m_iMap          = iMap;
	m_iBank         = iBank;
	m_iProg         = iProg;
	m_iInstrumentNr = 0;;
	m_fVolume       = 1.0f;
	m_iLoadMode     = 0;
}

// Default destructor.
qsamplerInstrument::~qsamplerInstrument (void)
{
}


// Instrument accessors.
void qsamplerInstrument::setMap ( int iMap )
{
	m_iMap = iMap;
}

int qsamplerInstrument::map (void) const
{
	return m_iMap;
}


void qsamplerInstrument::setBank ( int iBank )
{
	m_iBank = iBank;
}

int qsamplerInstrument::bank (void) const
{
	return m_iBank;
}


void qsamplerInstrument::setProg ( int iProg )
{
	m_iProg = iProg;
}

int qsamplerInstrument::prog (void) const
{
	return m_iProg;
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
bool qsamplerInstrument::mapInstrument (void)
{
#ifdef CONFIG_MIDI_INSTRUMENT

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	if (m_iMap < 0 || m_iBank < 0 || m_iProg < 0)
		return false;

	lscp_midi_instrument_t instr;

	instr.map  = m_iMap;
	instr.bank = (m_iBank & 0x0fff);
	instr.prog = (m_iProg & 0x7f);

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
			lscpEscapePath(m_sInstrumentFile).latin1(),
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


bool qsamplerInstrument::unmapInstrument (void)
{
#ifdef CONFIG_MIDI_INSTRUMENT

	if (m_iMap < 0 || m_iBank < 0 || m_iProg < 0)
		return false;

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	lscp_midi_instrument_t instr;

	instr.map  = m_iMap;
	instr.bank = (m_iBank & 0x0fff);
	instr.prog = (m_iProg & 0x7f);

	if (::lscp_unmap_midi_instrument(pMainForm->client(), &instr) != LSCP_OK) {
		pMainForm->appendMessagesClient("lscp_unmap_midi_instrument");
		return false;
	}

	return true;

#else

	return false;

#endif
}


bool qsamplerInstrument::getInstrument (void)
{
#ifdef CONFIG_MIDI_INSTRUMENT

	if (m_iMap < 0 || m_iBank < 0 || m_iProg < 0)
		return false;

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return false;
	if (pMainForm->client() == NULL)
		return false;

	lscp_midi_instrument_t instr;

	instr.map  = m_iMap;
	instr.bank = (m_iBank & 0x0fff);
	instr.prog = (m_iProg & 0x7f);

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


// Instrument map name enumerator.
QStringList qsamplerInstrument::getMapNames (void)
{
	QStringList maps;

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return maps;
	if (pMainForm->client() == NULL)
		return maps;

#ifdef CONFIG_MIDI_INSTRUMENT
	int *piMaps = ::lscp_list_midi_instrument_maps(pMainForm->client());
	if (piMaps == NULL) {
		if (::lscp_client_get_errno(pMainForm->client()))
			pMainForm->appendMessagesClient("lscp_list_midi_instruments");
	} else {
		for (int iMap = 0; piMaps[iMap] >= 0; iMap++) {
			const QString& sMapName = getMapName(piMaps[iMap]);
			if (!sMapName.isEmpty())
				maps.append(sMapName);
		}
	}
#endif

	return maps;
}

// Instrument map name enumerator.
QString qsamplerInstrument::getMapName ( int iMidiMap )
{
	QString sMapName;

	qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
	if (pMainForm == NULL)
		return sMapName;
	if (pMainForm->client() == NULL)
		return sMapName;

#ifdef CONFIG_MIDI_INSTRUMENT
	const char *pszMapName
		= ::lscp_get_midi_instrument_map_name(pMainForm->client(), iMidiMap);
	if (pszMapName == NULL) {
		pszMapName = " -";
		if (::lscp_client_get_errno(pMainForm->client()))
			pMainForm->appendMessagesClient("lscp_get_midi_instrument_name");
	}
	sMapName = QString("%1 - %2").arg(iMidiMap).arg(pszMapName);
#endif

	return sMapName;
}


// end of qsamplerInstrument.cpp
