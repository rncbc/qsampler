// qsamplerChannel.h
//
/****************************************************************************
   Copyright (C) 2004-2023, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007, Christian Schoenebeck

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

#ifndef __qsamplerChannel_h
#define __qsamplerChannel_h

#include <QTableWidgetItem>
#include <QItemDelegate>

#include <lscp/client.h>
#include <lscp/device.h>

#include "qsamplerOptions.h"

namespace QSampler {

class Device;

// Typedef'd QMap.
typedef QMap<int, int> ChannelRoutingMap;


//-------------------------------------------------------------------------
// QSampler::Channel - Sampler channel structure.
//

class Channel
{
public:

	// Constructor.
	Channel(int iChannelID = -1);
	// Default destructor.
	~Channel();

	// Add/remove sampler channel methods.
	bool     addChannel();
	bool     removeChannel();

	// Sampler channel ID accessors.
	int      channelID() const;
	void     setChannelID(int iChannelID);

	// Readable channel name.
	QString  channelName() const;

	// Engine name property.
	const QString& engineName() const;
	bool     loadEngine(const QString& sEngineName);

	// Instrument file and index.
	const QString& instrumentFile() const;
	int      instrumentNr() const;
	const QString& instrumentName() const;
	int      instrumentStatus() const;

	// Instrument file loader.
	bool     loadInstrument(const QString& sInstrumentFile, int iInstrumentNr);
	// Special instrument file/name/number settler.
	bool     setInstrument(const QString& sInstrumentFile, int iInstrumentNr);

	// MIDI input driver (DEPRECATED).
	const QString& midiDriver() const;
	bool     setMidiDriver(const QString& sMidiDriver);

	// MIDI input device.
	int      midiDevice() const;
	bool     setMidiDevice(int iMidiDevice);

	// MIDI input port.
	int      midiPort() const;
	bool     setMidiPort(int iMidiPort);

	// MIDI input channel.
	int      midiChannel() const;
	bool     setMidiChannel(int iMidiChannel);

	// MIDI instrument map.
	int      midiMap() const;
	bool     setMidiMap(int iMidiMap);

	// Audio output driver (DEPRECATED).
	const QString& audioDriver() const;
	bool     setAudioDriver(const QString& sAudioDriver);

	// Audio output device.
	int      audioDevice() const;
	bool     setAudioDevice(int iAudioDevice);

	// Sampler channel volume.
	float    volume() const;
	bool     setVolume(float fVolume);

	// Sampler channel mute state.
	bool     channelMute() const;
	bool     setChannelMute(bool bMute);

	// Sampler channel solo state.
	bool     channelSolo() const;
	bool     setChannelSolo(bool bSolo);

	// Audio routing accessors.
	int      audioChannel(int iAudioOut) const;
	bool     setAudioChannel(int iAudioOut, int iAudioIn);
	// The audio routing map itself.
	const ChannelRoutingMap& audioRouting() const;

	// Istrument name remapper.
	void     updateInstrumentName();

	// Channel info structure map executive.
	bool     updateChannelInfo();

	// Channel setup dialog form.
	bool     channelSetup(QWidget *pParent);

	// Reset channel method.
	bool     channelReset();

	// Spawn instrument editor method.
	bool     editChannel();

	// Message logging methods (brainlessly mapped to main form's).
	void     appendMessages       (const QString & s) const;
	void     appendMessagesColor  (const QString & s, const QColor& rgb) const;
	void     appendMessagesText   (const QString & s) const;
	void     appendMessagesError  (const QString & s) const;
	void     appendMessagesClient (const QString & s) const;

	// Context menu event handler.
	void contextMenuEvent(QContextMenuEvent *pEvent);

	// Common (invalid) name-helpers.
	static QString noEngineName();
	static QString noInstrumentName();
	static QString loadingInstrument();

	// Check whether a given file is an instrument file.
	static bool isDlsInstrumentFile (const QString& sInstrumentFile);
	static bool isSf2InstrumentFile (const QString& sInstrumentFile);

	// Retrieve the available instrument name(s) of an instrument file (.gig).
	static QString getInstrumentName (const QString& sInstrumentFile,
							int iInstrumentNr, bool bInstrumentNames);
	static QStringList getInstrumentList (const QString& sInstrumentFile,
							bool bInstrumentNames);

private:

	// Unique channel identifier.
	int     m_iChannelID;

	// Sampler channel info map.
	QString m_sEngineName;
	QString m_sInstrumentName;
	QString m_sInstrumentFile;
	int     m_iInstrumentNr;
	int     m_iInstrumentStatus;
	QString m_sMidiDriver;
	int     m_iMidiDevice;
	int     m_iMidiPort;
	int     m_iMidiChannel;
	int     m_iMidiMap;
	QString m_sAudioDriver;
	int     m_iAudioDevice;
	float   m_fVolume;
	bool    m_bMute;
	bool    m_bSolo;

	// The audio routing mapping.
	ChannelRoutingMap m_audioRouting;
};


//-------------------------------------------------------------------------
// QSampler::ChannelRoutingModel - data model for audio routing
//                                 (used for QTableView)
//

struct ChannelRoutingItem {
	QStringList options;
	int         selection;
};

class ChannelRoutingModel : public QAbstractTableModel
{
	Q_OBJECT
public:

	ChannelRoutingModel(QObject* pParent = nullptr);

	// overridden methods from subclass(es)
	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex()) const;
	Qt::ItemFlags flags(const QModelIndex& index) const;
	bool setData(const QModelIndex& index, const QVariant& value,
		int role = Qt::EditRole);
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const;

	// own methods
	ChannelRoutingMap routingMap() const { return m_routing; }

	void clear() { m_routing.clear(); }

public slots:

	void refresh(Device *pDevice,
		const ChannelRoutingMap& routing);

private:

	Device *m_pDevice;
	ChannelRoutingMap m_routing;
};


//-------------------------------------------------------------------------
// QSampler::ChannelRoutingDelegate - table cell renderer for audio routing
//

class ChannelRoutingDelegate : public QItemDelegate
{
	Q_OBJECT

public:

	ChannelRoutingDelegate(QObject* pParent = nullptr);

	QWidget* createEditor(QWidget *pParent,
		const QStyleOptionViewItem& option, const QModelIndex& index) const;
	void setEditorData(QWidget *pEditor, const QModelIndex& index) const;
	void setModelData(QWidget *pEditor, QAbstractItemModel* model,
		const QModelIndex& index) const;
	void updateEditorGeometry(QWidget *pEditor,
		const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

} // namespace QSampler

// So we can use it i.e. through QVariant
Q_DECLARE_METATYPE(QSampler::ChannelRoutingItem)

#endif  // __qsamplerChannel_h


// end of qsamplerChannel.h
