# Process this file with autoconf to produce a configure script.
AC_INIT(Qsampler, 0.4.2, rncbc@rncbc.org, qsampler)

AC_CONFIG_SRCDIR(src/qsampler.cpp)
AC_CONFIG_HEADERS(src/config.h)
AC_CONFIG_FILES(Makefile qsampler.spec src/src.pri src/qsampler.desktop)

# Build version string.
AC_CACHE_VAL([ac_cv_build_version], [
   ac_cv_build_version=$(git describe --tags --dirty --abbrev=6 2>/dev/null)
   if test -n "$ac_cv_build_version"; then
      ac_cv_build_version=$(echo $ac_cv_build_version | sed -r 's/^[^_]*[_v]//i')
      ac_cv_build_version=$(echo $ac_cv_build_version | sed -r 's/-g/git./')
      ac_cv_build_version=$(echo $ac_cv_build_version | sed -r 's/[[_|-]]+/./g')
      ac_cv_build_version_extra=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
      if test "x$ac_cv_build_version_extra" != "xmaster"; then
         ac_cv_build_version="$ac_cv_build_version [[$ac_cv_build_version_extra]]"
      fi
   else
      ac_cv_build_version=$PACKAGE_VERSION
   fi
])
ac_build_version="$ac_cv_build_version"
AC_DEFINE_UNQUOTED(CONFIG_BUILD_VERSION, ["$ac_build_version"], [Build version string.])

# Sanitized version string.
AC_CACHE_VAL([ac_cv_version], [
   ac_cv_version=$(echo $ac_cv_build_version | sed -r 's/^([[0-9|\.]]+).*$/\1/')
])
ac_version="$ac_cv_version"
AC_DEFINE_UNQUOTED(CONFIG_VERSION, ["$ac_version"], [Version string.])
AC_SUBST(ac_version)

# Set default installation prefix.
AC_PREFIX_DEFAULT(/usr/local)
if test "x$prefix" = "xNONE"; then
   prefix=$ac_default_prefix
fi
if test "x$exec_prefix" = "xNONE"; then
   exec_prefix=$prefix
fi
eval ac_prefix=$prefix
AC_SUBST(ac_prefix)
AC_DEFINE_UNQUOTED(CONFIG_PREFIX, ["$ac_prefix"], [Default installation prefix.])

# Set default installation directories.
eval ac_bindir=$bindir
AC_SUBST(ac_bindir)
AC_DEFINE_UNQUOTED(CONFIG_BINDIR, ["$ac_bindir"], [Default executable binary path.])

eval ac_libdir=$libdir
AC_SUBST(ac_libdir)
AC_DEFINE_UNQUOTED(CONFIG_LIBDIR, ["$ac_libdir"], [Default object library path.])

eval datarootdir=$datarootdir
eval ac_datadir=$datadir
AC_SUBST(ac_datadir)
AC_DEFINE_UNQUOTED(CONFIG_DATADIR, ["$ac_datadir"], [Default arch-idependent data path.])

eval ac_mandir=$mandir
AC_SUBST(ac_mandir)
AC_DEFINE_UNQUOTED(CONFIG_MANDIR, ["$ac_mandir"], [Default man page path.])


# Enable debugging argument option.
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug], [enable debugging (default=no)]),
  [ac_debug="$enableval"])

if test "x$ac_debug" = "xyes"; then
   AC_DEFINE(CONFIG_DEBUG, 1, [Define if debugging is enabled.])
   ac_stacktrace="yes"
   ac_debug="debug"
else
   ac_stacktrace="no"
   ac_debug="release"
fi
AC_SUBST(ac_debug)


# Enable Qt4/5 availability.
AC_ARG_ENABLE(qt4,
  AC_HELP_STRING([--enable-qt4], [enable Qt4 build (default=no)]),
  [ac_qt4="$enableval"],
  [ac_qt4="no"])


# Enable libgig availability.
AC_ARG_ENABLE(libgig,
  AC_HELP_STRING([--enable-libgig], [enable libgig interface (default=yes)]),
  [ac_libgig="$enableval"],
  [ac_libgig="yes"])

# Enable X11 unique/single instance.
AC_ARG_ENABLE(xunique,
  AC_HELP_STRING([--enable-xunique], [enable X11 unique/single instance (default=yes)]),
  [ac_xunique="$enableval"],
  [ac_xunique="yes"])

# Enable debugger stack-trace option (assumes --enable-debug).
AC_ARG_ENABLE(stacktrace,
  AC_HELP_STRING([--enable-stacktrace], [enable debugger stack-trace (default=no)]),
  [ac_stacktrace="$enableval"])


# Standard installation base dirs.
ac_with_paths=""

# Set for alternate Qt4/5 installation dir.
AC_ARG_WITH(qt4,
  AC_HELP_STRING([--with-qt4=PATH], [use alternate Qt4 install path]),
  [ac_qt4_path="$withval"], [ac_qt4_path="no"])

AC_ARG_WITH(qt5,
  AC_HELP_STRING([--with-qt5=PATH], [use alternate Qt5 install path]),
  [ac_qt5_path="$withval"], [ac_qt5_path="no"])

if test "x$ac_qt4_path" != "xno"; then
   ac_with_paths="$ac_with_paths $ac_qt4_path"
   ac_qt4="yes"
fi

if test "x$ac_qt5_path" != "xno"; then
   ac_with_paths="$ac_with_paths $ac_qt5_path"
   ac_qt4="no"
fi

# Set for alternate liblscp installation dir.
AC_ARG_WITH(liblscp,
  AC_HELP_STRING([--with-liblscp=PATH], [use alternate liblscp install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Set for alternate libgig installation dir.
AC_ARG_WITH(libgig,
  AC_HELP_STRING([--with-libgig=PATH], [use alternate libgig install path]),
  [ac_with_paths="$ac_with_paths $withval"])


# Honor user specified flags.
ac_cflags=$CFLAGS
ac_ldflags=$LDFLAGS


# Checks for programs.
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP

# Checks for languages.
AC_LANG_CPLUSPLUS

# Check for pkg-config.
PKG_PROG_PKG_CONFIG

# Check whether -std=c++11 support is necessary (4 < g++ version < 6).
AC_CACHE_CHECK([for g++ major version], [ac_cv_gxx_version_major], [
   ac_cv_gxx_version_major=$($CC -dumpversion)
   if test -n "$ac_cv_gxx_version_major"; then
      ac_cv_gxx_version_major=$(echo $ac_cv_gxx_version_major | cut -d'.' -f1);
   fi
   ac_gxx_version_major=$(($ac_cv_gxx_version_major + 0))
])
ac_gxx_version_major=$ac_cv_gxx_version_major
if test $ac_gxx_version_major -gt 4 -a $ac_gxx_version_major -lt 6; then
   CXXFLAGS="-std=c++11 $CXXFLAGS"
fi

# Check for proper flags.
ac_arch=$(uname -m)

# Check for install paths and alternatives...
ac_libdirs="lib"

if test "x$ac_arch" = "xx86_64"; then
   ac_libdirs="$ac_libdirs lib64"
fi

CFLAGS="-fPIC $CFLAGS"
CPPFLAGS="-fPIC $CPPFLAGS"

# Prepend alternate dependencies paths.
ac_path=$PATH

for X in $ac_with_paths; do
  if test -d $X/bin; then
    ac_path="$X/bin:$ac_path"
  fi
  if test -d $X/include; then
    CFLAGS="-I$X/include $CFLAGS "
    CPPFLAGS="-I$X/include $CPPFLAGS"
    ac_incpath="$X/include $ac_incpath"
  fi
  for Y in $ac_libdirs; do
    if test -d $X/$Y; then
       LIBS="-L$X/$Y $LIBS"
       ac_libs="-L$X/$Y $ac_libs"
    fi
  done
done

# A common error message:
ac_errmsg="not found in current PATH. Maybe QT development environment isn't available."

if test "x$ac_qt4" = "xyes"; then
   AC_PATH_PROG(ac_qmake, qmake-qt4, [no], $ac_path)
else
   AC_PATH_PROG(ac_qmake, qmake-qt5, [no], $ac_path)
fi
if test "x$ac_qmake" = "xno"; then
   AC_PATH_PROG(ac_cv_qmake, qmake, [no], $ac_path)
   ac_qmake=$ac_cv_qmake
fi
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_errmsg])
fi

# Check for proper Qt major version.
AC_CACHE_CHECK([for Qt major version], [ac_cv_qt_version_major], [
   ac_cv_qt_version_major=$($ac_qmake -query QT_VERSION | cut -d'.' -f1)
   ac_cv_qt_version_major=$(($ac_cv_qt_version_major + 0))
])
ac_qt_version_major=$ac_cv_qt_version_major
if test "x$ac_qt4" = "xyes"; then
   if test $ac_qt_version_major -ne 4; then
      AC_MSG_ERROR([qmake-qt4 $ac_errmsg (qt4-devel)])
   fi
else
   if test $ac_qt_version_major -ne 5; then
      AC_MSG_ERROR([qmake-qt5 $ac_errmsg (qt5-devel)])
   fi
fi

# Check for proper Qt install path.
AC_CACHE_CHECK([for Qt install path], [ac_cv_qt_install_path], [
   ac_cv_qt_install_path=$($ac_qmake -query QT_INSTALL_BINS)
])
ac_qt_install_path=$ac_cv_qt_install_path
if test -d $ac_qt_install_path; then
   ac_path="$ac_qt_install_path:$ac_path"
fi

# Check it again, now with updated PATH, just in case...
AC_PATH_PROG(ac_cv_qmake, qmake, [no], $ac_path)
ac_qmake=$ac_cv_qmake
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_errmsg])
fi

AC_SUBST(ac_qmake)

AC_CACHE_CHECK([for Qt install headers], [ac_cv_qt_install_headers], [
   ac_cv_qt_install_headers=$($ac_qmake -query QT_INSTALL_HEADERS)
])
ac_qt_install_headers=$ac_cv_qt_install_headers
if test -d $ac_qt_install_headers; then
   CFLAGS="-I$ac_qt_install_headers $CFLAGS "
   CPPFLAGS="-I$ac_qt_install_headers $CPPFLAGS"
   ac_incpath="$ac_qt_install_headers $ac_incpath"
fi

AC_CACHE_CHECK([for Qt install libraries], [ac_cv_qt_install_libs], [
   ac_cv_qt_install_libs=$($ac_qmake -query QT_INSTALL_LIBS)
])
ac_qt_install_libs=$ac_cv_qt_install_libs
if test -d $ac_qt_install_libs; then
   LIBS="-L$ac_qt_install_libs $LIBS"
   ac_libs="-L$ac_qt_install_libs $ac_libs"
fi

# Finally, check for proper Qt4/5 version.
if test "x$ac_qt4" = "xyes"; then
   AC_CACHE_CHECK([for Qt library version >= 4.4],
      ac_cv_qtversion, [
      AC_TRY_COMPILE([#include "QtCore/qglobal.h"], [
         #if QT_VERSION < 0x040400 || QT_VERSION >= 0x050000
         #error Qt library 4.4 or greater required.
         #endif
      ], ac_cv_qtversion="yes", [
         echo "no; Qt 4.4 or greater is required"
         exit 1
      ])
   ])
else
   AC_CACHE_CHECK([for Qt library version >= 5.1],
      ac_cv_qtversion, [
      AC_TRY_COMPILE([#include "QtCore/qglobal.h"], [
         #if QT_VERSION < 0x050100 || QT_VERSION >= 0x060000
         #error Qt library 5.1 or greater required.
         #endif
      ], ac_cv_qtversion="yes", [
         echo "no; Qt 5.1 or greater is required"
         exit 1
      ])
   ])
fi

# Check for Qt moc utility.
AC_PATH_PROG(ac_moc, moc, [no], $ac_path)
if test "x$ac_moc" = "xno"; then
   AC_MSG_ERROR([moc $ac_errmsg])
fi
AC_SUBST(ac_moc)

# Check for Qt uic utility.
AC_PATH_PROG(ac_uic, uic, [no], $ac_path)
if test "x$ac_uic" = "xno"; then
   AC_MSG_ERROR([uic $ac_errmsg])
fi
AC_SUBST(ac_uic)

# Check for Qt lupdate utility.
AC_PATH_PROG(ac_lupdate, lupdate, [no], $ac_path)
if test "x$ac_lupdate" = "xno"; then
   AC_MSG_ERROR([lupdate $ac_errmsg])
fi
AC_SUBST(ac_lupdate)

# Check for Qt lrelease utility.
AC_PATH_PROG(ac_lrelease, lrelease, [no], $ac_path)
if test "x$ac_release" = "xno"; then
   AC_MSG_ERROR([lrelease $ac_errmsg])
fi
AC_SUBST(ac_lrelease)


# Checks for libraries.
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(X11, main)
AC_CHECK_LIB(Xext, main)

# Check for round math function.
AC_CHECK_LIB(m, round, [ac_round="yes"], [ac_round="no"])
if test "x$ac_round" = "xyes"; then
   AC_DEFINE(CONFIG_ROUND, 1, [Define if round is available.])
fi

# Check for mandatory libraries.
PKG_CHECK_MODULES([LSCP], [lscp], [ac_liblscp="yes"], [ac_liblscp="no"])
if test "x$ac_liblscp" = "xyes"; then
   AC_DEFINE(CONFIG_LIBLSCP, 1, [Define if liblscp is available.])
   ac_cflags="$ac_cflags $LSCP_CFLAGS"
   ac_libs="$ac_libs $LSCP_LIBS"
   CFLAGS="$CFLAGS $LSCP_CFLAGS"
   CPPFLAGS="$CPPFLAGS $LSCP_CFLAGS"
   LIBS="$LIBS $LSCP_LIBS"
else
   AC_MSG_ERROR([*** LSCP library not found (liblscp).])
fi

case "{$host}" in
   *mingw*)
   ac_libs="$ac_libs -lws2_32"
   LIBS="$LIBS -lws2_32"
   ;;
esac

AC_CACHE_CHECK([for instrument_name in lscp_channel_info_t],
  ac_cv_instrument_name, [
  AC_TRY_COMPILE([#include "lscp/client.h"], [
	lscp_channel_info_t info;
	info.instrument_name = 0;
    ], ac_cv_instrument_name="yes", ac_cv_instrument_name="no")
])
ac_instrument_name=$ac_cv_instrument_name
if test "x$ac_instrument_name" = "xyes"; then
   AC_DEFINE(CONFIG_INSTRUMENT_NAME, 1, [Define if instrument_name is available.])
fi

AC_CACHE_CHECK([for mute/solo in lscp_channel_info_t],
  ac_cv_mute_solo, [
  AC_TRY_COMPILE([#include "lscp/client.h"], [
	lscp_channel_info_t info;
	info.mute = 0;
	info.solo = 0;
    ], ac_cv_mute_solo="yes", ac_cv_mute_solo="no")
])
ac_mute_solo=$ac_cv_mute_solo
if test "x$ac_mute_solo" = "xyes"; then
   AC_CHECK_LIB(lscp, lscp_set_channel_mute, [ac_mute_solo="yes"], [ac_mute_solo="no"])
fi
if test "x$ac_mute_solo" = "xyes"; then
   AC_CHECK_LIB(lscp, lscp_set_channel_solo, [ac_mute_solo="yes"], [ac_mute_solo="no"])
fi
if test "x$ac_mute_solo" = "xyes"; then
   AC_DEFINE(CONFIG_MUTE_SOLO, 1, [Define if mute/solo is available.])
fi

AC_CHECK_LIB(lscp, lscp_map_midi_instrument, [ac_midi_instrument="yes"], [ac_midi_instrument="no"])
if test "x$ac_midi_instrument" = "xyes"; then
  AC_DEFINE(CONFIG_MIDI_INSTRUMENT, 1, [Define if MIDI instrument mapping is available.])
fi

AC_CHECK_LIB(lscp, lscp_create_fxsend, [ac_fxsend="yes"], [ac_fxsend="no"])
if test "x$ac_fxsend" = "xyes"; then
  AC_DEFINE(CONFIG_FXSEND, 1, [Define if FX sends is available.])
  AC_CACHE_CHECK([for FX send level in lscp_fxsend_info_t],
    ac_cv_fxsend_level, [
    AC_TRY_COMPILE([#include "lscp/client.h"], [
	  lscp_fxsend_info_t info;
	  info.level = 0.0f;
      ], ac_cv_fxsend_level="yes", ac_cv_fxsend_level="no")
  ])
  ac_fxsend_level=$ac_cv_fxsend_level
  if test "x$ac_fxsend_level" = "xyes"; then
	  AC_DEFINE(CONFIG_FXSEND_LEVEL, 1, [Define if FX send level is available.])
  fi
  AC_CHECK_LIB(lscp, lscp_set_fxsend_name, [ac_fxsend_rename="yes"], [ac_fxsend_rename="no"])
  if test "x$ac_fxsend_rename" = "xyes"; then
	  AC_DEFINE(CONFIG_FXSEND_RENAME, 1, [Define if FX send rename is available.])
  fi
fi

AC_CACHE_CHECK([for audio_routing array type],
  ac_cv_audio_routing, [
  AC_TRY_COMPILE([#include "lscp/client.h"], [
    lscp_channel_info_t info;
    char ch = info.audio_routing[0][0];
    ], ac_cv_audio_routing="no", ac_cv_audio_routing="yes")
])
ac_audio_routing=$ac_cv_audio_routing
if test "x$ac_audio_routing" = "xyes"; then
   AC_DEFINE(CONFIG_AUDIO_ROUTING, 1, [Define if audio_routing is an integer array.])
fi

AC_CHECK_LIB(lscp, lscp_set_volume, [ac_volume="yes"], [ac_volume="no"])
if test "x$ac_volume" = "xyes"; then
  AC_DEFINE(CONFIG_VOLUME, 1, [Define if global volume is available.])
fi

AC_CHECK_LIB(lscp, lscp_edit_channel_instrument, [ac_edit_instrument="yes"], [ac_edit_instrument="no"])
if test "x$ac_edit_instrument" = "xyes"; then
  AC_DEFINE(CONFIG_EDIT_INSTRUMENT, 1, [Define if instrument editing is available.])
fi

AC_CACHE_CHECK([for CHANNEL_MIDI LSCP event support in liblscp],
  ac_cv_channel_midi_event, [
  AC_TRY_COMPILE([
	#include "lscp/client.h"
	#include "lscp/event.h"
	], [
	lscp_event_t ev;
	ev = LSCP_EVENT_CHANNEL_MIDI;
    ], ac_cv_channel_midi_event="yes", ac_cv_channel_midi_event="no")
])
ac_channel_midi_event=$ac_cv_channel_midi_event
if test "x$ac_channel_midi_event" = "xyes"; then
   AC_DEFINE(CONFIG_EVENT_CHANNEL_MIDI, 1, [Define if LSCP CHANNEL_MIDI event support is available.])
fi

AC_CACHE_CHECK([for DEVICE_MIDI LSCP event support in liblscp],
  ac_cv_device_midi_event, [
  AC_TRY_COMPILE([
	#include "lscp/client.h"
	#include "lscp/event.h"
	], [
	lscp_event_t ev;
	ev = LSCP_EVENT_DEVICE_MIDI;
    ], ac_cv_device_midi_event="yes", ac_cv_device_midi_event="no")
])
ac_device_midi_event=$ac_cv_device_midi_event
if test "x$ac_device_midi_event" = "xyes"; then
   AC_DEFINE(CONFIG_EVENT_DEVICE_MIDI, 1, [Define if LSCP DEVICE_MIDI event support is available.])
fi

AC_CHECK_LIB(lscp, lscp_get_voices, [ac_max_voices="yes"], [ac_max_voices="no"])
if test "x$ac_max_voices" = "xyes"; then
  AC_DEFINE(CONFIG_MAX_VOICES, 1, [Define if max. voices / streams is available.])
fi

# Check for optional libraries.
if test "x$ac_libgig" = "xyes"; then
   PKG_CHECK_MODULES([GIG], [gig >= 3.3.0], [ac_libgig="yes"], [ac_libgig="no"])
fi
if test "x$ac_libgig" = "xyes"; then
   AC_DEFINE(CONFIG_LIBGIG, 1, [Define if libgig is available.])
   ac_cflags="$ac_cflags $GIG_CFLAGS"
   ac_libs="$ac_libs $GIG_LIBS"
   CFLAGS="$CFLAGS $GIG_CFLAGS"
   CPPFLAGS="$CPPFLAGS $GIG_CFLAGS"
   LIBS="$LIBS $GIG_LIBS"
   AC_DEFINE(CONFIG_LIBGIG_SETAUTOLOAD, 1, [Define if libgig provides gig::File::SetAutoLoad() method.])
   ac_libgig_setautoload="yes"
else
   AC_MSG_WARN([*** GIG library not found (libgig).])
fi

# Check for round math function.
AC_CHECK_LIB(m, lroundf, [ac_round="yes"], [ac_round="no"])
if test "x$ac_round" = "xyes"; then
   AC_DEFINE(CONFIG_ROUND, 1, [Define if round is available.])
fi


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h signal.h sys/socket.h)

if test "x$ac_libgig" = "xyes"; then
   AC_CHECK_HEADER(libgig/SF.h, [ac_libgig_sf2="yes"], [ac_libgig_sf2="no"])
   if test "x$ac_libgig_sf2" = "xyes"; then
      AC_DEFINE(CONFIG_LIBGIG_SF2, 1, [Define if libgig/SF.h is available.])
   fi
fi

# Check for X11 unique/single instance.
if test "x$ac_xunique" = "xyes" -a "x$ac_qt4" = "xno"; then
   PKG_CHECK_MODULES([QT5X11EXTRAS], [Qt5X11Extras], [ac_xunique="yes"], [ac_xunique="no"])
fi
if test "x$ac_xunique" = "xyes"; then
   AC_DEFINE(CONFIG_XUNIQUE, 1, [Define if X11 unique/single instance is enabled.])
   if test "x$ac_qt4" = "xno"; then
      ac_qx11extras="x11extras"
   fi
   # Some recent distros (eg. fedora, debian) require this.
   if test "x$ac_cv_lib_X11_main" = "xyes"; then
      ac_libs="$ac_libs -lX11"
   fi
fi
AC_SUBST(ac_qx11extras)

# Check for debugging stack-trace.
if test "x$ac_stacktrace" = "xyes"; then
   AC_DEFINE(CONFIG_STACKTRACE, 1, [Define if debugger stack-trace is enabled.])
fi

AC_SUBST(ac_cflags)
AC_SUBST(ac_ldflags)
AC_SUBST(ac_incpath)
AC_SUBST(ac_libs)


# Checks for typedefs, structures, and compiler characteristics.
# AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS(system)

# Finally produce a configure header file and the makefiles.
AC_OUTPUT

# make clean > /dev/null 2>&1

# Output summary message

echo
echo "  $PACKAGE_NAME $ac_build_version"
echo
echo "  Build target . . . . . . . . . . . . . . . . . . .: $ac_debug"
echo
echo "  LSCP instrument name support . . . . . . . . . . .: $ac_instrument_name"
echo "  LSCP mute/solo support . . . . . . . . . . . . . .: $ac_mute_solo"
echo "  LSCP MIDI instrument support . . . . . . . . . . .: $ac_midi_instrument"
echo "  LSCP FX send support . . . . . . . . . . . . . . .: $ac_fxsend"
echo "  LSCP FX send level support . . . . . . . . . . . .: $ac_fxsend_level"
echo "  LSCP FX send rename support  . . . . . . . . . . .: $ac_fxsend_rename"
echo "  LSCP audio routing support . . . . . . . . . . . .: $ac_audio_routing"
echo "  LSCP volume support  . . . . . . . . . . . . . . .: $ac_volume"
echo "  LSCP edit instrument support . . . . . . . . . . .: $ac_edit_instrument"
echo "  GigaSampler instrument file support (libgig) . . .: $ac_libgig"
if test "x$ac_libgig" = "xyes"; then
echo "  libgig supports fast information retrieval . . . .: $ac_libgig_setautoload"
echo "  libgig supports SoundFont2 instruments files . . .: $ac_libgig_sf2"
fi
echo "  LSCP channel MIDI event support  . . . . . . . . .: $ac_channel_midi_event"
echo "  LSCP device MIDI event support . . . . . . . . . .: $ac_device_midi_event"
echo "  LSCP runtime max. voices / disk streams support  .: $ac_max_voices"
echo
echo "  X11 Unique/Single instance . . . . . . . . . . . .: $ac_xunique"
echo "  Debugger stack-trace (gdb) . . . . . . . . . . . .: $ac_stacktrace"
echo
echo "  Install prefix . . . . . . . . . . . . . . . . . .: $ac_prefix"
echo
echo "Now type 'make', followed by 'make install' as root."
echo
