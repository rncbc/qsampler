INCPATH  += ../src ../../liblscp

HEADERS  += config.h \
			../src/qsamplerChannelForm.h \
			../src/qsamplerChannelStrip.h \
			../src/qsamplerDeviceForm.h \
			../src/qsamplerInstrumentForm.h \
			../src/qsamplerInstrumentListForm.h \
			../src/qsamplerMainForm.h \
			../src/qsamplerOptionsForm.h \
			../src/qsamplerAbout.h \
			../src/qsamplerOptions.h \
			../src/qsamplerChannel.h \
			../src/qsamplerMessages.h \
			../src/qsamplerInstrument.h \
			../src/qsamplerInstrumentList.h \
			../src/qsamplerDevice.h \
			../src/qsamplerUtilities.h

SOURCES  += ../src/main.cpp \
			../src/qsamplerChannelForm.cpp \
			../src/qsamplerChannelStrip.cpp \
			../src/qsamplerDeviceForm.cpp \
			../src/qsamplerInstrumentForm.cpp \
			../src/qsamplerInstrumentListForm.cpp \
			../src/qsamplerMainForm.cpp \
			../src/qsamplerOptionsForm.cpp \
			../src/qsamplerOptions.cpp \
			../src/qsamplerChannel.cpp \
			../src/qsamplerMessages.cpp \
			../src/qsamplerInstrument.cpp \
			../src/qsamplerInstrumentList.cpp \
			../src/qsamplerDevice.cpp \
			../src/qsamplerUtilities.cpp

FORMS     = ../src/qsamplerMainForm.ui \
			../src/qsamplerChannelStrip.ui \
			../src/qsamplerChannelForm.ui \
			../src/qsamplerOptionsForm.ui \
			../src/qsamplerInstrumentForm.ui \
			../src/qsamplerInstrumentListForm.ui \
			../src/qsamplerDeviceForm.ui

RESOURCES = ../icons/qsampler.qrc

TEMPLATE  = app
CONFIG   += qt thread warn_on release
LANGUAGE  = C++

win32 {
	CONFIG  += console
	INCPATH += C:\usr\local\include
	LIBS    += -LC:\usr\local\lib
	LIBS += c:/msys/1.0/local/lib/liblscp.a
        LIBS    += -lws2_32
}

# Qt3 support
QT += qt3support
