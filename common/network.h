/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __network_h
#define __network_h

#include <glib.h>

typedef enum {
	NET_CONNECT,
	NET_CONNECT_FAIL,
	NET_CLOSE,
	NET_READ
} NetEvent;

typedef struct _Service Service;
typedef struct _Session Session;

typedef void (*NetNotifyFunc) (Session * ses, NetEvent event,
			       const gchar * line, gpointer user_data);

/** Initialize the network drivers */
void net_init(void);

/* Finish the network drivers */
void net_finish(void);

Session *net_new(NetNotifyFunc notify_func, gpointer user_data);
void net_free(Session ** ses);

void net_set_user_data(Session * ses, gpointer user_data);
void net_set_notify_func(Session * ses, NetNotifyFunc notify_func,
			 gpointer user_data);

gboolean net_connect(Session * ses, const gchar * host,
		     const gchar * port);
gboolean net_connected(Session * ses);

/** Check whether the connection is alive by sending messages.
 * @param ses The session
 * @param period Interval in seconds, use 0 to stop checking
 */
void net_set_check_connection_alive(Session * ses, guint period);

/** Check whether the connection is has timed out.
 * @param ses The session
 * @return TRUE when the connection timed out
 */
gboolean net_get_connection_timed_out(Session * ses);

/** Create a service that listens on the mentioned port.
 *  @param port The port
 *  @param notify_func The notification function for new Sessions
 *  @param user_data The user data in new Sessions
 *  @retval error_message If opening fails, a description of the error
 *          You should g_free the error_message
 *  @return The service, or NULL on error
 */
Service *net_service_new(guint16 port, NetNotifyFunc notify_func,
			 gpointer user_data, gchar ** error_message);

/** Free the service that was created by net_service_new.
 *  @param service The service to free
 */
void net_service_free(Service * service);

/** Get peer name
 *  @param ses The session
 *  @retval hostname The resolved hostname (free with g_free)
 *  @retval servname The resolved port name/service name (free with g_free)
 *  @retval error The error when it fails, or NULL to ignore
 *  @return TRUE is successful
 */
gboolean net_get_peer_name(Session * ses, gchar ** hostname,
			   gchar ** servname, GError ** error);

/** Close a session after the pending data was sent.
 * @param ses The session to close
 */
void net_close(Session * ses);
void net_printf(Session * ses, const gchar * fmt, ...);

/** Write data.
 * @param ses  The session
 * @param data The data to send
 */
void net_write(Session * ses, const gchar * data);

/** Get the name of the metaserver.
 *  First the environment variable PIONEERS_METASERVER is queried
 *  If it is not set, the use_default flag is used.
 * @param use_default If true, return the default metaserver if the
 *                    environment variable is not set.
 *                    If false, return the hostname of this computer.
 * @return The hostname of the metaserver
 */
gchar *get_metaserver_name(gboolean use_default);

/** Get the directory of the game related files.
 *  First the environment variable PIONEERS_DIR is queried
 *  If it is not set, the default value is returned
 */
const gchar *get_pioneers_dir(void);

#endif
