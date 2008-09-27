#ifndef MPD_H
#define MPD_H

typedef struct mpd mpd;
typedef struct mpd_result mpd_result;
typedef enum mpd_command mpd_command;
typedef enum mpd_error mpd_error;

extern mpd_result *MPD_OK;

mpd *
mpd_create (void);

void
mpd_destroy (mpd *);

const char *
mpd_get_last_error (mpd *, mpd_error *id)
  __attribute__ ((__nonnull__ (1)));

const char *
mpd_get_version (mpd *)
  __attribute__ ((__nonnull__ (1)));;

mpd_error
mpd_set_hostname (mpd *, const char *hostname)
  __attribute__ ((__nonnull__ (1, 2)));

mpd_error
mpd_set_port (mpd *, unsigned short port)
  __attribute__ ((__nonnull__ (1)));

const char *
mpd_get_hostname (mpd *)
  __attribute__ ((__nonnull__ (1)));

unsigned short
mpd_get_port (mpd *)
  __attribute__ ((__nonnull__ (1)));

mpd_error
mpd_connect (mpd *)
  __attribute__ ((__nonnull__ (1)));

mpd_error
mpd_disconnect (mpd *)
  __attribute__ ((__nonnull__ (1)));

mpd_result *
mpd_send_command (mpd *, mpd_command command, ...)
  __attribute__ ((__nonnull__ (1)));

const char *
mpd_result_get (mpd_result *, const char *key)
  __attribute__ ((__nonnull__ (1)));

const char *
mpd_result_next_key (mpd_result *)
  __attribute__ ((__nonnull__ (1)));

void
mpd_result_reset (mpd_result *)
  __attribute__ ((__nonnull__ (1)));

void
mpd_result_free (mpd_result *);

enum mpd_command {
  MPD_COMMAND_PLAY,
  MPD_COMMAND_PLAYID,
  MPD_COMMAND_STOP,
  MPD_COMMAND_PAUSE,
  MPD_COMMAND_STATUS,
  MPD_COMMAND_KILL,
  MPD_COMMAND_CLOSE,
  MPD_COMMAND_ADD,
  MPD_COMMAND_ADDID,
  MPD_COMMAND_DELETE,
  MPD_COMMAND_DELETEID,
  MPD_COMMAND_PLAYLIST,
  MPD_COMMAND_SHUFFLE,
  MPD_COMMAND_CLEAR,
  MPD_COMMAND_SAVE,
  MPD_COMMAND_LOAD,
  MPD_COMMAND_LISTPLAYLIST,
  MPD_COMMAND_LISTPLAYLISTINFO,
  MPD_COMMAND_LSINFO,
  MPD_COMMAND_RM,
  MPD_COMMAND_PLAYLISTINFO,
  MPD_COMMAND_PLAYLISTID,
  MPD_COMMAND_FIND,
  MPD_COMMAND_SEARCH,
  MPD_COMMAND_UPDATE,
  MPD_COMMAND_NEXT,
  MPD_COMMAND_PREVIOUS,
  MPD_COMMAND_LISTALL,
  MPD_COMMAND_VOLUME,
  MPD_COMMAND_REPEAT,
  MPD_COMMAND_RANDOM,
  MPD_COMMAND_STATS,
  MPD_COMMAND_CLEAR_ERROR,
  MPD_COMMAND_LIST,
  MPD_COMMAND_MOVE,
  MPD_COMMAND_MOVEID,
  MPD_COMMAND_SWAP,
  MPD_COMMAND_SWAPID,
  MPD_COMMAND_SEEK,
  MPD_COMMAND_SEEKID,
  MPD_COMMAND_LISTALLINFO,
  MPD_COMMAND_PING,
  MPD_COMMAND_SETVOL,
  MPD_COMMAND_PASSWORD,
  MPD_COMMAND_CROSSFADE,
  MPD_COMMAND_URL_HANDLERS,
  MPD_COMMAND_PLCHANGES,
  MPD_COMMAND_PLCHANGESPOSID,
  MPD_COMMAND_CURRENTSONG,
  MPD_COMMAND_ENABLE_DEV,
  MPD_COMMAND_DISABLE_DEV,
  MPD_COMMAND_DEVICES,
  MPD_COMMAND_COMMANDS,
  MPD_COMMAND_NOTCOMMANDS,
  MPD_COMMAND_PLAYLISTCLEAR,
  MPD_COMMAND_PLAYLISTADD,
  MPD_COMMAND_PLAYLISTFIND,
  MPD_COMMAND_PLAYLISTSEARCH,
  MPD_COMMAND_PLAYLISTMOVE,
  MPD_COMMAND_PLAYLISTDELETE,
  MPD_COMMAND_TAGTYPES,
  MPD_COMMAND_COUNT,
  MPD_COMMAND_RENAME,

  __MPD_COMMAND_LAST
};

enum mpd_error {
  MPD_SUCCESS,
  MPD_ERROR,
  MPD_ERROR_NOT_CONNECTED,
  MPD_ERROR_ALREADY_CONNECTED,
  MPD_ERROR_ACK,
  MPD_ERROR_NOT_MPD,
  MPD_ERROR_CONNECT,

  __MPD_ERROR_LAST
};

#endif	/* MPD_H */
