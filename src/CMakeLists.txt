
include_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${QT_INCLUDES}
)

link_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT_LIBRARY_DIR}
)

if (CONFIG_LIBLSCP)
  include_directories (${LSCP_INCLUDE_DIRS})
  link_directories (${LSCP_LIBRARY_DIRS})
endif ()

if (CONFIG_LIBGIG)
  include_directories (${GIG_INCLUDE_DIRS})
  link_directories (${GIG_LIBRARY_DIRS})
endif ()

configure_file (cmake_config.h ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set (ac_prefix ${CMAKE_INSTALL_PREFIX})

configure_file (qsampler.desktop ${CMAKE_CURRENT_BINARY_DIR}/qsampler.desktop)

set (HEADERS
  qsampler.h
  qsamplerAbout.h
  qsamplerOptions.h
  qsamplerChannel.h
  qsamplerMessages.h
  qsamplerInstrument.h
  qsamplerInstrumentList.h
  qsamplerDevice.h
  qsamplerFxSend.h
  qsamplerFxSendsModel.h
  qsamplerUtilities.h
  qsamplerInstrumentForm.h
  qsamplerInstrumentListForm.h
  qsamplerDeviceForm.h
  qsamplerDeviceStatusForm.h
  qsamplerChannelStrip.h
  qsamplerChannelForm.h
  qsamplerChannelFxForm.h
  qsamplerOptionsForm.h
  qsamplerMainForm.h
)

set (SOURCES
  qsampler.cpp
  qsamplerOptions.cpp
  qsamplerChannel.cpp
  qsamplerMessages.cpp
  qsamplerInstrument.cpp
  qsamplerInstrumentList.cpp
  qsamplerDevice.cpp
  qsamplerFxSend.cpp
  qsamplerFxSendsModel.cpp
  qsamplerUtilities.cpp
  qsamplerInstrumentForm.cpp
  qsamplerInstrumentListForm.cpp
  qsamplerDeviceForm.cpp
  qsamplerDeviceStatusForm.cpp
  qsamplerChannelStrip.cpp
  qsamplerChannelForm.cpp
  qsamplerChannelFxForm.cpp
  qsamplerOptionsForm.cpp
  qsamplerMainForm.cpp
)

set (FORMS
  qsamplerInstrumentForm.ui
  qsamplerInstrumentListForm.ui
  qsamplerDeviceForm.ui
  qsamplerChannelStrip.ui
  qsamplerChannelForm.ui
  qsamplerChannelFxForm.ui
  qsamplerOptionsForm.ui
  qsamplerMainForm.ui
)

set (RESOURCES
  qsampler.qrc
)

set (TRANSLATIONS
  translations/qsampler_cs.ts
  translations/qsampler_fr.ts
  translations/qsampler_ru.ts
)

qt5_add_translation ( QM_FILES ${TRANSLATIONS} )
add_custom_target( translations ALL DEPENDS ${QM_FILES} )

qt5_wrap_ui (UI_SOURCES ${FORMS})
qt5_wrap_cpp (MOC_SOURCES ${HEADERS})
qt5_add_resources (QRC_SOURCES ${RESOURCES})

add_executable (qsampler
  ${UI_SOURCES}
  ${MOC_SOURCES}
  ${QRC_SOURCES}
  ${SOURCES}
)

target_link_libraries (qsampler
  ${QT_LIBRARIES}
  ${LSCP_LIBRARIES}
)

if (CONFIG_LIBGIG)
  target_link_libraries (qsampler ${GIG_LIBRARIES})
endif ()

qt5_use_modules (qsampler Core Gui Widgets Network)

if (UNIX AND NOT APPLE)
  install (TARGETS qsampler
     RUNTIME DESTINATION bin)
  install (FILES ${QM_FILES}
     DESTINATION share/qsampler/translations)
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/qsampler.desktop
     DESTINATION share/applications)
  install (FILES images/qsampler.png
     DESTINATION share/pixmaps)
  install (FILES appdata/qsampler.appdata.xml
     DESTINATION share/metainfo)
endif ()
