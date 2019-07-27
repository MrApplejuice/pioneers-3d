/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003-2009 Bas Wijnen <wijnen@debian.org>
 * Copyright (C) 2006,2013 Roland Clobus <rclobus@rclobus.nl>
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
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include <glib.h>
#include <glib-object.h>
#include "network.h"
#include "game.h"
#include "version.h"
#include "log.h"
#include "common_glib.h"
#include "game-list.h"

typedef enum {
	META_UNKNOWN,
	META_CLIENT,
	META_SERVER_ALMOST,
	META_SERVER
} ClientType;

typedef struct _Client Client;
struct _Client {
	ClientType type;
	Session *session;

	gint protocol_major;
	gint protocol_minor;

	/* The rest of the structure is only used for METASERVER clients
	 */
	gchar *host;
	gchar *port;
	gchar *version;
	gint max;
	gint curr;
	gint previous_curr;
	gchar *terrain;
	gchar *title;
	gchar *vpoints;
	gchar *sevenrule;
};

static GMainLoop *event_loop;
static Service *service;

static gchar *redirect_location = NULL;
static gchar *myhostname = NULL;
static gboolean can_create_games;

static int port_low = 0;
static int port_high = 0;

static GList *client_list;

/* Command line data */
static gboolean make_daemon = FALSE;
static gchar *pidfile = NULL;
static gchar *port_range = NULL;
static gboolean enable_debug = FALSE;
static gboolean enable_syslog_debug = FALSE;
static gboolean show_version = FALSE;

static void log_to_syslog(gint msg_type, const gchar * text)
{
	int priority;
	gboolean to_syslog;

	to_syslog = TRUE;
	switch (msg_type) {
	case MSG_ERROR:
		priority = LOG_ERR;
		break;
	case MSG_INFO:
		priority = LOG_INFO;
		break;
	case MSG_TIMESTAMP:
		to_syslog = FALSE;
		break;
	default:
		priority = LOG_DEBUG;
		break;
	}
	if (to_syslog) {
		syslog(priority, "%s", text);
	}
	if (enable_syslog_debug) {
		if (msg_type == MSG_TIMESTAMP) {
			log_message_string_console(msg_type, text);
		} else {
			gchar *t = g_strdup_printf("%s\n", text);
			log_message_string_console(msg_type, t);
			g_free(t);
		}
	}
}

static void client_free(Client * client)
{
	if (client->session != NULL) {
		net_free(&client->session);
	}
	if (client->host != NULL)
		g_free(client->host);
	if (client->port != NULL)
		g_free(client->port);
	if (client->version != NULL)
		g_free(client->version);
	if (client->terrain != NULL)
		g_free(client->terrain);
	if (client->title != NULL)
		g_free(client->title);
	if (client->vpoints != NULL)
		g_free(client->vpoints);
	if (client->sevenrule != NULL)
		g_free(client->sevenrule);
	g_free(client);
}

static void client_list_servers(Client * client)
{
	GList *list;

	for (list = client_list; list != NULL; list = g_list_next(list)) {
		Client *scan = list->data;

		if (scan->type != META_SERVER)
			continue;
		net_printf(client->session,
			   "server\n"
			   "host=%s\n"
			   "port=%s\n"
			   "version=%s\n"
			   "max=%d\n"
			   "curr=%d\n",
			   scan->host, scan->port, scan->version,
			   scan->max, scan->curr);
		if (client->protocol_major == 0) {
			net_printf(client->session,
				   "map=%s\n"
				   "comment=%s\n",
				   scan->terrain, scan->title);
		} else if (client->protocol_major >= 1) {
			net_printf(client->session,
				   "vpoints=%s\n"
				   "sevenrule=%s\n"
				   "terrain=%s\n"
				   "title=%s\n",
				   scan->vpoints,
				   scan->sevenrule,
				   scan->terrain, scan->title);
		}
		net_printf(client->session, "end\n");
	}
}

/** Send the title and free the associated memory. */
static void client_send_type(gpointer data, gpointer user_data)
{
	GameParams *params = data;
	Session *ses = user_data;

	if (!params_game_is_unstartable(params)) {
		net_printf(ses, "title=%s\n", params->title);
	}
}

static void client_list_types(Client * client)
{
	game_list_foreach(client_send_type, client->session);
}

static void client_list_capability(Session * ses)
{
	if (can_create_games) {
		net_printf(ses, "create games\n");
	}
	net_printf(ses, "deregister dead connections\n");
	net_printf(ses, "end\n");
}

static const gchar *get_server_path(void)
{
	const gchar *console_server;
	if (!(console_server = g_getenv("PIONEERS_SERVER_CONSOLE")))
		if (!
		    (console_server = g_getenv("GNOCATAN_SERVER_CONSOLE")))
			console_server =
			    PIONEERS_SERVER_CONSOLE_PROGRAM_NAME;
	return console_server;
}

static void client_create_new_server(Client * client, const gchar * line)
{
	gint free_port;
	gboolean found_free_port;
	const char *console_server;
	unsigned int n;
	GList *list;
	GSpawnFlags spawn_flags = G_SPAWN_STDOUT_TO_DEV_NULL |
	    G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_SEARCH_PATH;
	gchar *child_argv[34];
	GError *error = NULL;
	gchar **split;

	split = g_strsplit(line, " ", 6);
	if (split[0] != NULL && split[1] != NULL && split[2] != NULL
	    && split[3] != NULL && split[4] != NULL && split[5] != NULL) {
		/* Data is present, but not validated */
		/* 0: terrain
		 * 1: number of players
		 * 2: number of points
		 * 3: sevens rule
		 * 4: number of computer players
		 * 5: title
		 */
	} else {
		net_printf(client->session, "Badly formatted request\n");
		g_strfreev(split);
		return;
	}

	console_server = get_server_path();

	/* Find a free port */
	found_free_port = FALSE;
	for (free_port = port_low; free_port <= port_high; free_port++) {
		gboolean in_use = FALSE;
		for (list = client_list; list != NULL;
		     list = g_list_next(list)) {
			Client *scan = list->data;
			if ((scan->port != NULL)
			    && (atoi(scan->port) == free_port)) {
				in_use = TRUE;
			}
		}
		if (!in_use) {
			/* Check whether the port is already in use */
			Service *test_available;
			gchar *error_message;

			test_available =
			    net_service_new(free_port, NULL, NULL,
					    &error_message);
			if (test_available != NULL) {
				net_service_free(test_available);
				found_free_port = TRUE;
				break;
			}
			g_free(error_message);
		}
	}
	if (!found_free_port) {
		net_printf(client->session,
			   "Starting server failed: "
			   "no port available\n");
		g_strfreev(split);
		return;
	}

	n = 0;
	child_argv[n++] = g_strdup(console_server);
	child_argv[n++] = g_strdup(console_server);
	child_argv[n++] = g_strdup("-g");
	child_argv[n++] = g_strdup(split[5]);
	child_argv[n++] = g_strdup("-P");
	child_argv[n++] = g_strdup(split[1]);
	child_argv[n++] = g_strdup("-v");
	child_argv[n++] = g_strdup(split[2]);
	child_argv[n++] = g_strdup("-R");
	child_argv[n++] = g_strdup(split[3]);
	child_argv[n++] = g_strdup("-T");
	child_argv[n++] = g_strdup(split[0]);
	child_argv[n++] = g_strdup("-p");
	child_argv[n++] = g_strdup_printf("%d", free_port);
	child_argv[n++] = g_strdup("-c");
	child_argv[n++] = g_strdup(split[4]);
	child_argv[n++] = g_strdup("-k");
	child_argv[n++] = g_strdup("1200");
	child_argv[n++] = g_strdup("-m");
	child_argv[n++] = g_strdup(myhostname);
	child_argv[n++] = g_strdup("-n");
	child_argv[n++] = g_strdup(myhostname);
	child_argv[n++] = g_strdup("-x");
	child_argv[n++] = g_strdup("-t");
	child_argv[n++] = g_strdup("1");
	child_argv[n++] = g_strdup("-r");
	child_argv[n] = NULL;
	g_assert(n < 34);

	g_strfreev(split);

	if (!g_spawn_async(NULL, child_argv, NULL, spawn_flags, NULL, NULL,
			   NULL, &error)) {
		log_message(MSG_ERROR, "cannot exec %s: %s",
			    child_argv[0], error->message);
		g_error_free(error);
	}
	for (n = 0; child_argv[n] != NULL; n++)
		g_free(child_argv[n]);

	net_printf(client->session, "host=%s\n", myhostname);
	net_printf(client->session, "port=%d\n", free_port);
	net_printf(client->session, "started\n");
	log_message(MSG_INFO, "new local server started on port %d, "
		    "requested by %s", free_port, client->host);
	return;
}

static gboolean check_str_info(const gchar * line, const gchar * prefix,
			       gchar ** data)
{
	guint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	if (*data != NULL)
		g_free(*data);
	*data = g_strdup(line + len);
	return TRUE;
}

static gboolean check_int_info(const gchar * line, const gchar * prefix,
			       gint * data)
{
	guint len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	*data = atoi(line + len);
	return TRUE;
}

static void try_make_server_complete(Client * client)
{
	gboolean ok = FALSE;

	if (client->type == META_SERVER) {
		if (client->curr != client->previous_curr) {
			log_message(MSG_INFO,
				    "server %s on port %s: "
				    "now %d of %d players",
				    client->host, client->port,
				    client->curr, client->max);
			client->previous_curr = client->curr;
		}
		return;
	}

	if (client->host != NULL
	    && client->port != NULL
	    && client->version != NULL
	    && client->max >= 0
	    && client->curr >= 0
	    && client->terrain != NULL && client->title != NULL) {
		if (client->protocol_major < 1) {
			if (!client->vpoints)
				client->vpoints = g_strdup("?");
			if (!client->sevenrule)
				client->sevenrule = g_strdup("?");
			ok = TRUE;
		} else {
			if (client->vpoints != NULL
			    && client->sevenrule != NULL)
				ok = TRUE;
		}
	}

	if (ok) {
		client->type = META_SERVER;
		log_message(MSG_INFO,
			    "server %s on port %s registered",
			    client->host, client->port);
		net_set_check_connection_alive(client->session, 480u);
	}
}

static void client_process_line(Client * client, const gchar * line)
{
	GError *error = NULL;

	switch (client->type) {
	case META_UNKNOWN:
	case META_CLIENT:
		if (strncmp(line, "version ", 8) == 0) {
			const gchar *p = line + 8;
			client->protocol_major = atoi(p);
			p += strspn(p, "0123456789");
			if (*p == '.')
				client->protocol_minor = atoi(p + 1);
		} else if (strcmp(line, "listservers") == 0 ||
			   /* still accept "client" request from proto 0 clients
			    * so we don't have to distinguish between client versions */
			   strcmp(line, "client") == 0) {
			client->type = META_CLIENT;
			client_list_servers(client);
			net_close(client->session);
		} else if (strcmp(line, "listtypes") == 0) {
			client->type = META_CLIENT;
			client_list_types(client);
			net_close(client->session);
		} else if (strncmp(line, "create ", 7) == 0
			   && can_create_games) {
			client->type = META_CLIENT;
			if (!net_get_peer_name
			    (client->session, &client->host, &client->port,
			     &error)) {
				log_message(MSG_ERROR, "%s",
					    error->message);
				g_error_free(error);
			}
			client_create_new_server(client, line + 7);
			net_close(client->session);
		} else if (strcmp(line, "server") == 0) {
			client->type = META_SERVER_ALMOST;
			client->max = -1;
			client->curr = -1;
			client->previous_curr = -1;
			if (!net_get_peer_name
			    (client->session, &client->host, &client->port,
			     &error)) {
				log_message(MSG_ERROR, "%s",
					    error->message);
				g_error_free(error);
			}
		} else if (strcmp(line, "capability") == 0) {
			client->type = META_CLIENT;
			client_list_capability(client->session);
		} else {
			net_printf(client->session, "bad command\n");
			net_close(client->session);
		}
		break;
	case META_SERVER:
	case META_SERVER_ALMOST:
		if (check_str_info(line, "host=", &client->host)
		    || check_str_info(line, "port=", &client->port)
		    || check_str_info(line, "version=", &client->version)
		    || check_int_info(line, "max=", &client->max)
		    || check_int_info(line, "curr=", &client->curr)
		    || check_str_info(line, "terrain=", &client->terrain)
		    || check_str_info(line, "title=", &client->title)
		    || check_str_info(line, "vpoints=", &client->vpoints)
		    || check_str_info(line, "sevenrule=",
				      &client->sevenrule)
		    /* meta-protocol 0.0 compat */
		    || check_str_info(line, "map=", &client->terrain)
		    || check_str_info(line, "comment=", &client->title))
			try_make_server_complete(client);
		else if (strcmp(line, "begin") == 0)
			net_close(client->session);
		break;
	}
}

static void log_about_closed_server(Client * client)
{
	if (client->host != NULL && client->port != NULL) {
		if (net_get_connection_timed_out(client->session)) {
			log_message(MSG_INFO,
				    "server %s on port %s did not respond, "
				    "deregistering", client->host,
				    client->port);
		} else {
			log_message(MSG_INFO,
				    "server %s on port %s unregistered",
				    client->host, client->port);
		}
	}
}

static void meta_event(Session * ses, NetEvent event, const gchar * line,
		       gpointer user_data)
{
	Client *client = (Client *) user_data;

	switch (event) {
	case NET_READ:
		/* there is data to be read */
		client_process_line(client, line);
		break;
	case NET_CLOSE:
		/* connection has been closed */
		if (client != NULL) {
			switch (client->type) {
			case META_UNKNOWN:
			case META_CLIENT:
				/* No logging required */
				break;
			case META_SERVER_ALMOST:
			case META_SERVER:
				log_about_closed_server(client);
				break;
			}
			client_list = g_list_remove(client_list, client);
			client_free(client);
		} else {
			net_free(&ses);
		}
		break;
	case NET_CONNECT:
		if (redirect_location != NULL) {
			net_printf(ses, "goto %s\n", redirect_location);
			net_close(ses);
		} else {
			net_printf(ses,
				   "welcome to the pioneers-metaserver "
				   "version %s\n", META_PROTOCOL_VERSION);
			client = g_malloc0(sizeof(*client));

			client->type = META_UNKNOWN;
			client->protocol_major = 0;
			client->protocol_minor = 0;
			client->session = ses;

			client_list = g_list_append(client_list, client);
			net_set_user_data(ses, client);
			net_set_check_connection_alive(ses, 30u);
		}
		break;
	case NET_CONNECT_FAIL:
		net_free(&ses);
		break;
	}

}

static void convert_to_daemon(void)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		log_message(MSG_ERROR, "could not fork: %s",
			    g_strerror(errno));
		exit(1);
	}
	if (pid != 0) {
		/* Write the pidfile if required.
		 */
		if (pidfile) {
			FILE *f = fopen(pidfile, "w");
			if (!f) {
				fprintf(stderr,
					"Unable to open pidfile '%s': %s\n",
					pidfile, g_strerror(errno));
			} else {
				int r = fprintf(f, "%d\n", pid);
				if (r <= 0) {
					fprintf(stderr,
						"Unable to write to pidfile "
						"'%s': %s\n", pidfile,
						g_strerror(errno));
				}
				fclose(f);
			}
		}
		/* I am the parent, if I exit then init(8) will become
		 * the parent of the child process
		 */
		exit(0);
	}

	/* Create a new session to become a process group leader
	 */
	if (setsid() < 0) {
		log_message(MSG_ERROR, "could not setsid: %s",
			    g_strerror(errno));
		exit(1);
	}
	if (chdir("/") < 0) {
		log_message(MSG_ERROR, "could not chdir to /: %s",
			    g_strerror(errno));
		exit(1);
	}
	umask(0);
}

static GOptionEntry commandline_entries[] = {
	{"daemon", 'd', 0, G_OPTION_ARG_NONE, &make_daemon,
	 /* Commandline metaserver: daemon */
	 N_("Daemonize the metaserver on start"), NULL},
	{"pidfile", 'P', 0, G_OPTION_ARG_STRING, &pidfile,
	 /* Commandline metaserver: pidfile */
	 N_("Pidfile to create when daemonizing (implies -d)"),
	 /* Commandline metaserver: pidfile argument */
	 N_("filename")},
	{"redirect", 'r', 0, G_OPTION_ARG_STRING, &redirect_location,
	 /* Commandline metaserver: redirect */
	 N_("Redirect clients to another metaserver"), NULL},
	{"servername", 's', 0, G_OPTION_ARG_STRING, &myhostname,
	 /* Commandline metaserver: server */
	 N_("Use this hostname when creating new games"),
	 /* Commandline metaserver: server argument */
	 N_("hostname")},
	{"port-range", 'p', 0, G_OPTION_ARG_STRING, &port_range,
	 /* Commandline metaserver: port-range */
	 N_("Use this port range when creating new games"),
	 /* Commandline metaserver: port-range argument */
	 N_("from-to")},
	{"debug", '\0', 0, G_OPTION_ARG_NONE, &enable_debug,
	 /* Commandline option of metaserver: enable debug logging */
	 N_("Enable debug messages"), NULL},
	{"syslog-debug", '\0', 0, G_OPTION_ARG_NONE, &enable_syslog_debug,
	 /* Commandline option of metaserver: syslog-debug */
	 N_("Debug syslog messages"), NULL},
	{"version", '\0', 0, G_OPTION_ARG_NONE, &show_version,
	 /* Commandline option of metaserver: version */
	 N_("Show version information"), NULL},
	{NULL, '\0', 0, 0, NULL, NULL, NULL}
};

static gboolean handle_break_quit_request(G_GNUC_UNUSED gpointer user_data)
{
	g_main_loop_quit(event_loop);
	return FALSE;
}

static void handle_break_signal(G_GNUC_UNUSED int signal)
{
	/* Cancelling the service is asynchronous, the event queue must be
	 * processed before g_main_loop_quit can be called. */
	net_service_free(service);
	service = NULL;
	g_idle_add(handle_break_quit_request, NULL);
}

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	gchar *error_message;

	set_ui_driver(&Glib_Driver);
	g_type_init();

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	/* have gettext return strings in UTF-8 */
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif

	/* Long description in the commandline for metaserver: help */
	context = g_option_context_new(_("- Metaserver for Pioneers"));
	g_option_context_add_main_entries(context,
					  commandline_entries, PACKAGE);
	g_option_context_parse(context, &argc, &argv, &error);
	g_option_context_free(context);
	if (error != NULL) {
		g_print("%s\n", error->message);
		g_error_free(error);
		return 1;
	};

	if (show_version) {
		g_print(_("Pioneers version:"));
		g_print(" ");
		g_print(FULL_VERSION);
		g_print(", ");
		g_print(_("metaserver protocol:"));
		g_print(" ");
		g_print(META_PROTOCOL_VERSION);
		g_print("\n");
		return 0;
	}
	log_set_func(log_to_syslog);
	set_enable_debug(enable_debug);

	if (port_range) {
		gint count;

		count = sscanf(port_range, "%d-%d", &port_low, &port_high);
		if ((count != 2) || (port_low < 0)
		    || (port_low > port_high)) {
			g_print("Port range '%s' is not valid\n",
				port_range);
			return 1;
		}
	}

	net_init();
	openlog("pioneers-metaserver", LOG_PID, LOG_USER);
	if (make_daemon || pidfile)
		convert_to_daemon();

	can_create_games = FALSE;
	game_list_prepare();
	if (!game_list_is_empty()) {
		gchar *server_name =
		    g_find_program_in_path(get_server_path());
		if (server_name) {
			can_create_games = TRUE;
		}
		g_free(server_name);
	}
	can_create_games = can_create_games && (port_range != NULL);

	if (!myhostname)
		myhostname = get_metaserver_name(FALSE);

	service =
	    net_service_new(atoi(PIONEERS_DEFAULT_META_PORT), meta_event,
			    NULL, &error_message);
	if (!service) {
		log_message(MSG_ERROR, "%s", error_message);
		g_free(error_message);
		return 1;
	}

	struct sigaction break_action;
	struct sigaction old_break_action;

	break_action.sa_handler = handle_break_signal;
	break_action.sa_flags = 0;
	sigemptyset(&break_action.sa_mask);
	sigaction(SIGINT, &break_action, &old_break_action);

	log_message(MSG_INFO, "Pioneers metaserver started.");

	event_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(event_loop);
	g_main_loop_unref(event_loop);

	sigaction(SIGINT, &old_break_action, NULL);
	g_free(pidfile);
	g_free(redirect_location);
	g_free(myhostname);
	g_free(port_range);
	net_service_free(service);
	game_list_cleanup();

	net_finish();
	return 0;
}
