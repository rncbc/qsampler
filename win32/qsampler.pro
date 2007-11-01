INCPATH  += ../src ../../liblscp

HEADERS  += ../src/qsamplerAbout.h \
			../src/qsamplerOptions.h \
			../src/qsamplerChannel.h \
			../src/qsamplerMessages.h \
			../src/qsamplerInstrument.h \
			../src/qsamplerInstrumentList.h \
			../src/qsamplerDevice.h \
			../src/qsamplerUtilities.h

SOURCES  += ../src/main.cpp \
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

LIBS += ../../liblscp/win32/liblscp.lib

win32 {
	CONFIG  += console
	INCPATH += C:\usr\local\include
	LIBS    += -LC:\usr\local\lib
}

# Qt3 support
QT += qt3support
