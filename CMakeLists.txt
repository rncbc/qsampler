cmake_minimum_required (VERSION 3.13)

project (qsampler
  VERSION 0.9.7
  DESCRIPTION "A LinuxSampler Qt GUI Interface"
  LANGUAGES C CXX)

set (CONFIG_VERSION ${PROJECT_VERSION})
execute_process (
  COMMAND git describe --tags --dirty --abbrev=6
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
  RESULT_VARIABLE GIT_DESCRIBE_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if (GIT_DESCRIBE_RESULT EQUAL 0)
  set (VERSION "${GIT_DESCRIBE_OUTPUT}")
  string (REGEX REPLACE "^[^0-9]+" "" VERSION "${VERSION}")
  string (REGEX REPLACE "^1_"      "" VERSION "${VERSION}")
  string (REGEX REPLACE "^[_vV]+"  "" VERSION "${VERSION}")
  string (REGEX REPLACE "-g"   "git." VERSION "${VERSION}")
  string (REGEX REPLACE "[_|-]"   "." VERSION "${VERSION}")
  execute_process (
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVPARSE_OUTPUT
    RESULT_VARIABLE GIT_REVPARSE_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (GIT_REVPARSE_RESULT EQUAL 0 AND NOT GIT_REVPARSE_OUTPUT STREQUAL "master")
    set (VERSION "${VERSION} [${GIT_REVPARSE_OUTPUT}]")
  endif ()
else ()
  set (VERSION "${PROJECT_VERSION}")
endif ()

set (PACKAGE_NAME "Qsampler")
set (PACKAGE_VERSION "${VERSION}")
set (PACKAGE_BUGREPORT "rncbc@rncbc.org")
set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set (PACKAGE_TARNAME "qsampler")

set (ac_version "${PROJECT_VERSION}")
set (ac_prefix "${CMAKE_INSTALL_PREFIX}")

set (CONFIG_BUILD_VERSION "${PACKAGE_VERSION}")

if (CONFIG_DEBUG)
  set (CMAKE_BUILD_TYPE "Debug")
endif ()
if (CMAKE_BUILD_TYPE MATCHES "Debug")
  set (CONFIG_DEBUG 1)
endif ()
if (CONFIG_DEBUG)
  set (CONFIG_BUILD_TYPE "debug")
else ()
  set (CONFIG_BUILD_TYPE "release")
endif ()

set (CONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")

include (GNUInstallDirs)
set (CONFIG_BINDIR  "${CONFIG_PREFIX}/${CMAKE_INSTALL_BINDIR}")
set (CONFIG_LIBDIR  "${CONFIG_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set (CONFIG_DATADIR "${CONFIG_PREFIX}/${CMAKE_INSTALL_DATADIR}")
set (CONFIG_MANDIR  "${CONFIG_PREFIX}/${CMAKE_INSTALL_MANDIR}")


# Enable libgig availability.
option (CONFIG_LIBGIG "Enable libgig interface (default=yes)" 1)

# Enable unique/single instance.
option (CONFIG_XUNIQUE "Enable unique/single instance (default=yes)" 1)

# Enable debugger stack-trace option (assumes --enable-debug).
option (CONFIG_STACKTRACE "Enable debugger stack-trace (default=no)" 0)

# Enable Wayland support option.
option (CONFIG_WAYLAND "Enable Wayland support (EXPERIMENTAL) (default=no)" 0)

# Enable Qt6 build preference.
option (CONFIG_QT6 "Enable Qt6 build (default=yes)" 1)


# Fix for new CMAKE_REQUIRED_LIBRARIES policy.
if (POLICY CMP0075)
  cmake_policy (SET CMP0075 NEW)
endif ()

# Check for Qt...
if (CONFIG_QT6)
  find_package (Qt6 QUIET)
  if (NOT Qt6_FOUND)
    set (CONFIG_QT6 0)
  endif ()
endif ()

if (CONFIG_QT6)
  find_package (QT QUIET NAMES Qt6)
else ()
  find_package (QT QUIET NAMES Qt5)
endif ()

find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg)

if (CONFIG_XUNIQUE)
  find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
endif ()

find_package (Qt${QT_VERSION_MAJOR}LinguistTools)

include (CheckIncludeFile)
include (CheckIncludeFiles)
include (CheckIncludeFileCXX)
include (CheckFunctionExists)
include (CheckLibraryExists)

# Checks for libraries.
if (WIN32)
  check_function_exists (lroundf CONFIG_ROUND)
else ()
  find_library (MATH_LIBRARY m)
  # Check for round math function.
  if (MATH_LIBRARY)
    set (CMAKE_REQUIRED_LIBRARIES "${MATH_LIBRARY};${CMAKE_REQUIRED_LIBRARIES}")
    check_function_exists (lroundf CONFIG_ROUND)
  else ()
    message (FATAL_ERROR "*** math library not found.")
  endif ()
endif ()

# Checks for header files.
if (UNIX AND NOT APPLE)
  check_include_files ("fcntl.h;unistd.h;signal.h" HAVE_SIGNAL_H)
endif ()


# Find package modules
include (FindPkgConfig)

# Check for LSCP libraries.
pkg_check_modules (LSCP REQUIRED IMPORTED_TARGET lscp)
if (LSCP_FOUND)
  find_library(LSCP_LIBRARY NAMES ${LSCP_LIBRARIES} HINTS ${LSCP_LIBDIR})
endif ()
if (LSCP_LIBRARY)
  set (CONFIG_LIBLSCP 1)
  set (CMAKE_REQUIRED_LIBRARIES "${LSCP_LIBRARY};${CMAKE_REQUIRED_LIBRARIES}")
  # Check for for instrument_name in lscp_channel_info_t.
  check_include_file (lscp/client.h HAVE_LSCP_CLIENT_H)
  if (NOT HAVE_LSCP_CLIENT_H)
    set (CONFIG_INSTRUMENT_NAME 0)
  else ()
    set (CONFIG_INSTRUMENT_NAME 1)    
  endif ()
  # Check for mute/solo in lscp_channel_info_t.
  if (NOT HAVE_LSCP_CLIENT_H)
    set (CONFIG_MUTE_SOLO 0)
  else ()
    set (CONFIG_MUTE_SOLO 1)    
  endif ()
  if (CONFIG_MUTE_SOLO)
    check_function_exists (lscp_set_channel_mute CONFIG_MUTE_SOLO)
  endif ()
  if (CONFIG_MUTE_SOLO)
    check_function_exists (lscp_set_channel_solo CONFIG_MUTE_SOLO)
  endif ()
  # Check if MIDI instrument mapping is available.
  check_function_exists (lscp_map_midi_instrument CONFIG_MIDI_INSTRUMENT)
  # Check if FX sends is available.
  check_function_exists (lscp_create_fxsend CONFIG_FXSEND)
  # Check for FX send level in lscp_fxsend_info_t
  if (CONFIG_FXSEND)
    if (NOT HAVE_LSCP_CLIENT_H)
      set (CONFIG_FXSEND_LEVEL 0)
    else ()
      set (CONFIG_FXSEND_LEVEL 1)    
    endif ()
  endif ()
  # Check if FX send rename is available.
  if (CONFIG_FXSEND)
    check_function_exists (lscp_set_fxsend_name CONFIG_FXSEND_RENAME)
  endif ()
  # Check for audio_routing array type
  if (NOT HAVE_LSCP_CLIENT_H)
    set (CONFIG_AUDIO_ROUTING 0)
  else ()
    set (CONFIG_AUDIO_ROUTING 1)    
  endif ()
  # Check if global volume is available.
  check_function_exists (lscp_set_volume CONFIG_VOLUME)
  # Check if instrument editing is available.
  check_function_exists (lscp_edit_channel_instrument CONFIG_EDIT_INSTRUMENT)
  # Check for CHANNEL_MIDI LSCP event support in liblscp],
  check_include_file (lscp/event.h HAVE_LSCP_EVENT_H)
  if (NOT HAVE_LSCP_EVENT_H)
    set (CONFIG_EVENT_CHANNEL_MIDI 0)
  else ()
    set (CONFIG_EVENT_CHANNEL_MIDI 1)    
  endif ()
  # Check for DEVICE_MIDI LSCP event support in liblscp],
  if (NOT HAVE_LSCP_EVENT_H)
    set (CONFIG_EVENT_DEVICE_MIDI 0)
  else ()
    set (CONFIG_EVENT_DEVICE_MIDI 1)    
  endif ()
  # Check if max. voices / streams is available.
  check_function_exists (lscp_get_voices CONFIG_MAX_VOICES)
else ()
  message (FATAL_ERROR "*** LSCP library not found.")
  set (CONFIG_LIBLSCP 0)
endif ()

# Check for GIG libraries.
if (CONFIG_LIBGIG)
  pkg_check_modules (GIG IMPORTED_TARGET gig>=3.3.0)
  if (GIG_FOUND)
    find_library(GIG_LIBRARY NAMES ${GIG_LIBRARIES} HINTS ${GIG_LIBDIR})
  endif ()
  if (GIG_LIBRARY)
    set (CONFIG_LIBGIG 1)
   #set (CMAKE_REQUIRED_LIBRARIES "${GIG_LIBRARY};${CMAKE_REQUIRED_LIBRARIES}")
    # liggig supports fast information retrieval.
    set (CONFIG_LIBGIG_SETAUTOLOAD 1)
    # Check if libgig/SF.h is available.
    check_include_file_cxx (libgig/SF.h HAVE_LIBGIG_SF_H)
    if (NOT HAVE_LIBGIG_SF_H)
      set (CONFIG_LIBGIG_SF2 0)
    else ()
      set (CONFIG_LIBGIG_SF2 1)    
    endif ()
  else ()
    message (WARNING "*** GIG library not found.")
    set (CONFIG_LIBGIG 0)
  endif ()
endif ()


add_subdirectory (src)


if (UNIX AND NOT APPLE)
  configure_file (${PROJECT_NAME}.spec.in ${PROJECT_NAME}.spec IMMEDIATE @ONLY)
endif ()


# Configuration status
macro (SHOW_OPTION text value)
  if (${value})
    message ("${text}: yes")
  else ()
    message ("${text}: no")
  endif ()
endmacro ()


message   ("\n  ${PACKAGE_NAME} ${PACKAGE_VERSION} (Qt ${QT_VERSION})")
message   ("\n  Build target . . . . . . . . . . . . . . . . . . .: ${CONFIG_BUILD_TYPE}\n")
show_option ("  LSCP instrument name support . . . . . . . . . . ." CONFIG_INSTRUMENT_NAME)
show_option ("  LSCP mute/solo support . . . . . . . . . . . . . ." CONFIG_MUTE_SOLO)
show_option ("  LSCP MIDI instrument support . . . . . . . . . . ." CONFIG_MIDI_INSTRUMENT)
show_option ("  LSCP FX send support . . . . . . . . . . . . . . ." CONFIG_FXSEND)
show_option ("  LSCP FX send level support . . . . . . . . . . . ." CONFIG_FXSEND_LEVEL)
show_option ("  LSCP FX send rename support  . . . . . . . . . . ." CONFIG_FXSEND_RENAME)
show_option ("  LSCP audio routing support . . . . . . . . . . . ." CONFIG_AUDIO_ROUTING)
show_option ("  LSCP volume support  . . . . . . . . . . . . . . ." CONFIG_VOLUME)
show_option ("  LSCP edit instrument support . . . . . . . . . . ." CONFIG_EDIT_INSTRUMENT)
show_option ("  GigaSampler instrument file support (libgig) . . ." CONFIG_LIBGIG)
if (CONFIG_LIBGIG)
show_option ("  libgig supports fast information retrieval . . . ." CONFIG_LIBGIG_SETAUTOLOAD)
show_option ("  libgig supports SoundFont2 instruments files . . ." CONFIG_LIBGIG_SF2)
endif ()
show_option ("  LSCP channel MIDI event support  . . . . . . . . ." CONFIG_EVENT_CHANNEL_MIDI)
show_option ("  LSCP device MIDI event support . . . . . . . . . ." CONFIG_EVENT_DEVICE_MIDI)
show_option ("  LSCP runtime max. voices / disk streams support  ." CONFIG_MAX_VOICES)
message     ("")
show_option ("  Unique/Single instance support . . . . . . . . . ." CONFIG_XUNIQUE)
show_option ("  Debugger stack-trace (gdb) . . . . . . . . . . . ." CONFIG_STACKTRACE)
message   ("\n  Install prefix . . . . . . . . . . . . . . . . . .: ${CONFIG_PREFIX}\n")
