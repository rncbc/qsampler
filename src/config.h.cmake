#ifndef CONFIG_H
#define CONFIG_H

/* Define to the title of this package. */
#cmakedefine PROJECT_TITLE "@PROJECT_TITLE@"

/* Define to the name of this package. */
#cmakedefine PROJECT_NAME "@PROJECT_NAME@"

/* Define to the version of this package. */
#cmakedefine PROJECT_VERSION "@PROJECT_VERSION@"

/* Define to the description of this package. */
#cmakedefine PROJECT_DESCRIPTION "@PROJECT_DESCRIPTION@"

/* Define to the homepage of this package. */
#cmakedefine PROJECT_HOMEPAGE_URL "@PROJECT_HOMEPAGE_URL@"

/* Define to the copyright of this package. */
#cmakedefine PROJECT_COPYRIGHT "@PROJECT_COPYRIGHT@"
#cmakedefine PROJECT_COPYRIGHT2 "@PROJECT_COPYRIGHT2@"

/* Define to the domain of this package. */
#cmakedefine PROJECT_DOMAIN "@PROJECT_DOMAIN@"


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


#endif /* CONFIG_H */
