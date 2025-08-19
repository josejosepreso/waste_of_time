#include <dbus/dbus.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

typedef struct {
	char *key;
	int val;
} t_symstruct;

static t_symstruct meta_lookup[] = {
	{ "mpris:trackid", TRACK_ID },
	{ "mpris:artUrl", ART_URL },
	{ "xesam:artist", ARTIST },
	{ "xesam:album", ALBUM },
	{ "xesam:title", SONG_TITLE }
};

#define N_KEYS ( sizeof(meta_lookup) / sizeof(meta_lookup[0]) )

int key_from_string(const char *key)
{
	for (size_t i = 0; i < N_KEYS; ++i)
		{
			t_symstruct *sym = &meta_lookup[i];
			if (!strcmp(sym->key, key))
				return sym->val;
		}

	return -1;
}

void show_song(track_t *track)
{
	printf("Track id: %s\n", track->trackId);
	printf("Art URL: %s\n", track->artUrl);
	printf("Artist: %s\n", track->artist);
	printf("Album: %s\n", track->album);
	printf("Title: %s\n", track->title);
}

bool client_running(DBusConnection *dbus_conn, DBusError dbus_error, DBusMessage *dbus_msg, DBusMessage *dbus_reply)
{
	if (NULL == (dbus_msg = dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListNames")))
		{
			dbus_connection_unref(dbus_conn);
			perror("ERROR: ::dbus_message_new_method_call - Unable to allocate memory for the message!");
			return false;
		}

	if (NULL == (dbus_reply = dbus_connection_send_with_reply_and_block(dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error)))
		{
			dbus_message_unref(dbus_msg);
			dbus_connection_unref(dbus_conn);
			perror(dbus_error.name);
			perror(dbus_error.message);
			return false;
		}

	int len;
	char **dbus_result = NULL;

	if (!dbus_message_get_args(dbus_reply, &dbus_error, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &dbus_result, &len, DBUS_TYPE_INVALID))
		{
			dbus_message_unref(dbus_msg);
			dbus_message_unref(dbus_reply);
			dbus_connection_unref(dbus_conn);
			perror(dbus_error.name);
			perror(dbus_error.message);
			return false;
		}

	dbus_message_unref(dbus_msg);
	dbus_message_unref(dbus_reply);

	for (int i = 0; i < len; ++i)
		if (!strcmp(MPRIS_CLIENT_NAME, dbus_result[i]))
			return true;

	return false;
}

track_t *get_current_track(DBusConnection *dbus_conn, DBusError dbus_error, DBusMessage *dbus_msg, DBusMessage *dbus_reply)
{
	if (NULL == (dbus_msg = dbus_message_new_method_call(MPRIS_CLIENT_NAME, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get")))
		{
			dbus_connection_unref(dbus_conn);
			perror("ERROR: ::dbus_message_new_method_call - Unable to allocate memory for the message!");
			return NULL;
		}

	DBusMessageIter args;

	dbus_message_iter_init_append(dbus_msg, &args);
	char *body;

	body = "org.mpris.MediaPlayer2.Player";
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &body)) return NULL;
	body = "Metadata";
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &body)) return NULL;

	if (NULL == (dbus_reply = dbus_connection_send_with_reply_and_block(dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error)))
		{
			dbus_message_unref(dbus_msg);
			dbus_message_unref(dbus_reply);
			dbus_connection_unref(dbus_conn);
			perror(dbus_error.name);
			perror(dbus_error.message);
			return NULL;
		}

	DBusMessageIter iter, variant_iter, array_iter, dict_iter;

	if (!dbus_message_iter_init(dbus_reply, &iter))
		{
			dbus_message_unref(dbus_msg);
			dbus_message_unref(dbus_reply);
			dbus_connection_unref(dbus_conn);
			perror (dbus_error.name);
			perror (dbus_error.message);
			return NULL;
		}

	assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT);

	dbus_message_iter_recurse(&iter, &variant_iter);

	assert(dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_ARRAY && dbus_message_iter_get_element_type(&variant_iter) == DBUS_TYPE_DICT_ENTRY);

	dbus_message_iter_recurse(&variant_iter, &array_iter);

	track_t *track = malloc(sizeof(track_t));

	while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID)
		{
			const char *key;
			char** val;

			dbus_message_iter_recurse(&array_iter, &dict_iter);
			assert(dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_STRING);
			dbus_message_iter_get_basic(&dict_iter, &key);

			switch (key_from_string(key))
				{
				case TRACK_ID:
					val = &track->trackId;
					break;
				case ART_URL:
					val = &track->artUrl;
					break;
				case ARTIST:
					val = &track->artist;
					break;
				case ALBUM:
					val = &track->album;
					break;
				case SONG_TITLE:
					val = &track->title;
					break;
				default:
					goto next_key;
					break;
				}

			if (dbus_message_iter_next(&dict_iter) && dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_VARIANT)
				{
					DBusMessageIter value_iter, iter_;

					dbus_message_iter_recurse(&dict_iter, &value_iter);

					int inner_type = dbus_message_iter_get_arg_type(&value_iter);

					switch (inner_type)
						{
						case DBUS_TYPE_STRING:
							dbus_message_iter_get_basic(&value_iter, val);
							break;
						case DBUS_TYPE_ARRAY:
							dbus_message_iter_recurse(&value_iter, &iter_);
							dbus_message_iter_get_basic(&iter_, val);
							break;
						default:
							break;
						}
			}

		next_key:
			dbus_message_iter_next(&array_iter);
	}

	dbus_message_unref(dbus_msg);
	dbus_message_unref(dbus_reply);

	return track;
}
