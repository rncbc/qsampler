// qsamplerFxSend.cpp
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

#include "qsamplerAbout.h"
#include "qsamplerFxSend.h"
#include "qsamplerUtilities.h"
#include "qsamplerOptions.h"
#include "qsamplerMainForm.h"

namespace QSampler {

// marks FxSend objects which don't exist on sampler side yet
#define NEW_FX_SEND		-1

FxSend::FxSend(int SamplerChannelID, int FxSendID) :
	m_iSamplerChannelID(SamplerChannelID),
	m_iFxSendID(FxSendID), m_bDelete(false), m_bModified(false)
{
	m_MidiCtrl = 91;
	m_Depth    = 0.0f;
}

FxSend::FxSend(int SamplerChannelID) :
	m_iSamplerChannelID(SamplerChannelID),
	m_iFxSendID(NEW_FX_SEND), m_bDelete(false), m_bModified(true)
{
	m_MidiCtrl = 91;
	m_Depth    = 0.0f;
}

FxSend::~FxSend() {
}

int FxSend::id() const {
	return m_iFxSendID;
}

bool FxSend::isNew() const {
	return m_iFxSendID == NEW_FX_SEND;
}

void FxSend::setDeletion(bool bDelete) {
	m_bDelete = bDelete;
	m_bModified = true;
}

bool FxSend::deletion() const {
	return m_bDelete;
}

void FxSend::setName(const QString& sName) {
	m_FxSendName = sName;
	m_bModified = true;
}

bool FxSend::isModified() const {
	return m_bModified;
}

const QString& FxSend::name() const {
	return m_FxSendName;
}

void FxSend::setSendDepthMidiCtrl(int iMidiController) {
	m_MidiCtrl = iMidiController;
	m_bModified = true;
}

int FxSend::sendDepthMidiCtrl() const {
	return m_MidiCtrl;
}

void FxSend::setCurrentDepth(float depth) {
	m_Depth = depth;
	m_bModified = true;
}

float FxSend::currentDepth() const {
	return m_Depth;
}

int FxSend::audioChannel(int iAudioSrc) const {
	if (iAudioSrc < 0 || iAudioSrc >= m_AudioRouting.size())
		return -1;

	return m_AudioRouting[iAudioSrc];
}

bool FxSend::setAudioChannel(int iAudioSrc, int iAudioDst) {
	if (iAudioSrc < 0 || iAudioSrc >= m_AudioRouting.size())
		return false;

	m_AudioRouting[iAudioSrc] = iAudioDst;
	m_bModified = true;

	return true;
}

const FxSendRoutingMap& FxSend::audioRouting() const {
	return m_AudioRouting;
}

bool FxSend::getFromSampler() {
#if CONFIG_FXSEND
	m_bModified = false;

	// in case this is a new, actually not yet existing FX send, ignore update
	if (isNew())
		return true;

	MainForm *pMainForm = MainForm::getInstance();
	if (!pMainForm || !pMainForm->client())
		return false;

	lscp_fxsend_info_t* pFxSendInfo =
		::lscp_get_fxsend_info(
			pMainForm->client(),
			m_iSamplerChannelID,
			m_iFxSendID);

	if (!pFxSendInfo) {
		pMainForm->appendMessagesClient("lscp_get_fxsend_info");
		return false;
	}

	m_FxSendName = qsamplerUtilities::lscpEscapedTextToRaw(pFxSendInfo->name);
	m_MidiCtrl   = pFxSendInfo->midi_controller;
	m_Depth      = pFxSendInfo->level;

	m_AudioRouting.clear();
	if (pFxSendInfo->audio_routing)
		for (int i = 0; pFxSendInfo->audio_routing[i]; ++i)
			m_AudioRouting[i] = pFxSendInfo->audio_routing[i];

	return true;
#else // CONFIG_FXSEND
	return false;
#endif // CONFIG_FXSEND
}

bool FxSend::applyToSampler() {
#if CONFIG_FXSEND
	MainForm *pMainForm = MainForm::getInstance();
	if (!pMainForm || !pMainForm->client())
		return false;

	// in case FX send doesn't exist on sampler side yet, create it
	if (isNew()) {
		// doesn't exist and scheduled for deletion? nothing to do
		if (deletion()) {
			m_bModified = false;
			return true;
		}

		int result =
			::lscp_create_fxsend(
				pMainForm->client(),
				m_iSamplerChannelID,
				m_MidiCtrl, NULL
			);
		if (result == -1) {
			pMainForm->appendMessagesClient("lscp_create_fxsend");
			return false;
		}
		m_iFxSendID = result;
	}

	lscp_status_t result;

	// delete FX send on sampler side
	if (deletion()) {
		result =
			::lscp_destroy_fxsend(
				pMainForm->client(), m_iSamplerChannelID, m_iFxSendID
			);
		if (result != LSCP_OK) {
			pMainForm->appendMessagesClient("lscp_destroy_fxsend");
			return false;
		}
		m_bModified = false;
		return true;
	}

	// set FX send depth MIDI controller
	result =
		::lscp_set_fxsend_midi_controller(
			pMainForm->client(),
			m_iSamplerChannelID, m_iFxSendID, m_MidiCtrl
		);
	if (result != LSCP_OK) {
		pMainForm->appendMessagesClient("lscp_set_fxsend_midi_controller");
		return false;
	}

#if CONFIG_FXSEND_RENAME
	// set FX send's name
	result =
		::lscp_set_fxsend_name(
			pMainForm->client(),
			m_iSamplerChannelID, m_iFxSendID,
			qsamplerUtilities::lscpEscapeText(
				m_FxSendName
			).toUtf8().constData()
		);
	if (result != LSCP_OK) {
		pMainForm->appendMessagesClient("lscp_set_fxsend_name");
		return false;
	}
#endif // CONFIG_FXSEND_RENAME

	// set FX send current send level
	result =
		::lscp_set_fxsend_level(
			pMainForm->client(),
			m_iSamplerChannelID, m_iFxSendID, m_Depth
		);
	if (result != LSCP_OK) {
		pMainForm->appendMessagesClient("lscp_set_fxsend_level");
		return false;
	}

	// set FX send's audio routing
	for (int i = 0; i < m_AudioRouting.size(); ++i) {
		result =
			::lscp_set_fxsend_audio_channel(
				pMainForm->client(), m_iSamplerChannelID, m_iFxSendID,
				i, /*audio source*/
				m_AudioRouting[i] /*audio destination*/
			);
		if (result != LSCP_OK) {
			pMainForm->appendMessagesClient("lscp_set_fxsend_audio_channel");
			return false;
		}
	}

	m_bModified = false;
	return true;
#else // CONFIG_FXSEND
	return false;
#endif // CONFIG_FXSEND
}

QList<int> FxSend::allFxSendsOfSamplerChannel(int samplerChannelID) {
	QList<int> sends;

	MainForm *pMainForm = MainForm::getInstance();
	if (!pMainForm || !pMainForm->client())
		return sends;

#ifdef CONFIG_FXSEND
	int *piSends = ::lscp_list_fxsends(pMainForm->client(), samplerChannelID);
	if (!piSends) {
		if (::lscp_client_get_errno(pMainForm->client()))
			pMainForm->appendMessagesClient("lscp_list_fxsends");
	} else {
		for (int iSend = 0; piSends[iSend] >= 0; ++iSend)
			sends.append(piSends[iSend]);
	}
#endif // CONFIG_FXSEND

	return sends;
}

} // namespace QSampler

// end of qsamplerFxSend.cpp
