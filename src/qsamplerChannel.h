// qsamplerChannel.h
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

#ifndef __qsamplerChannel_h
#define __qsamplerChannel_h

#include <QTableWidgetItem>
#include <QAbstractTableModel>
#include <QMetaType>
#include <QItemDelegate>
#include <QFontMetrics>
#include <QModelIndex>
#include <QSize>

#include <lscp/client.h>
#include <lscp/device.h>

#include "qsamplerOptions.h"

class qsamplerDevice;


// Typedef'd QMap.
typedef QMap<int, int> qsamplerChannelRoutingMap;


//-------------------------------------------------------------------------
// qsamplerChannel - Sampler channel structure.
//

class qsamplerChannel
{
public:

	// Constructor.
	qsamplerChannel(int iChannelID = -1);
	// Default destructor.
	~qsamplerChannel();

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
	const qsamplerChannelRoutingMap& audioRouting() const;

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
	void     appendMessagesColor  (const QString & s, const QString & c) const;
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
	static bool isInstrumentFile (const QString& sInstrumentFile);

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
	qsamplerChannelRoutingMap m_audioRouting;
};


//-------------------------------------------------------------------------
// qsamplerChannelRoutingTable - Channel routing table widget.
//

#if 0
class qsamplerChannelRoutingTable : public QTable
{
	Q_OBJECT

public:

	// Constructor.
	qsamplerChannelRoutingTable(QWidget *pParent = 0, const char *pszName = 0);
	// Default destructor.
	~qsamplerChannelRoutingTable();

	// Common parameter table renderer.
	void refresh(qsamplerDevice *pDevice,
		const qsamplerChannelRoutingMap& routing);

	// Commit any pending editing.
	void flush();
};
#endif

struct ChannelRoutingItem {
    QStringList options;
    int         selection;
};

// so we can use it i.e. through QVariant
Q_DECLARE_METATYPE(ChannelRoutingItem)

class ChannelRoutingModel : public QAbstractTableModel {
        Q_OBJECT
    public:
        ChannelRoutingModel(QObject* parent = 0);

        // overridden methods from subclass(es)
        int rowCount(const QModelIndex &parent) const;
        int columnCount(const QModelIndex &parent) const;
        QVariant data(const QModelIndex &index, int role) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    public slots:
        void refresh(qsamplerDevice *pDevice, const qsamplerChannelRoutingMap& routing);

    private:
        qsamplerDevice* pDevice;
        qsamplerChannelRoutingMap routing;
};

class ChannelRoutingDelegate : public QItemDelegate {
        Q_OBJECT
    public:
        ChannelRoutingDelegate(QObject* parent = 0);

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const;

        void setEditorData(QWidget* editor, const QModelIndex& index) const;
        void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const;

        void updateEditorGeometry(QWidget* editor,
            const QStyleOptionViewItem& option, const QModelIndex& index) const;
};


//-------------------------------------------------------------------------
// qsamplerChannelRoutingComboBox - Custom combo box for routing table.
//

/*
class qsamplerChannelRoutingComboBox : public QTableWidgetItem
{
public:

	// Constructor.
	qsamplerChannelRoutingComboBox(QTableWidget *pTable,
		const QStringList& list, const QPixmap& pixmap);

	// Public accessors.
	void setCurrentItem(int iCurrentItem);
	int currentItem() const;

protected:

	// Virtual implemetations.
	QWidget *createEditor() const;
	void setContentFromEditor(QWidget *pWidget);

private:

	// Initial value holders
	QStringList m_list;
	int m_iCurrentItem;
};
*/

#endif  // __qsamplerChannel_h


// end of qsamplerChannel.h
