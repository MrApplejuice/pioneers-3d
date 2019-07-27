/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2005-2013 Roland Clobus <rclobus@rclobus.nl>
 * Copyright (C) 2005 Keishi Suenaga
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

#include "config.h"
#include <stdlib.h>
#include <gio/gio.h>

#include "network.h"
#include "log.h"

struct _Service {
	GSocketListener *listener;
	GCancellable *cancellable;
	NetNotifyFunc notify_func;
	gpointer user_data;
	GSList *sessions;
	gboolean delayed_free;
};

struct _Session {
	GSocketConnection *connection;
	GCancellable *input_cancel;
	time_t last_response;	/* used for activity detection.  */
	guint timer_id;
	gboolean timed_out;
	gpointer user_data;
	Service *service; /**< Associated service, when applicable */

	gchar *host;
	guint16 port;

	char read_buff[16 * 1024];
	size_t read_len;
	gboolean entered;

	NetNotifyFunc notify_func;
	guint period; /**< Period in s for keep-alive checks */
};

static void notify(Session * ses, NetEvent event, const gchar * line)
{
	if (ses->notify_func != NULL)
		ses->notify_func(ses, event, line, ses->user_data);
}

static gboolean net_close_internal(Session * ses)
{
	if (ses->timer_id != 0) {
		g_source_remove(ses->timer_id);
		ses->timer_id = 0;
	}

	if (ses->input_cancel != NULL
	    && !g_cancellable_is_cancelled(ses->input_cancel)) {
		g_cancellable_cancel(ses->input_cancel);
	}

	if (ses->connection != NULL) {
		g_io_stream_close(G_IO_STREAM(ses->connection), NULL,
				  NULL);
		g_object_unref(ses->connection);
		ses->connection = NULL;
	}

	return !ses->entered;
}

void net_close(Session * ses)
{
	if (net_close_internal(ses))
		notify(ses, NET_CLOSE, NULL);
}

static gboolean ping_function(gpointer s)
{
	Session *ses = (Session *) s;
	double interval = difftime(time(NULL), ses->last_response);
	/* Ask for activity every ses->period seconds, but don't ask if there
	 * was activity anyway.  */
	if (interval >= 2 * ses->period) {
		/* There was no response to the ping in time.  The connection
		 * should be considered dead.  */
		log_message(MSG_ERROR,
			    "No activity and no response to ping.  Closing connection\n");
		debug("(%p) --> %s", ses->connection, "no response");
		ses->timed_out = TRUE;
		net_close(ses);
	} else if (interval >= ses->period) {
		/* There was no activity.
		 * Send a ping (but don't update activity time).  */
		net_write(ses, "hello\n");
		ses->timer_id =
		    g_timeout_add(ses->period * 1000, ping_function, s);
	} else {
		/* Everything is fine.  Reschedule this check.  */
		ses->timer_id = g_timeout_add((guint)
					      ((ses->period -
						interval) * 1000),
					      ping_function, s);
	}
	/* Return FALSE to not reschedule this timeout.  If it needed to be
	 * rescheduled, it has been done explicitly above (with a different
	 * timeout).  */
	return FALSE;
}

void net_write(Session * ses, const gchar * data)
{
	g_return_if_fail(ses != NULL);
	if (ses->connection != NULL) {
		size_t len;
		gssize num;
		GError *error;

		len = strlen(data);
		error = NULL;
		num =
		    g_output_stream_write(g_io_stream_get_output_stream
					  (G_IO_STREAM(ses->connection)),
					  data, len, NULL, &error);
		if (num < 0) {
			log_message(MSG_ERROR,
				    _("Error writing to socket: %s\n"),
				    error->message);
			g_error_free(error);
			net_close(ses);
			return;
		}
		if (strcmp(data, "yes\n") && strcmp(data, "hello\n")) {
			debug("(%p) --> %s", ses->connection, data);
		}
		if ((size_t) num != len) {
			log_message(MSG_ERROR,
				    _("Could not send all data\n"));
			net_close(ses);
			return;
		}
		return;
	}
}

void net_printf(Session * ses, const gchar * fmt, ...)
{
	char *buff;
	va_list ap;

	va_start(ap, fmt);
	buff = g_strdup_vprintf(fmt, ap);
	va_end(ap);

	net_write(ses, buff);
	g_free(buff);
}

static ssize_t find_line(char *buff, size_t len)
{
	size_t idx;

	for (idx = 0; idx < len; idx++)
		if (buff[idx] == '\n')
			return (ssize_t) idx;
	return -1;
}

static gboolean input_ready(GObject * pollable_stream, gpointer user_data)
{
	Session *ses = (Session *) user_data;
	gssize num;
	size_t offset;
	GError *error;

	/* There is data from this connection: record the time.  */
	ses->last_response = time(NULL);

	if (ses->read_len == sizeof(ses->read_buff)) {
		/* We are in trouble now - the application has not
		 * been processing the data we have been
		 * reading. Assume something has gone wrong and
		 * disconnect
		 */
		log_message(MSG_ERROR,
			    _("Read buffer overflow - disconnecting\n"));
		net_close(ses);
		return FALSE;
	}

	error = NULL;
	num =
	    g_pollable_input_stream_read_nonblocking
	    (G_POLLABLE_INPUT_STREAM(pollable_stream),
	     ses->read_buff + ses->read_len,
	     sizeof(ses->read_buff) - ses->read_len, ses->input_cancel,
	     &error);

	if (g_cancellable_is_cancelled(ses->input_cancel)) {
		g_error_free(error);
		return FALSE;
	}

	if (num == 0) {
		net_close(ses);
		return FALSE;
	}

	if (num < 0) {
		log_message(MSG_ERROR, _("Error reading socket: %s\n"),
			    error->message);
		g_error_free(error);
		net_close(ses);
		return FALSE;
	}

	ses->read_len += (size_t) num;

	if (ses->entered) {
		return TRUE;
	}
	ses->entered = TRUE;

	offset = 0;
	while (ses->connection != NULL && offset < ses->read_len) {
		gchar *line = ses->read_buff + offset;
		ssize_t len = find_line(line, ses->read_len - offset);

		if (len < 0)
			break;
		line[len] = '\0';
		offset += (size_t) (len + 1);

		if (!strcmp(line, "hello")) {
			net_write(ses, "yes\n");
			continue;
		}
		if (!strcmp(line, "yes")) {
			continue;	/* Don't notify the program */
		}

		debug("(%p) <-- %s", ses->connection, line);

		notify(ses, NET_READ, line);
	}

	if (offset < ses->read_len) {
		/* Did not process all data in buffer - discard
		 * processed data and copy remaining data to beginning
		 * of buffer until next time
		 */
		memmove(ses->read_buff, ses->read_buff + offset,
			ses->read_len - offset);
		ses->read_len -= offset;
	} else
		/* Processed all data in buffer, discard it
		 */
		ses->read_len = 0;

	ses->entered = FALSE;
	if (ses->connection == NULL) {
		net_close(ses);
	}
	return TRUE;		/* Keep the source */
}

Session *net_new(NetNotifyFunc notify_func, gpointer user_data)
{
	Session *ses;

	ses = g_malloc0(sizeof(*ses));
	ses->notify_func = notify_func;
	ses->user_data = user_data;
	ses->connection = NULL;
	ses->timed_out = FALSE;

	return ses;
}

void net_set_user_data(Session * ses, gpointer user_data)
{
	g_return_if_fail(ses != NULL);
	g_return_if_fail(ses->notify_func != NULL);
	ses->user_data = user_data;
}

void net_set_notify_func(Session * ses, NetNotifyFunc notify_func,
			 gpointer user_data)
{
	g_return_if_fail(ses != NULL);

	ses->notify_func = notify_func;
	ses->user_data = user_data;
}

void net_set_check_connection_alive(Session * ses, guint period)
{
	ses->period = period;
	if (period > 0) {
		ses->last_response = time(NULL);
		if (ses->timer_id != 0) {
			g_source_remove(ses->timer_id);
		}
		ses->timer_id =
		    g_timeout_add(period * 1000, ping_function, ses);
	} else {
		if (ses->timer_id != 0) {
			g_source_remove(ses->timer_id);
			ses->timer_id = 0;
		}
	}
}

gboolean net_get_connection_timed_out(Session * ses)
{
	return ses->timed_out;
}

gboolean net_connected(Session * ses)
{
	return (ses->connection != NULL);
}

/** Start listening on the connection in the session */
static void net_start_listening(Session * ses)
{
	GSource *input_source;

	g_assert(ses->connection != NULL);

	ses->input_cancel = g_cancellable_new();
	input_source =
	    g_pollable_input_stream_create_source(G_POLLABLE_INPUT_STREAM
						  (g_io_stream_get_input_stream
						   (G_IO_STREAM
						    (ses->connection))),
						  ses->input_cancel);
	g_source_set_callback(input_source, (GSourceFunc) input_ready, ses,
			      NULL);
	g_source_attach(input_source, NULL);
	g_source_unref(input_source);
}

gboolean net_connect(Session * ses, const gchar * host, const gchar * port)
{
	GSocketClient *client;
	GError *error;

	g_return_val_if_fail(ses->host == NULL, FALSE);
	g_return_val_if_fail(ses->connection == NULL, FALSE);

	client = g_socket_client_new();

	ses->host = g_strdup(host);
	ses->port = atoi(port);

	error = NULL;
	ses->connection =
	    g_socket_client_connect_to_host(client, host, ses->port, NULL,
					    &error);
	if (ses->connection == NULL) {
		log_message(MSG_ERROR, _("Error connecting to %s: %s\n"),
			    host, error->message);
		g_error_free(error);
		return FALSE;
	}
	net_start_listening(ses);
	return TRUE;
}

static gboolean net_delayed_free(gpointer user_data)
{
	Session *ses = user_data;
	g_free(ses);
	return FALSE;
}

/* Free and NULL-ify the session *ses */
void net_free(Session ** ses)
{
	g_return_if_fail(ses != NULL);

	/* If the sessions is still in use, do not free it */
	if (!net_close_internal(*ses)) {
		g_warning("Request to free session %p was denied, this "
			  "programming error results in a memory leak\n",
			  *ses);
		return;
	}
	if ((*ses)->service != NULL) {
		Service *service = (*ses)->service;
		service->sessions =
		    g_slist_remove(service->sessions, *ses);
		if (service->delayed_free && service->sessions == NULL) {
			g_free(service);
		}
	}

	g_free((*ses)->host);

	if ((*ses)->input_cancel != NULL) {
		g_object_unref((*ses)->input_cancel);
		g_idle_add(net_delayed_free, *ses);
	} else {
		g_free(*ses);
	}
	*ses = NULL;
}

gchar *get_metaserver_name(gboolean use_default)
{
	gchar *temp;

	temp = g_strdup(g_getenv("PIONEERS_METASERVER"));
	if (!temp)
		temp = g_strdup(g_getenv("GNOCATAN_META_SERVER"));
	if (!temp) {
		if (use_default)
			temp = g_strdup(PIONEERS_DEFAULT_METASERVER);
		else {
			temp = g_strdup(g_get_host_name());
		}
	}
	return temp;
}

const gchar *get_pioneers_dir(void)
{
	const gchar *pioneers_dir = g_getenv("PIONEERS_DIR");
	if (!pioneers_dir)
		pioneers_dir = g_getenv("GNOCATAN_DIR");
	if (!pioneers_dir)
		pioneers_dir = PIONEERS_DIR_DEFAULT;
	return pioneers_dir;
}

static void net_service_incoming(GObject * object, GAsyncResult * result,
				 gpointer user_data)
{
	GSocketListener *listener = G_SOCKET_LISTENER(object);
	Service *service = user_data;
	GSocketConnection *connection;
	GError *error = NULL;
	Session *ses;

	connection =
	    g_socket_listener_accept_finish(listener, result, NULL,
					    &error);
	if (error) {
		if (g_error_matches
		    (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_error_free(error);
			return;
		}
		g_warning("fail: %s", error->message);
		g_error_free(error);
	} else {
		g_assert(service->listener == listener);

		ses = net_new(service->notify_func, service->user_data);
		ses->service = service;
		ses->connection = connection;
		service->sessions = g_slist_append(service->sessions, ses);

		net_start_listening(ses);
		notify(ses, NET_CONNECT, NULL);
	}

	g_cancellable_reset(service->cancellable);

	/* requeue */
	g_socket_listener_accept_async(listener, service->cancellable,
				       net_service_incoming, service);
}

Service *net_service_new(guint16 port, NetNotifyFunc notify_func,
			 gpointer user_data, gchar ** error_message)
{
	Service *service;
	GSocketListener *socket_listener;
	GError *error;

	socket_listener = g_socket_listener_new();
	error = NULL;
	if (!g_socket_listener_add_inet_port
	    (socket_listener, port, NULL, &error)) {
		*error_message = g_strdup(error->message);
		g_error_free(error);
		g_socket_listener_close(socket_listener);
		g_object_unref(socket_listener);
		return NULL;
	}
	service = g_malloc0(sizeof(*service));
	service->listener = socket_listener;
	service->cancellable = g_cancellable_new();
	service->notify_func = notify_func;
	service->user_data = user_data;
	service->sessions = NULL;
	service->delayed_free = FALSE;
	g_socket_listener_accept_async(socket_listener,
				       service->cancellable,
				       net_service_incoming, service);
	return service;
}

void net_service_free(Service * service)
{
	GSList *list;
	gboolean delayed_free;

	if (service == NULL) {
		return;
	}

	g_cancellable_cancel(service->cancellable);
	g_object_unref(service->cancellable);
	service->cancellable = NULL;
	g_object_unref(service->listener);
	service->listener = NULL;

	/* Make a copy of all service related data,
	 * because net_free will remove the session
	 * and free the service (when applicable) */
	list = g_slist_copy(service->sessions);

	/* Close all remaining sessions */
	delayed_free = FALSE;
	while (list != NULL) {
		service->delayed_free = TRUE;
		delayed_free = TRUE;
		Session *ses = list->data;
		net_close(ses);
		list = g_slist_remove(list, ses);
	}

	/* Delay freeing the memory, to allow the callbacks to finish */
	if (!delayed_free) {
		g_free(service);
	}
}

gboolean net_get_peer_name(Session * ses, gchar ** hostname,
			   gchar ** servname, GError ** error)
{
	GInetAddress *inet_address;
	GSocketAddress *remote_address;
	GResolver *resolver;
	gchar *name;

	*hostname = g_strdup(_("unknown"));
	*servname = g_strdup(_("unknown"));

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	remote_address =
	    g_socket_connection_get_remote_address(ses->connection, error);
	if (error != NULL && *error != NULL) {
		g_object_unref(remote_address);
		return FALSE;
	}
	g_free(*servname);
	*servname =
	    g_strdup_printf("%u",
			    g_inet_socket_address_get_port
			    (G_INET_SOCKET_ADDRESS(remote_address)));

	inet_address =
	    g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS
					      (remote_address));
	g_object_unref(remote_address);

	resolver = g_resolver_get_default();
	name =
	    g_resolver_lookup_by_address(resolver, inet_address, NULL,
					 error);
	g_object_unref(resolver);
	if (error != NULL && *error != NULL) {
		return FALSE;
	}
	g_free(*hostname);
	*hostname = name;

	return TRUE;
}

void net_init(void)
{
	/* Do nothing thanks to GIO */
}

void net_finish(void)
{
	/* Do nothing thanks to GIO */
}
