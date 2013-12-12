# Process this file with autoconf to produce a configure script.
AC_INIT(Qsampler, 0.2.2.43, rncbc@rncbc.org, qsampler)

AC_CONFIG_SRCDIR(src/qsampler.cpp)
AC_CONFIG_HEADERS(src/config.h)
AC_CONFIG_FILES(Makefile qsampler.spec src/src.pri src/qsampler.desktop)

# Set default installation prefix.
AC_PREFIX_DEFAULT(/usr/local)
ac_prefix=$prefix
if test "x$ac_prefix" = "xNONE"; then
    ac_prefix=$ac_default_prefix
fi
AC_SUBST(ac_prefix)
AC_DEFINE_UNQUOTED(CONFIG_PREFIX, ["$ac_prefix"], [Default installation prefix.])

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
  AC_HELP_STRING([--enable-qt4], [enable Qt4 build (default=yes)]),
  [ac_qt4="$enableval"],
  [ac_qt4="yes"])

AC_ARG_ENABLE(qt5,
  AC_HELP_STRING([--enable-qt5], [enable Qt5 build (default=no)]),
  [ac_qt5="$enableval"],
  [ac_qt5="no"])

if test "x$ac_qt4" = "xno"; then
   ac_qt5="yes"
fi
if test "x$ac_qt5" = "xyes"; then
   ac_qt4="no"
fi

# Enable libgig availability.
AC_ARG_ENABLE(libgig,
  AC_HELP_STRING([--enable-libgig], [enable libgig interface (default=yes)]),
  [ac_libgig="$enableval"],
  [ac_libgig="yes"])

# Enable debugger stack-trace option (assumes --enable-debug).
AC_ARG_ENABLE(stacktrace,
  AC_HELP_STRING([--enable-stacktrace], [enable debugger stack-trace (default=no)]),
  [ac_stacktrace="$enableval"])


if test "$cross_compiling" != yes; then
# Standard installation base dirs.
ac_with_paths="/usr /usr/local"

ac_qtdirs="qt"

if test "x$ac_qt4" = "xyes"; then
   ac_qtdirs="qt4 $ac_qtdirs"
fi
if test "x$ac_qt5" = "xyes"; then
   ac_qtdirs="qt5 $ac_qtdirs"
fi

# Some a-la-debian alternatives...
for X in /usr/lib /usr/lib64 /usr/share; do
  for Y in $ac_qtdirs; do
    if test -d $X/$Y/bin; then
      ac_with_paths="$ac_with_paths $X/$Y"
    fi
  done
done
fi

# Set for alternate Qt installation dir.
AC_ARG_WITH(qt,
  AC_HELP_STRING([--with-qt=PATH], [use alternate Qt install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Set for alternate liblscp installation dir.
AC_ARG_WITH(liblscp,
  AC_HELP_STRING([--with-liblscp=PATH], [use alternate liblscp install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Set for alternate libgig installation dir.
AC_ARG_WITH(libgig,
  AC_HELP_STRING([--with-libgig=PATH], [use alternate libgig install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_GCC_TRADITIONAL

# Checks for languages.
AC_LANG_C
AC_LANG_CPLUSPLUS

# Prepend alternate dependencies paths.
ac_path=$PATH
for X in $ac_with_paths; do
  if test -d $X/bin; then
    ac_path="$X/bin:$ac_path"
  fi
# if test -x $X/qmake; then
#   ac_path="$X:$ac_path"
# fi
  if test -d $X/include; then
    for Y in $ac_qtdirs; do
      if test -d $X/include/$Y; then
        CFLAGS="-I$X/include/$Y $CFLAGS"
        CPPFLAGS="-I$X/include/$Y $CPPFLAGS"
        ac_incpath="$X/include/$Y $ac_incpath"
      fi
    done
    CFLAGS="-I$X/include $CFLAGS"
    CPPFLAGS="-I$X/include $CPPFLAGS"
    ac_incpath="$X/include $ac_incpath"
  fi
  if test -d $X/lib64; then
    LIBS="-L$X/lib64 $LIBS"
    ac_libs="-L$X/lib64 $ac_libs"
  fi
  if test -d $X/lib; then
    LIBS="-L$X/lib $LIBS"
    ac_libs="-L$X/lib $ac_libs"
  fi
done

# Check for proper flags.
ac_arch=`uname -m`
if test "x$ac_arch" = "xx86_64"; then
   CFLAGS="-fPIC $CFLAGS"
   CPPFLAGS="-fPIC $CPPFLAGS"
fi

# Check for proper Qt version.
AC_CACHE_CHECK([for Qt library version >= 4.4],
  ac_cv_qtversion, [
  AC_TRY_LINK([#include "QtCore/qglobal.h"], [
#if QT_VERSION < 0x040400
#error Qt library 4.4 or greater required.
#endif
    ], ac_cv_qtversion="yes", [
      echo "no; Qt 4.4 or greater is required"
      exit
    ])
])

# A common error message:
ac_errmsg="not found in current PATH. Maybe QT development environment isn't available (qt-devel)."

# Check for Qt qmake utility.
AC_PATH_PROG(ac_qmake, qmake, [no], $ac_path)
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_errmsg])
fi
AC_SUBST(ac_qmake)

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
AC_CHECK_LIB(lscp, main, [ac_liblscp="yes"], [ac_liblscp="no"])
if test "x$ac_liblscp" = "xno"; then
   AC_MSG_ERROR([LSCP library not found.])
fi
ac_libs="$ac_libs -llscp"
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
   AC_CHECK_LIB(gig, main, [ac_libgig="yes"], [ac_libgig="no"])
   if test "x$ac_libgig" = "xyes"; then
      AC_DEFINE(CONFIG_LIBGIG, 1, [Define if libgig is available.])
      ac_libs="$ac_libs -lgig"
      AC_MSG_CHECKING([for gig::File::SetAutoLoad() method in libgig])
      AC_LANG_SAVE
      AC_LANG_CPLUSPLUS
      CXXFLAGS="$ac_libs"
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <gig.h>
      ],[
    gig::File file;
    file.SetAutoLoad(false);
      ],
      have_libgig_setautoload="yes",
      have_libgig_setautoload="no"
      )
      AC_LANG_RESTORE
      AC_MSG_RESULT([$have_libgig_setautoload])
      if test "x$have_libgig_setautoload" = "xyes"; then
          AC_DEFINE(HAVE_LIBGIG_SETAUTOLOAD, 1, [Define if libgig provides gig::File::SetAutoLoad() method.])
      fi
   fi
fi

# Check for round math function.
AC_CHECK_LIB(m, lroundf, [ac_round="yes"], [ac_round="no"])
if test "x$ac_round" = "xyes"; then
   AC_DEFINE(CONFIG_ROUND, 1, [Define if round is available.])
fi

# Check for debugging stack-trace.
if test "x$ac_stacktrace" = "xyes"; then
   AC_DEFINE(CONFIG_STACKTRACE, 1, [Define if debugger stack-trace is enabled.])
fi

# Some recent distros (eg. fedora, debian) require this.
if test "x$ac_cv_lib_X11_main" = "xyes"; then
   ac_libs="$ac_libs -lX11"
fi

AC_SUBST(ac_libs)
AC_SUBST(ac_incpath)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h signal.h sys/socket.h)

AC_CHECK_HEADER(lscp/client.h, [ac_lscp_h="yes"], [ac_lscp_h="no"])
if test "x$ac_lscp_h" = "xno"; then
   AC_MSG_ERROR([LSCP headers not found.])
fi

# Checks for typedefs, structures, and compiler characteristics.
# AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS(system)

# Finally produce a configure header file and the makefiles.
AC_OUTPUT

# make clean > /dev/null 2>&1

# Output summary message

echo
echo "  $PACKAGE_NAME $PACKAGE_VERSION"
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
echo "  libgig supports fast information retrieval . . . .: $have_libgig_setautoload"
fi
echo "  LSCP channel MIDI event support  . . . . . . . . .: $ac_channel_midi_event"
echo "  LSCP device MIDI event support . . . . . . . . . .: $ac_device_midi_event"
echo "  LSCP runtime max. voices / disk streams support  .: $ac_max_voices"
echo
echo "  Debugger stack-trace (gdb) . . . . . . . . . . . .: $ac_stacktrace"
echo
echo "  Install prefix . . . . . . . . . . . . . . . . . .: $ac_prefix"
echo
echo "Now type 'make', followed by 'make install' as root."
echo
