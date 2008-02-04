// qsamplerFxSend.h
//
/****************************************************************************
   Copyright (C) 2008, Christian Schoenebeck

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

#ifndef __qsamplerFxSend_h
#define __qsamplerFxSend_h

#include <QStringList>
#include <QMap>
#include <QList>

namespace QSampler {

// Typedef'd QMap.
typedef QMap<int, int> FxSendRoutingMap;

class FxSend {
public:
	// retrieve existing FX send
	FxSend(int SamplerChannelID, int FxSendID);

	// create a new FX send
	FxSend(int SamplerChannelID);

	~FxSend();

	int id() const;

	// whether FX send exists on sampler side yet
	bool isNew() const;

	// whether scheduled for deletion
	bool deletion() const;
	void setDeletion(bool bDelete);

	bool isModified() const;

	void setName(const QString& sName);
	const QString& name() const;

	void setSendDepthMidiCtrl(int iMidiController);
	int sendDepthMidiCtrl() const;

	void setCurrentDepth(float depth);
	float currentDepth() const;

	// Audio routing accessors.
	int  audioChannel(int iAudioSrc) const;
	bool setAudioChannel(int iAudioSrc, int iAudioDst);
	// The audio routing map itself.
	const FxSendRoutingMap& audioRouting() const;

	bool getFromSampler();
	bool applyToSampler();

	static QList<int> allFxSendsOfSamplerChannel(int samplerChannelID);

private:
	int m_iSamplerChannelID;
	int m_iFxSendID;
	bool m_bDelete;
	bool m_bModified;

	QString m_FxSendName;
	int m_MidiCtrl;
	float m_Depth;
	FxSendRoutingMap m_AudioRouting;
};

} // namespace QSampler

#endif  // __qsamplerFxSend_h

// end of __qsamplerFxSend.h
