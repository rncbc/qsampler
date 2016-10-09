# qsampler.pro
#
NAME = qsampler

TARGET = $${NAME}
TEMPLATE = app

include(src.pri)

#DEFINES += DEBUG

HEADERS += config.h \
	qsamplerAbout.h \
	qsamplerOptions.h \
	qsamplerChannel.h \
	qsamplerMessages.h \
	qsamplerInstrument.h \
	qsamplerInstrumentList.h \
	qsamplerDevice.h \
	qsamplerFxSend.h \
	qsamplerFxSendsModel.h \
	qsamplerUtilities.h \
	qsamplerInstrumentForm.h \
	qsamplerInstrumentListForm.h \
	qsamplerDeviceForm.h \
	qsamplerDeviceStatusForm.h \
	qsamplerChannelStrip.h \
	qsamplerChannelForm.h \
	qsamplerChannelFxForm.h \
	qsamplerOptionsForm.h \
	qsamplerMainForm.h

SOURCES += \
	qsampler.cpp \
	qsamplerOptions.cpp \
	qsamplerChannel.cpp \
	qsamplerMessages.cpp \
	qsamplerInstrument.cpp \
	qsamplerInstrumentList.cpp \
	qsamplerDevice.cpp \
	qsamplerFxSend.cpp \
	qsamplerFxSendsModel.cpp \
	qsamplerUtilities.cpp \
	qsamplerInstrumentForm.cpp \
	qsamplerInstrumentListForm.cpp \
	qsamplerDeviceForm.cpp \
	qsamplerDeviceStatusForm.cpp \
	qsamplerChannelStrip.cpp \
	qsamplerChannelForm.cpp \
	qsamplerChannelFxForm.cpp \
	qsamplerOptionsForm.cpp \
	qsamplerMainForm.cpp

FORMS += \
	qsamplerInstrumentForm.ui \
	qsamplerInstrumentListForm.ui \
	qsamplerDeviceForm.ui \
	qsamplerChannelStrip.ui \
	qsamplerChannelForm.ui \
	qsamplerChannelFxForm.ui \
	qsamplerOptionsForm.ui \
	qsamplerMainForm.ui

RESOURCES += \
	qsampler.qrc


TRANSLATIONS += \
	translations/qsampler_cs.ts \
	translations/qsampler_ru.ts


unix {

	# variables
	OBJECTS_DIR = .obj
	MOC_DIR     = .moc
	UI_DIR      = .ui

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	isEmpty(BINDIR) {
		BINDIR = $${PREFIX}/bin
	}

	isEmpty(DATADIR) {
		DATADIR = $${PREFIX}/share
	}

	#DEFINES += DATADIR=\"$${DATADIR}\"

	# make install
	INSTALLS += target desktop icon appdata \
		icon_scalable mimeinfo mimetypes mimetypes_scalable

	target.path = $${BINDIR}

	desktop.path = $${DATADIR}/applications
	desktop.files += $${NAME}.desktop

	icon.path = $${DATADIR}/icons/hicolor/32x32/apps
	icon.files += images/$${NAME}.png

	icon_scalable.path = $${DATADIR}/icons/hicolor/scalable/apps
	icon_scalable.files += images/$${NAME}.svg

	appdata.path = $${DATADIR}/appdata
	appdata.files += appdata/$${NAME}.appdata.xml

	mimeinfo.path = $${DATADIR}/mime/packages
	mimeinfo.files += mimetypes/$${NAME}.xml

	mimetypes.path = $${DATADIR}/icons/hicolor/32x32/mimetypes
	mimetypes.files += mimetypes/application-x-$${NAME}-session.png

	mimetypes_scalable.path = $${DATADIR}/icons/hicolor/scalable/mimetypes
	mimetypes_scalable.files += mimetypes/application-x-$${NAME}-session.svg
}

win32 {

	CONFIG(debug, debug|release): CONFIG += console
	INSTALLS += target
	target.path = $${PREFIX}/bin
}

macx {

	QMAKE_MAC_SDK = $$(SDKROOT)
	CONFIG += $$(QMAKE_ARCHS)
}


# QT5 support
!lessThan(QT_MAJOR_VERSION, 5) {
	QT += widgets
}
