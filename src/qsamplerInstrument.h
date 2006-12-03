// qsamplerInstrument.h
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

#ifndef __qsamplerInstrument_h
#define __qsamplerInstrument_h

#include <qstring.h>


//-------------------------------------------------------------------------
// qsamplerInstrument - MIDI instrument map structure.
//

class qsamplerInstrument
{
public:

	// Constructor.
	qsamplerInstrument(int iBank = -1, int iProg = -1);

	// Default destructor.
	~qsamplerInstrument();

	// Instrument accessors.
	void setBank(int iBank);
	int bank() const;

	void setProgram(int iProgram);
	int program() const;

	void setName(const QString& sName);
	const QString& name() const;

	void setEngineName(const QString& sEngineName);
	const QString& engineName() const;

	void setInstrumentFile(const QString& sInstrumentFile);
	const QString& instrumentFile() const;

	const QString& instrumentName() const;

	void setInstrumentNr(int InstrumentNr);
	int instrumentNr() const;

	void setVolume(float fVolume);
	float volume() const;

	void setLoadMode(int iLoadMode);
	int loadMode() const;

	// Sync methods.
	bool get();
	bool map();
	bool unmap();

private:

	// Instance variables.
	int     m_iBank;
	int     m_iProgram;
	QString m_sName;
	QString m_sEngineName;
	QString m_sInstrumentFile;
	QString m_sInstrumentName;
	int     m_iInstrumentNr;
	float   m_fVolume;
	int     m_iLoadMode;

};


#endif  // __qsamplerInstrument_h


// end of qsamplerInstrument.h
