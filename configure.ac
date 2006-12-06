# Process this file with autoconf to produce a configure script.
AC_INIT(Qsampler, 0.1.3.6, rncbc@rncbc.org, qsampler)
AC_CONFIG_SRCDIR(src/qsamplerMainForm.ui.h)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(Makefile qsampler.pro qsampler.spec)

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
  [ac_debug="debug"],
  [ac_debug="release"])
AC_SUBST(ac_debug)

if test "x$ac_debug" = "xdebug"; then
   AC_DEFINE(CONFIG_DEBUG, 1, [Define if debugging is enabled.])
fi

# Enable libgig availability.
AC_ARG_ENABLE(libgig,
  AC_HELP_STRING([--disable-libgig], [disable libgig interface (default=no)]),
  [ac_libgig="no"],
  [ac_libgig="yes"])

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
for X in $ac_with_paths; do
  if test -d $X/include; then
    CFLAGS="$CFLAGS -I$X/include"
    CPPFLAGS="$CPPFLAGS -I$X/include"
    ac_incpath="$X/include $ac_incpath"
  fi
  if test -d $X/lib64; then
    LIBS="$LIBS -L$X/lib64"
    ac_libs="$ac_libs -L$X/lib64"
  fi
  if test -d $X/lib; then
    LIBS="$LIBS -L$X/lib"
    ac_libs="-L$X/lib $ac_libs"
  fi
done

# Check for QTDIR environment variable.
AC_MSG_CHECKING([whether QTDIR environment variable is set])
if test "x$QTDIR" = "x"; then
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([QTDIR must be properly set.])
else
  AC_MSG_RESULT([$QTDIR])
fi

CFLAGS="$CFLAGS -I$QTDIR/include"
CPPFLAGS="$CPPFLAGS -I$QTDIR/include"
if test -d $QTDIR/lib64; then
  LIBS="$LIBS -L$QTDIR/lib64"
else
  LIBS="$LIBS -L$QTDIR/lib"
fi
for X in qt qt3; do
  if test -d $QTDIR/include/$X; then
    CFLAGS="$CFLAGS -I$QTDIR/include/$X"
    CPPFLAGS="$CPPFLAGS -I$QTDIR/include/$X"
  fi
done

# Checks for Qt library.
for X in qt-mt qt; do
  if test "x$ac_qtlib" = "x"; then
     AC_CHECK_LIB($X, main, [ac_qtlib=$X])
  fi
done
if test "x$ac_qtlib" = "x"; then
  AC_MSG_ERROR([Qt library not found. Maybe QTDIR isn't properly set.])
fi
AC_SUBST(ac_qtlib)

# Check for Qt multi-thread support.
if test "x$ac_qtlib" = "xqt-mt"; then
  ac_thread="thread"
fi
AC_SUBST(ac_thread)

AC_CACHE_CHECK([for Qt library version >= 3.1.1],
  ac_qtlib_version, [
  AC_TRY_LINK([#include "qglobal.h"], [
#if QT_VERSION < 0x030101
#error Qt library 3.1.1 or greater required.
#endif
    ],
    ac_qtlib_version="yes", [
      echo "no; Qt 3.1.1 or greater is required"
      exit
    ])
])

# A common error message:
ac_qtdir_errmsg="not found in current PATH. Maybe QT development environment isn't available (qt3-devel)."

# Check for Qt qmake utility.
AC_PATH_PROG(ac_qmake, qmake, [no], $QTDIR/bin:${PATH})
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_qtdir_errmsg])
fi
AC_SUBST(ac_qmake)

# Check for Qt moc utility.
AC_PATH_PROG(ac_moc, moc, [no], $QTDIR/bin:${PATH})
if test "x$ac_moc" = "xno"; then
   AC_MSG_ERROR([moc $ac_qtdir_errmsg])
fi
AC_SUBST(ac_moc)

# Check for Qt uic utility.
AC_PATH_PROG(ac_uic, uic, [no], $QTDIR/bin:${PATH})
if test "x$ac_uic" = "xno"; then
   AC_MSG_ERROR([uic $ac_qtdir_errmsg])
fi
AC_SUBST(ac_uic)

# Checks for libraries.
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(X11, main)
AC_CHECK_LIB(Xext, main)
AC_CHECK_LIB($ac_qtlib, main)

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

AC_CACHE_CHECK([for instrument_name in lscp_channel_info_t],
  ac_instrument_name, [
  AC_TRY_COMPILE([#include "lscp/client.h"], [
	lscp_channel_info_t info;
	info.instrument_name = 0;
    ], ac_instrument_name="yes", ac_instrument_name="no")
])
if test "x$ac_instrument_name" = "xyes"; then
   AC_DEFINE(CONFIG_INSTRUMENT_NAME, 1, [Define if instrument_name is available.])
fi

AC_CACHE_CHECK([for mute/solo in lscp_channel_info_t],
  ac_mute_solo, [
  AC_TRY_COMPILE([#include "lscp/client.h"], [
	lscp_channel_info_t info;
	info.mute = 0;
	info.solo = 0;
    ], ac_mute_solo="yes", ac_mute_solo="no")
])
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

# Check for optional libraries.
if test "x$ac_libgig" = "xyes"; then
   AC_CHECK_LIB(gig, main, [ac_libgig="yes"], [ac_libgig="no"])
   if test "x$ac_libgig" = "xyes"; then
      AC_DEFINE(CONFIG_LIBGIG, 1, [Define if libgig is available.])
      ac_libs="$ac_libs -lgig"
   fi
fi

AC_SUBST(ac_libs)
AC_SUBST(ac_incpath)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h signal.h)

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

echo ""
echo "Now type 'make', followed by 'make install' as root."
echo ""
