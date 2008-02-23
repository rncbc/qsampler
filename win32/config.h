
#define PACKAGE_NAME    "Qsampler"
#define PACKAGE_VERSION "0.2.1.12"

#define CONFIG_PREFIX   "."
#define CONFIG_DEBUG    1

#if defined(__MINGW32__)
#define CONFIG_ROUND 1
#endif

#define CONFIG_INSTRUMENT_NAME  1
#define CONFIG_MUTE_SOLO        1
#define CONFIG_EDIT_INSTRUMENT  1
#define CONFIG_MIDI_INSTRUMENT  1
#define CONFIG_AUDIO_ROUTING    1
#define CONFIG_FXSEND           1
#define CONFIG_FXSEND_LEVEL     1
#define CONFIG_FXSEND_RENAME    1
#define CONFIG_VOLUME           1

#define CONFIG_EVENT_CHANNEL_MIDI 1
#define CONFIG_EVENT_DEVICE_MIDI  1

#undef  HAVE_SIGNAL_H


