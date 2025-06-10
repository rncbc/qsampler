# project (qsampler)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
  file (REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
endif ()
configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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
  qsamplerPaletteForm.h
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
  qsamplerPaletteForm.cpp
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
  qsamplerPaletteForm.ui
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

if (QT_VERSION VERSION_LESS 5.15.0)
  qt5_add_translation (QM_FILES ${TRANSLATIONS})
else ()
  qt_add_translation (QM_FILES ${TRANSLATIONS})
endif ()

add_custom_target (translations ALL DEPENDS ${QM_FILES})

if (APPLE)
  set (ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/images/${PROJECT_NAME}.icns)
  list (APPEND SOURCES ${ICON_FILE})
  set (MACOSX_BUNDLE_ICON_FILE ${PROJECT_NAME}.icns)
  set_source_files_properties (${ICON_FILE} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
endif ()


add_executable (${PROJECT_NAME}
  ${HEADERS}
  ${SOURCES}
  ${FORMS}
  ${RESOURCES}
)

# Add some debugger flags.
if (CONFIG_DEBUG AND UNIX AND NOT APPLE)
  set (CONFIG_DEBUG_OPTIONS -g -fsanitize=address -fno-omit-frame-pointer)
  target_compile_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
  target_link_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
endif ()

set_target_properties (${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

if (WIN32)
  set_target_properties (${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE true)
endif ()

if (APPLE)
  set_target_properties (${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE true)
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg)

if (CONFIG_XUNIQUE)
  target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Network)
endif ()

if (CONFIG_LIBLSCP)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::LSCP)
endif ()

if (CONFIG_LIBGIG)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::GIG)
endif ()

if (MICROSOFT)
  target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib)
elseif (MINGW)
  target_link_libraries(${PROJECT_NAME} PRIVATE wsock32 ws2_32)
endif()


if (UNIX AND NOT APPLE)
  install (TARGETS ${PROJECT_NAME} RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (FILES ${QM_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations)
  install (FILES images/${PROJECT_NAME}.png
    RENAME org.rncbc.${PROJECT_NAME}.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
  install (FILES images/${PROJECT_NAME}.svg
    RENAME org.rncbc.${PROJECT_NAME}.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
  install (FILES appdata/org.rncbc.${PROJECT_NAME}.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install (FILES appdata/org.rncbc.${PROJECT_NAME}.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
  install (FILES mimetypes/org.rncbc.${PROJECT_NAME}.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
  install (FILES mimetypes/org.rncbc.${PROJECT_NAME}.application-x-${PROJECT_NAME}-session.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/mimetypes)
  install (FILES mimetypes/org.rncbc.${PROJECT_NAME}.application-x-${PROJECT_NAME}-session.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/mimetypes)
  install (FILES man1/${PROJECT_NAME}.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
  install (FILES man1/${PROJECT_NAME}.fr.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/fr/man1 RENAME ${PROJECT_NAME}.1)
  install (FILES palette/KXStudio.conf palette/Wonton\ Soup.conf
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/palette)
endif ()

if (WIN32)
  install (TARGETS ${PROJECT_NAME} RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (FILES ${QM_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations)
endif ()

if (APPLE)
  install (TARGETS ${PROJECT_NAME} BUNDLE
    DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (FILES ${QM_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations)
endif ()

if (WIN32 AND CONFIG_INSTALL_QT)
  get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
  get_filename_component(_qmake_location "${_qmake_executable}" DIRECTORY)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qmake_location}")
  if (WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND "${WINDEPLOYQT_EXECUTABLE}"
              #--verbose=2
              "$<TARGET_FILE:${PROJECT_NAME}>"
      COMMENT "Running windeployqt...")
   endif ()
endif ()

if (APPLE AND CONFIG_INSTALL_QT)
  get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
  get_filename_component(_qmake_location "${_qmake_executable}" DIRECTORY)
  find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qmake_location}")
  if (MACDEPLOYQT_EXECUTABLE)
    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND "${MACDEPLOYQT_EXECUTABLE}"
              "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
              -always-overwrite
              #-verbose=3
      COMMENT "Running macdeployqt...")
   endif ()
endif ()
