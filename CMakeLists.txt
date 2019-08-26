project(QSAMPLER)

cmake_minimum_required(VERSION 3.1)

set (VERSION "0.5.7")

set (CONFIG_VERSION ${VERSION})
execute_process (
  COMMAND git describe --tags --dirty --abbrev=6
  OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
  RESULT_VARIABLE GIT_DESCRIBE_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if (GIT_DESCRIBE_RESULT EQUAL 0)
  set (VERSION "${GIT_DESCRIBE_OUTPUT}")
  string (REGEX REPLACE "^[^_vV]+" "" VERSION "${VERSION}")
  string (REGEX REPLACE "^[_vV]+"  "" VERSION "${VERSION}")
  string (REGEX REPLACE "-g"   "git." VERSION "${VERSION}")
  string (REGEX REPLACE "[_|-]+"  "." VERSION "${VERSION}")
  execute_process (
    COMMAND git rev-parse --abbrev-ref HEAD
    OUTPUT_VARIABLE GIT_REVPARSE_OUTPUT
    RESULT_VARIABLE GIT_REVPARSE_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (GIT_REVPARSE_RESULT EQUAL 0 AND NOT GIT_REVPARSE_OUTPUT STREQUAL "master")
    set (VERSION "${VERSION} [${GIT_REVPARSE_OUTPUT}]")
  endif ()  
endif ()

set (PACKAGE_NAME "Qsampler")
set (PACKAGE_VERSION "${VERSION}")
set (PACKAGE_BUGREPORT "rncbc@rncbc.org")
set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set (PACKAGE_TARNAME "qsampler")

set (CONFIG_BUILD_VERSION "${PACKAGE_VERSION}")

if (CMAKE_BUILD_TYPE)
  set (CONFIG_BUILD_TYPE CMAKE_BUILD_TYPE)
else ()
  set (CONFIG_BUILD_TYPE "release")
endif ()

set (CONFIG_DEBUG)
if (CONFIG_BUILD_TYPE MATCHES "debug")
  set(CONFIG_DEBUG 1)
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

# Fix for new CMAKE_REQUIRED_LIBRARIES policy.
if (POLICY CMP0075)
  cmake_policy (SET CMP0075 NEW)
endif ()

# Check for Qt
find_package (Qt5 REQUIRED NO_MODULE COMPONENTS Core Gui Widgets)

if (CONFIG_XUNIQUE)
  find_package (Qt5 REQUIRED NO_MODULE COMPONENTS Network)
endif ()

find_package (Qt5LinguistTools)

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
find_package (PkgConfig REQUIRED)

# Check for LSCP libraries.
pkg_check_modules (LSCP REQUIRED lscp)
set (CONFIG_LIBLSCP ${LSCP_FOUND})
if (CONFIG_LIBLSCP)
  include_directories (${LSCP_INCLUDE_DIRS})
  link_directories (${LSCP_LIBRARY_DIRS})
  set (CMAKE_REQUIRED_LIBRARIES "${LSCP_LIBRARIES};${CMAKE_REQUIRED_LIBRARIES}")
  # Check for for instrument_name in lscp_channel_info_t.
  check_include_file (lscp/client.h CONFIG_INSTRUMENT_NAME)
  # Check for mute/solo in lscp_channel_info_t.
  check_include_file (lscp/client.h CONFIG_MUTE_SOLO)
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
    check_include_file (lscp/client.h CONFIG_FXSEND_LEVEL)
  endif ()
  # Check if FX send rename is available.
  if (CONFIG_FXSEND)
    check_function_exists (lscp_set_fxsend_name CONFIG_FXSEND_RENAME)
  endif ()
  # Check for audio_routing array type
  check_include_file (lscp/client.h CONFIG_AUDIO_ROUTING)
  # Check if global volume is available.
  check_function_exists (lscp_set_volume CONFIG_VOLUME)
  # Check if instrument editing is available.
  check_function_exists (lscp_edit_channel_instrument CONFIG_EDIT_INSTRUMENT)
  # Check for CHANNEL_MIDI LSCP event support in liblscp],
  check_include_file (lscp/event.h CONFIG_EVENT_CHANNEL_MIDI)
  # Check for DEVICE_MIDI LSCP event support in liblscp],
  check_include_file (lscp/event.h CONFIG_EVENT_DEVICE_MIDI)
  # Check if max. voices / streams is available.
  check_function_exists (lscp_get_voices CONFIG_MAX_VOICES)
else ()
  message (FATAL_ERROR "*** LSCP library not found.")
endif ()

# Check for GIG libraries.
if (CONFIG_LIBGIG)
  pkg_check_modules (GIG gig>=3.3.0)
  set (CONFIG_LIBGIG ${GIG_FOUND})
  if (CONFIG_LIBGIG)
    include_directories (${GIG_INCLUDE_DIRS})
    link_directories (${GIG_LIBRARY_DIRS})
    link_libraries (${GIG_LIBRARIES})
#   set (CMAKE_REQUIRED_LIBRARIES "${GIG_LIBRARIES};${CMAKE_REQUIRED_LIBRARIES}")
    # liggig supports fast information retrieval.
    set (CONFIG_LIBGIG_SETAUTOLOAD 1)
    # Check if libgig/SF.h is available.
    check_include_file_cxx (libgig/SF.h CONFIG_LIBGIG_SF2)
  else ()
    message (WARNING "*** GIG library not found.")
  endif ()
endif ()


add_subdirectory (src)

configure_file (qsampler.spec.in qsampler.spec IMMEDIATE @ONLY)

install (FILES qsampler.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install (FILES qsampler.fr.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/fr/man1 RENAME qsampler.1)

# Configuration status
macro (SHOW_OPTION text value)
  if (${value})
    message("${text}: yes")
  else ()
    message("${text}: no")
  endif ()
endmacro ()


message   ("\n  ${PACKAGE_NAME} ${PACKAGE_VERSION}")
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
message   ("\n  Install prefix . . . . . . . . . . . . . . . . . .: ${CMAKE_INSTALL_PREFIX}")
message   ("\nNow type 'make', followed by 'make install' as root.\n")