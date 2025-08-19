#ifndef CLIENT_H_
#define CLIENT_H_

#include <dbus/dbus.h>
#include <stdbool.h>

#define MPRIS_CLIENT_NAME "org.mpris.MediaPlayer2.spotify"

typedef struct {
	char *trackId;
	char *artUrl;
	char *artist;
	char *album;
	char *title;
} track_t;

enum {
	TRACK_ID,
	ART_URL,
	ARTIST,
	ALBUM,
	SONG_TITLE
};

void show_song(track_t *);

bool client_running(DBusConnection *, DBusError, DBusMessage *, DBusMessage *);

track_t *get_current_track(DBusConnection *, DBusError, DBusMessage *, DBusMessage *);

#endif
