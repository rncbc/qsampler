INCPATH  += ../src

HEADERS   = ../src/qsamplerAbout.h \
			../src/qsamplerOptions.h \
			../src/qsamplerChannel.h \
			../src/qsamplerMessages.h \
			../src/qsamplerInstrument.h \
			../src/qsamplerInstrumentList.h \
			../src/qsamplerDevice.h \
			../src/qsamplerUtilities.h \
			../src/qsamplerInstrumentForm.h \
			../src/qsamplerInstrumentListForm.h \
			../src/qsamplerDeviceForm.h \
			../src/qsamplerChannelStrip.h \
			../src/qsamplerChannelForm.h \
			../src/qsamplerOptionsForm.h \
			../src/qsamplerMainForm.h

SOURCES   = ../src/main.cpp \
			../src/qsamplerOptions.cpp \
			../src/qsamplerChannel.cpp \
			../src/qsamplerMessages.cpp \
			../src/qsamplerInstrument.cpp \
			../src/qsamplerInstrumentList.cpp \
			../src/qsamplerDevice.cpp \
			../src/qsamplerUtilities.cpp \
			../src/qsamplerInstrumentForm.cpp \
			../src/qsamplerInstrumentListForm.cpp \
			../src/qsamplerDeviceForm.cpp \
			../src/qsamplerChannelStrip.cpp \
			../src/qsamplerChannelForm.cpp \
			../src/qsamplerOptionsForm.cpp \
			../src/qsamplerMainForm.cpp

FORMS     = ../src/qsamplerInstrumentForm.ui \
			../src/qsamplerInstrumentListForm.ui \
			../src/qsamplerDeviceForm.ui \
			../src/qsamplerChannelStrip.ui \
			../src/qsamplerChannelForm.ui \
			../src/qsamplerOptionsForm.ui \
			../src/qsamplerMainForm.ui

RESOURCES = ../icons/qsampler.qrc

TEMPLATE  = app
CONFIG   += qt thread warn_on release
LANGUAGE  = C++

LIBS     += -llscp

win32 {
	CONFIG  += console
	INCPATH += C:\usr\local\include
	LIBS    += -LC:\usr\local\lib -lws2_32
}
