#ifndef CONFIG_H
#define CONFIG_H

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME "@PACKAGE_NAME@"

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING "@PACKAGE_STRING@"

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION "@PACKAGE_VERSION@"

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME "@PACKAGE_TARNAME@"

/* Define to the version of this package. */
#cmakedefine CONFIG_VERSION "@CONFIG_VERSION@"

/* Define to the build version of this package. */
#cmakedefine CONFIG_BUILD_VERSION "@CONFIG_BUILD_VERSION@"

/* Default installation prefix. */
#cmakedefine CONFIG_PREFIX "@CONFIG_PREFIX@"

/* Define to target installation dirs. */
#cmakedefine CONFIG_BINDIR "@CONFIG_BINDIR@"
#cmakedefine CONFIG_LIBDIR "@CONFIG_LIBDIR@"
#cmakedefine CONFIG_DATADIR "@CONFIG_DATADIR@"
#cmakedefine CONFIG_MANDIR "@CONFIG_MANDIR@"

/* Define if debugging is enabled. */
#cmakedefine CONFIG_DEBUG @CONFIG_DEBUG@

/* Define to 1 if you have the <signal.h> header file. */
#cmakedefine HAVE_SIGNAL_H @HAVE_SIGNAL_H@

/* Define if round is available. */
#cmakedefine CONFIG_ROUND @CONFIG_ROUND@

/* Define if liblscp is available. */
#cmakedefine CONFIG_LIBLSCP @CONFIG_LIBLSCP@

/* Define if instrument_name is available. */
#cmakedefine CONFIG_INSTRUMENT_NAME @CONFIG_INSTRUMENT_NAME@

/* Define if mute/solo is available. */
#cmakedefine CONFIG_MUTE_SOLO @CONFIG_MUTE_SOLO@

/* Define if MIDI instrument mapping is available. */
#cmakedefine CONFIG_MIDI_INSTRUMENT @CONFIG_MIDI_INSTRUMENT@

/* Define if FX sends is available. */
#cmakedefine CONFIG_FXSEND @CONFIG_FXSEND@

/* Define if FX send level is available. */
#cmakedefine CONFIG_FXSEND_LEVEL @CONFIG_FXSEND_LEVEL@

/* Define if FX send rename is available. */
#cmakedefine CONFIG_FXSEND_RENAME @CONFIG_FXSEND_RENAME@

/* Define if audio_routing is an integer array. */
#cmakedefine CONFIG_AUDIO_ROUTING @CONFIG_AUDIO_ROUTING@

/* Define if global volume is available. */
#cmakedefine CONFIG_VOLUME @CONFIG_VOLUME@

/* Define if instrument editing is available. */
#cmakedefine CONFIG_EDIT_INSTRUMENT @CONFIG_EDIT_INSTRUMENT@

/* Define if LSCP CHANNEL_MIDI event support is available. */
#cmakedefine CONFIG_EVENT_CHANNEL_MIDI @CONFIG_EVENT_CHANNEL_MIDI@

/* Define if LSCP DEVICE_MIDI event support is available. */
#cmakedefine CONFIG_EVENT_DEVICE_MIDI @CONFIG_EVENT_DEVICE_MIDI@

/* Define if max. voices / streams is available. */
#cmakedefine CONFIG_MAX_VOICES @CONFIG_MAX_VOICES@

/* Define if libgig is available. */
#cmakedefine CONFIG_LIBGIG @CONFIG_LIBGIG@

/* Define if libgig provides gig::File::SetAutoLoad() method. */
#cmakedefine CONFIG_LIBGIG_SETAUTOLOAD @CONFIG_LIBGIG_SETAUTOLOAD@

/* Define if round is available. */
#cmakedefine CONFIG_ROUND @CONFIG_ROUND@

/* Define if libgig/SF.h is available. */
#cmakedefine CONFIG_LIBGIG_SF2 @CONFIG_LIBGIG_SF2@

/* Define if unique/single instance is enabled. */
#cmakedefine CONFIG_XUNIQUE @CONFIG_XUNIQUE@

/* Define if debugger stack-trace is enabled. */
#cmakedefine CONFIG_STACKTRACE @CONFIG_STACKTRACE@


/* Define if Wayland is supported */
#cmakedefine CONFIG_WAYLAND @CONFIG_WAYLAND@

#endif /* CONFIG_H */
