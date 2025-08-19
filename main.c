#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "client.h"

int main(void)
{
	DBusConnection *dbus_conn = NULL;
	DBusError dbus_error;
	DBusMessage *dbus_msg = NULL;
	DBusMessage *dbus_reply = NULL;

	dbus_error_init(&dbus_error);

	if (NULL == (dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error)))
		{
			perror(dbus_error.name);
			perror(dbus_error.message);
			return EXIT_FAILURE;
		}

	if (!client_running(dbus_conn, dbus_error, dbus_msg, dbus_reply))
		{
			perror("Couldn't find client running");
			return EXIT_FAILURE;
		}

	track_t *track = get_current_track(dbus_conn, dbus_error, dbus_msg, dbus_reply);

	dbus_connection_unref(dbus_conn);

	if (track == NULL)
		return EXIT_FAILURE;

	show_song(track);

	return 0;
}
