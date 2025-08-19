#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define MPRIS_CLIENT_NAME "org.mpris.MediaPlayer2.spotify"

typedef struct {
	char *id;
	char *length;
	char *artUrl;
	char *artist;
	char *album;
	char *title;
} track_t;

typedef struct {
	int int_val;
	char *string_val;
	double double_val;
} value_t;

void add_property(track_t *track, const char *key, char *value)
{
	if (!strcmp(key, "xesam:artist")) {
		track->artist = value;
		return;
	}
}

int main(void)
{
	DBusError dbus_error;
	DBusConnection *dbus_conn = NULL;
	DBusMessage *dbus_msg = NULL;
	DBusMessage *dbus_reply = NULL;
	char **dbus_result = NULL;

	dbus_error_init(&dbus_error);

	if (NULL == (dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error))) {
		perror(dbus_error.name);
		perror(dbus_error.message);
		return EXIT_FAILURE;
	}

	if (NULL == (dbus_msg = dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListNames"))) {
		dbus_connection_unref(dbus_conn);
		perror("ERROR: ::dbus_message_new_method_call - Unable to allocate memory for the message!");
		return EXIT_FAILURE;
	}

	if (NULL == (dbus_reply = dbus_connection_send_with_reply_and_block(dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error))) {
		dbus_message_unref(dbus_msg);
		dbus_connection_unref(dbus_conn);
		perror(dbus_error.name);
		perror(dbus_error.message);
		return EXIT_FAILURE;
	}

	int len;

	if (!dbus_message_get_args(dbus_reply, &dbus_error, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &dbus_result, &len, DBUS_TYPE_INVALID)) {
		dbus_message_unref(dbus_msg);
		dbus_message_unref(dbus_reply);
		dbus_connection_unref(dbus_conn);
		perror(dbus_error.name);
		perror(dbus_error.message);
		return EXIT_FAILURE;
	}

	bool found = false;

	for (int i = 0; i < len; ++i)
		if (!strcmp(MPRIS_CLIENT_NAME, dbus_result[i]))
			found = true;

	dbus_message_unref(dbus_msg);
	dbus_message_unref(dbus_reply);
	// dbus_connection_unref(dbus_conn);

	if (!found) {
		perror("Couldn't find client running");
		return EXIT_FAILURE;
	}

	// CONNECT TO SPOTIFY
	if (NULL == (dbus_msg = dbus_message_new_method_call(MPRIS_CLIENT_NAME, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get"))) {
		dbus_connection_unref(dbus_conn);
		perror("ERROR: ::dbus_message_new_method_call - Unable to allocate memory for the message!");
		return EXIT_FAILURE;
	}

	DBusMessageIter args;

	dbus_message_iter_init_append(dbus_msg, &args);
	char *body;

	body = "org.mpris.MediaPlayer2.Player";
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &body)) return EXIT_FAILURE;
	body = "Metadata";
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &body)) return EXIT_FAILURE;

	if (NULL == (dbus_reply = dbus_connection_send_with_reply_and_block(dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error))) {
		dbus_message_unref(dbus_msg);
		dbus_message_unref(dbus_reply);
		dbus_connection_unref(dbus_conn);
		perror(dbus_error.name);
		perror(dbus_error.message);
		return EXIT_FAILURE;
	}

	DBusMessageIter iter, variant_iter, array_iter, dict_iter;

	if (!dbus_message_iter_init(dbus_reply, &iter)) {
		dbus_message_unref(dbus_msg);
		dbus_message_unref(dbus_reply);
		dbus_connection_unref(dbus_conn);
		perror (dbus_error.name);
		perror (dbus_error.message);
		return EXIT_FAILURE;
  }

	assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT);

	dbus_message_iter_recurse(&iter, &variant_iter);

	assert(dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_ARRAY && dbus_message_iter_get_element_type(&variant_iter) == DBUS_TYPE_DICT_ENTRY);

	dbus_message_iter_recurse(&variant_iter, &array_iter);

	track_t track = {0};

	while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID) {
		const char *key;

		dbus_message_iter_recurse(&array_iter, &dict_iter);

		assert(dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_STRING);

		dbus_message_iter_get_basic(&dict_iter, &key);
		printf("%s: ", key);

		if (dbus_message_iter_next(&dict_iter) && dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_VARIANT) {
			DBusMessageIter value_iter;
			dbus_message_iter_recurse(&dict_iter, &value_iter);

			int inner_type = dbus_message_iter_get_arg_type(&value_iter);

			if (inner_type == DBUS_TYPE_STRING) {
				const char *val;
				dbus_message_iter_get_basic(&value_iter, &val);
				printf("%s\n", val);
			}

			if (inner_type == DBUS_TYPE_INT32 || inner_type == DBUS_TYPE_UINT32 || inner_type == DBUS_TYPE_INT64 || inner_type == DBUS_TYPE_UINT64) {
				int val;
				dbus_message_iter_get_basic(&value_iter, &val);
				printf("%d\n", val);
			}

			if (inner_type == DBUS_TYPE_DOUBLE) {
				double val;
				dbus_message_iter_get_basic(&value_iter, &val);
				printf("%f\n", val);
			}

			if (inner_type == DBUS_TYPE_ARRAY) {
				DBusMessageIter iter_;

				dbus_message_iter_recurse(&value_iter, &iter_);

				const char *str;
				dbus_message_iter_get_basic(&iter_, &str);
				printf("%s\n", str);
			}
		}

		dbus_message_iter_next(&array_iter);
	}

	dbus_message_unref(dbus_msg);
	dbus_message_unref(dbus_reply);
	dbus_connection_unref(dbus_conn);

	return 0;
}
