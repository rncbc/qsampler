INCLUDEPATH += ../src ../../liblscp

SOURCES += ../src/main.cpp \
           ../src/qsamplerOptions.cpp \
           ../src/qsamplerChannel.cpp \
           ../src/qsamplerMessages.cpp \
	       ../src/qsamplerDevice.cpp

HEADERS += ../src/qsamplerAbout.h \
           ../src/qsamplerOptions.h \
           ../src/qsamplerChannel.h \
           ../src/qsamplerMessages.h \
	       ../src/qsamplerDevice.h

FORMS    = ../src/qsamplerMainForm.ui \
           ../src/qsamplerChannelStrip.ui \
           ../src/qsamplerChannelForm.ui \
           ../src/qsamplerOptionsForm.ui \
	       ../src/qsamplerDeviceForm.ui

IMAGES   = ../icons/qsampler.png \
           ../icons/qsamplerChannel.png \
           ../icons/qsamplerDevice.png \
           ../icons/fileNew.png \
           ../icons/fileOpen.png \
           ../icons/fileSave.png \
           ../icons/fileRestart.png \
           ../icons/fileReset.png \
           ../icons/editAddChannel.png \
           ../icons/editRemoveChannel.png \
           ../icons/editSetupChannel.png \
           ../icons/editResetChannel.png \
           ../icons/editResetAllChannels.png \
           ../icons/channelsArrange.png \
           ../icons/formAccept.png \
           ../icons/formReject.png \
           ../icons/formRefresh.png \
           ../icons/deviceCreate.png \
           ../icons/deviceDelete.png \
           ../icons/displaybg1.png \
           ../icons/midi1.png \
           ../icons/midi2.png \
           ../icons/audio1.png \
           ../icons/audio2.png

TEMPLATE = app
CONFIG  += qt warn_on debug
LANGUAGE = C++

LIBS    += ../../liblscp/win32/liblscp.lib

