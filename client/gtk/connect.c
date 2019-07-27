/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003,2006 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2004,2010 Roland Clobus <rclobus@rclobus.nl>
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
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "frontend.h"
#include "network.h"
#include "log.h"
#include "config-gnome.h"
#include "select-game.h"
#include "game-settings.h"
#include "game-rules.h"
#include "metaserver.h"
#include "avahi.h"
#include "avahi-browser.h"
#include "client.h"
#include "common_gtk.h"
#include "game-list.h"

const int PRIVATE_GAME_HISTORY_SIZE = 10;

static gboolean connect_spectator;	/* Prefer to be a spectator */
static gboolean connect_spectator_allowed;	/* Spectator allowed */
static gchar *connect_server;	/* Name of the server */
static gchar *connect_port;	/* Port of the server */

static GtkWidget *connect_dlg;	/* Dialog for starting a new game */
static GtkWidget *name_entry;	/* Name of the player */
static GtkWidget *spectator_toggle;	/* Prefer to be a spectator */
static GtkWidget *metaserver_entry;	/* Name of the metaserver */

static GtkWidget *meta_dlg;	/* Dialog for joining a public game */
static GtkWidget *server_status;	/* Description of the current metaserver */
static GtkListStore *meta_games_model;	/* The list of the games at the metaserver */
static GtkWidget *meta_games_view;	/* The view on meta_games_model */

enum {
	META_RESPONSE_NEW = 1,	/* Response for new game */
	META_RESPONSE_REFRESH = 2	/* Response for refresh of the list */
};

enum {				/* The columns of the meta_games_model */
	C_META_HOST,
	C_META_PORT,
	C_META_HOST_SORTABLE,	/* Invisible, used for sorting */
	C_META_VERSION,
	C_META_MAX,
	C_META_CUR,
	C_META_TERRAIN,
	C_META_VICTORY,
	C_META_SEVENS,
	C_META_MAP,
	META_N_COLUMNS
};

static GtkWidget *cserver_dlg;	/* Dialog for creating a public game */
static GtkWidget *select_game;	/* select game type */
static GtkWidget *game_settings;	/* game settings widget */
static GtkWidget *aiplayers_spin;	/* number of AI players */
static GtkWidget *game_rules;	/* Adjust some rules */

static gboolean cfg_terrain;	/* Random terrain */
static guint cfg_num_players;
static guint cfg_victory_points;
static guint cfg_sevens_rule;
static guint cfg_ai_players;
static const gchar *cfg_gametype;	/* Will be set be the widget */

static GtkWidget *connect_private_dlg;	/* Join a private game */
static GtkWidget *host_entry;	/* Host name entry */
static GtkWidget *port_entry;	/* Host port entry */

static enum {
	GAMETYPE_MODE_SIGNON,
	GAMETYPE_MODE_LIST
} gametype_mode;

static enum {
	CREATE_MODE_SIGNON,
	CREATE_MODE_WAIT_FOR_INFO,
	CREATE_MODE_STARTING_OK,
	CREATE_MODE_STARTING_ERROR
} create_mode;

static enum {
	MODE_SIGNON,
	MODE_REDIRECT,
	MODE_LIST,
	MODE_CAPABILITY,
	MODE_REDIRECT_OVERFLOW,
	MODE_DONE,
	MODE_SERVER_INFO
} meta_mode;

/** Information about the metaserver */
static struct {
	/** Server name */
	gchar *server;
	/** Port */
	gchar *port;
	/** Major version number of metaserver protocol */
	gint version_major;
	/** Minor version number of metaserver protocol */
	gint version_minor;
	/** Number of times the metaserver has redirected */
	gint num_redirects;
	/** The metaserver can create remote games */
	gboolean can_create_games;
	/** Active session */
	Session *session;
	/** Number of available game titles */
	guint num_available_titles;
	/** The settings of a game */
	GameParams *params;
} metaserver_info = {
NULL, NULL, 0, 0, 0, FALSE, NULL, 0u, NULL};

#define STRARG_LEN 128
#define INTARG_LEN 16

static gchar server_host[STRARG_LEN];
static gchar server_port[STRARG_LEN];
static gchar server_version[STRARG_LEN];
static gchar server_max[INTARG_LEN];
static gchar server_curr[INTARG_LEN];
static gchar server_vpoints[STRARG_LEN];
static gchar *server_sevenrule = NULL;
static gchar *server_terrain = NULL;
static gchar server_title[STRARG_LEN];

static void query_metaserver(const gchar * server, const gchar * port);
static void show_waiting_box(const gchar * message, const gchar * server,
			     const gchar * port);
static void close_waiting_box(void);

static void connect_set_field(gchar ** field, const gchar * value);
static void connect_close_all(gboolean user_pressed_ok,
			      gboolean can_be_spectator);
static void set_metaserver_info(void);
static void connect_private_dialog(G_GNUC_UNUSED GtkWidget * widget,
				   GtkWindow * parent);

static void connect_set_field(gchar ** field, const gchar * value)
{
	gchar *temp = g_strdup(value);
	g_free(*field);
	if (temp != NULL) {
		*field = g_strdup(g_strstrip(temp));
	} else {
		*field = NULL;
	}
	g_free(temp);
}

/* Reset all pointers to the destroyed children of the dialog */
static void connect_dlg_destroyed(GtkWidget * widget,
				  GtkWidget ** widget_pointer)
{
	name_entry = NULL;
	spectator_toggle = NULL;
	metaserver_entry = NULL;
	gtk_widget_destroyed(widget, widget_pointer);
}

static void connect_name_change_cb(G_GNUC_UNUSED gpointer ns)
{
	gchar *name = notifying_string_get(requested_name);
	if (name_entry != NULL)
		gtk_entry_set_text(GTK_ENTRY(name_entry), name);
	g_free(name);
}

/* Public functions */
gboolean connect_get_spectator(void)
{
	return connect_spectator && connect_spectator_allowed;
}

void connect_set_spectator(gboolean spectator)
{
	connect_spectator = spectator;
	connect_spectator_allowed = TRUE;
	if (spectator_toggle != NULL)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
					     (spectator_toggle),
					     connect_spectator);
}

const gchar *connect_get_server(void)
{
	return connect_server;
}

void connect_set_server(const gchar * server)
{
	connect_set_field(&connect_server, server);
}

static gchar *connect_get_metaserver(void)
{
	if (metaserver_entry == NULL)
		return NULL;
	return metaserver_get(METASERVER(metaserver_entry));
}

void connect_set_metaserver(const gchar * metaserver)
{
	connect_set_field(&metaserver_info.server, metaserver);
	if (metaserver_entry != NULL)
		metaserver_add(METASERVER(metaserver_entry),
			       metaserver_info.server);
}

const gchar *connect_get_port(void)
{
	return connect_port;
}

void connect_set_port(const gchar * port)
{
	connect_set_field(&connect_port, port);
}

static void connect_close_all(gboolean user_pressed_ok,
			      gboolean can_be_spectator)
{
	connect_spectator_allowed = can_be_spectator;
	if (user_pressed_ok) {
		gchar *metaserver;

		notifying_string_set(requested_name,
				     gtk_entry_get_text(GTK_ENTRY
							(name_entry)));
		connect_spectator =
		    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
						 (spectator_toggle));
		/* Save connect dialogue entries */
		metaserver = connect_get_metaserver();
		config_set_string("connect/metaserver", metaserver);
		g_free(metaserver);

		frontend_gui_register_destroy(connect_dlg,
					      GUI_CONNECT_TRY);
	} else {
		frontend_gui_register_destroy(connect_dlg,
					      GUI_CONNECT_CANCEL);
	}
	if (connect_dlg)
		gtk_widget_destroy(GTK_WIDGET(connect_dlg));
	if (meta_dlg)
		gtk_widget_destroy(GTK_WIDGET(meta_dlg));
	if (cserver_dlg)
		gtk_widget_destroy(GTK_WIDGET(cserver_dlg));
	if (connect_private_dlg)
		gtk_widget_destroy(GTK_WIDGET(connect_private_dlg));
}

/* Messages explaining some delays */
static void show_waiting_box(const gchar * message, const gchar * server,
			     const gchar * port)
{
	if (meta_dlg) {
		gchar *s = g_strdup_printf(_("Metaserver at %s, port %s"),
					   server, port);
		gtk_label_set_text(GTK_LABEL(server_status), s);
		g_free(s);
	}
	log_message(MSG_INFO, message);
}

static void close_waiting_box(void)
{
	log_message(MSG_INFO, _("Finished.\n"));
}

/* -------------------- get game types -------------------- */

static void meta_gametype_notify(Session * ses, NetEvent event,
				 const gchar * line,
				 G_GNUC_UNUSED gpointer user_data)
{
	switch (event) {
	case NET_CONNECT:
		metaserver_info.num_available_titles = 0u;
		break;
	case NET_CONNECT_FAIL:
		log_message(MSG_ERROR,
			    _("The metaserver is not available "
			      "anymore.\n"));
		net_free(&ses);
		metaserver_info.session = NULL;
		if (cserver_dlg != NULL) {
			gtk_widget_destroy(GTK_WIDGET(cserver_dlg));
		}
		break;
	case NET_CLOSE:
		if (gametype_mode == GAMETYPE_MODE_SIGNON) {
			log_message(MSG_ERROR,
				    _("The metaserver closed "
				      "the connection "
				      "unexpectedly.\n"));
			if (cserver_dlg != NULL) {
				gtk_widget_destroy(GTK_WIDGET
						   (cserver_dlg));
			}
		} else {
			close_waiting_box();
		}
		if (cserver_dlg != NULL
		    && metaserver_info.num_available_titles > 0u) {
			gtk_dialog_set_response_sensitive(GTK_DIALOG
							  (cserver_dlg),
							  GTK_RESPONSE_OK,
							  TRUE);
		}
		net_free(&ses);
		metaserver_info.session = NULL;
		break;
	case NET_READ:
		switch (gametype_mode) {
		case GAMETYPE_MODE_SIGNON:
			net_printf(ses, "listtypes\n");
			gametype_mode = GAMETYPE_MODE_LIST;
			break;
		case GAMETYPE_MODE_LIST:
			/* A server description looks like this:
			 * title=%s\n
			 */
			if (strncmp(line, "title=", 6) == 0) {
				const GameParams *params;

				line += 6;
				if (game_list_is_empty()) {
					game_list_prepare();
				}
				params = game_list_find_item(line);
				if (params != NULL) {
					select_game_add_details(SELECTGAME
								(select_game),
								params);
				} else {
					select_game_add(SELECTGAME
							(select_game),
							line);
				}
				metaserver_info.num_available_titles++;
			}
			break;
		}
		break;
	}
}

static void get_metaserver_games_types(const gchar * server,
				       const gchar * port)
{
	show_waiting_box(_("Receiving game names from the metaserver.\n"),
			 server, port);
	g_assert(metaserver_info.session == NULL);
	metaserver_info.session = net_new(meta_gametype_notify, NULL);
	if (net_connect(metaserver_info.session, server, port))
		gametype_mode = GAMETYPE_MODE_SIGNON;
	else {
		net_free(&metaserver_info.session);
		log_message(MSG_ERROR,
			    _("The metaserver is not available "
			      "anymore.\n"));
		if (cserver_dlg != NULL) {
			gtk_widget_destroy(GTK_WIDGET(cserver_dlg));
		}
	}
}

/* -------------------- create game server -------------------- */

static void meta_create_notify(Session * ses, NetEvent event,
			       const gchar * line,
			       G_GNUC_UNUSED gpointer user_data)
{
	switch (event) {
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL:
		log_message(MSG_ERROR,
			    _("The metaserver is not available "
			      "anymore.\n"));
		net_free(&ses);
		metaserver_info.session = NULL;
		if (cserver_dlg != NULL) {
			gtk_widget_destroy(GTK_WIDGET(cserver_dlg));
		}
		break;
	case NET_CLOSE:
		net_free(&ses);
		metaserver_info.session = NULL;
		switch (create_mode) {
		case CREATE_MODE_STARTING_OK:
			log_message(MSG_INFO,
				    _("New game server requested "
				      "on %s port %s.\n"),
				    connect_server, connect_port);
			/* The metaserver is now busy creating the new game.
			 * UGLY FIX: Wait for some time */
			g_usleep(500000);
			connect_close_all(TRUE, FALSE);
			break;
		case CREATE_MODE_STARTING_ERROR:
			log_message(MSG_ERROR,
				    _("Incomplete information about the "
				      "new game server received.\n"));
			if (cserver_dlg != NULL) {
				gtk_dialog_set_response_sensitive
				    (GTK_DIALOG(cserver_dlg),
				     GTK_RESPONSE_OK, TRUE);
			}
			break;
		case CREATE_MODE_SIGNON:
		case CREATE_MODE_WAIT_FOR_INFO:
			log_message(MSG_ERROR,
				    _("The metaserver closed "
				      "the connection "
				      "unexpectedly.\n"));
			if (cserver_dlg != NULL) {
				gtk_dialog_set_response_sensitive
				    (GTK_DIALOG(cserver_dlg),
				     GTK_RESPONSE_OK, TRUE);
			}
			break;
		}
		break;
	case NET_READ:
		switch (create_mode) {
		case CREATE_MODE_SIGNON:
			net_printf(ses,
				   "create %d %d %d %d %d %s\n",
				   cfg_terrain, cfg_num_players,
				   cfg_victory_points, cfg_sevens_rule,
				   cfg_ai_players, cfg_gametype);
			create_mode = CREATE_MODE_WAIT_FOR_INFO;
			connect_set_field(&connect_server, NULL);
			connect_set_field(&connect_port, NULL);
			break;

		case CREATE_MODE_WAIT_FOR_INFO:
			if (strncmp(line, "host=", 5) == 0)
				connect_set_field(&connect_server,
						  line + 5);
			else if (strncmp(line, "port=", 5) == 0)
				connect_set_field(&connect_port, line + 5);
			else if (strcmp(line, "started") == 0) {
				if (connect_get_server() != NULL
				    && connect_get_port() != NULL) {
					create_mode =
					    CREATE_MODE_STARTING_OK;
				} else {
					create_mode =
					    CREATE_MODE_STARTING_ERROR;
				}
				net_close(ses);
				metaserver_info.session = NULL;
			} else
				log_message(MSG_ERROR,
					    _("Unknown message from the "
					      "metaserver: %s\n"), line);
			break;
		case CREATE_MODE_STARTING_OK:
		case CREATE_MODE_STARTING_ERROR:
			log_message(MSG_ERROR,
				    _("Unknown message from the "
				      "metaserver: %s\n"), line);
			break;
		}
	}
}

/* -------------------- get running servers info -------------------- */

static gboolean check_str_info(const gchar * line, const gchar * prefix,
			       gchar * data)
{
	size_t len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	strncpy(data, line + len, STRARG_LEN);
	return TRUE;
}

static gboolean check_int_info(const gchar * line, const gchar * prefix,
			       gchar * data)
{
	size_t len = strlen(prefix);

	if (strncmp(line, prefix, len) != 0)
		return FALSE;
	sprintf(data, "%d", atoi(line + len));
	return TRUE;
}

static void server_end(void)
{
	GtkTreeIter iter;

	if (meta_dlg) {
		gtk_list_store_append(meta_games_model, &iter);
		gtk_list_store_set(meta_games_model, &iter,
				   C_META_HOST, server_host,
				   C_META_PORT, server_port,
				   C_META_HOST_SORTABLE,
				   g_strdup_printf("%s:%s", server_host,
						   server_port),
				   C_META_VERSION, server_version,
				   C_META_MAX, server_max, C_META_CUR,
				   server_curr, C_META_TERRAIN,
				   server_terrain, C_META_VICTORY,
				   server_vpoints, C_META_SEVENS,
				   server_sevenrule, C_META_MAP,
				   server_title, -1);
	}
}

static void meta_free_session(Session * ses)
{
	if (ses == metaserver_info.session) {
		metaserver_info.session = NULL;
	}
	net_free(&ses);
}

/* Developer note: very similar code exists in server/meta.c
 * Keep both routines up-to-date
 */
static void meta_notify(Session * ses,
			NetEvent event, const gchar * line,
			G_GNUC_UNUSED gpointer user_data)
{
	gchar argument[STRARG_LEN];
	switch (event) {
	case NET_READ:
		if (ses != metaserver_info.session) {
			log_message(MSG_ERROR,
				    _("Receiving data from inactive "
				      "session: %s\n"), line);
			return;
		}
		switch (meta_mode) {
		case MODE_SIGNON:
		case MODE_REDIRECT:
			if (strncmp(line, "goto ", 5) == 0) {
				gchar **split_result;
				const gchar *port;
				if (metaserver_info.num_redirects++ >= 10) {
					log_message(MSG_ERROR,
						    _("Too many "
						      "metaserver "
						      "redirects.\n"));
					meta_mode = MODE_REDIRECT_OVERFLOW;
					net_close(ses);
					return;
				}
				meta_mode = MODE_REDIRECT;
				meta_free_session(ses);
				split_result = g_strsplit(line, " ", 0);
				g_assert(split_result[0] != NULL);
				g_assert(!strcmp(split_result[0], "goto"));
				if (split_result[1]) {
					g_free(metaserver_info.server);
					g_free(metaserver_info.port);

					metaserver_info.server =
					    g_strdup(split_result[1]);

					port = PIONEERS_DEFAULT_META_PORT;
					if (split_result[2])
						port = split_result[2];
					metaserver_info.port =
					    g_strdup(port);
					query_metaserver
					    (metaserver_info.server,
					     metaserver_info.port);
				} else {
					log_message(MSG_ERROR,
						    _("Bad redirect line: "
						      "%s\n"), line);
				};
				g_strfreev(split_result);
				break;
			}

			metaserver_info.version_major = 0;
			metaserver_info.version_minor = 0;
			metaserver_info.can_create_games = FALSE;
			if (strncmp(line, "welcome ", 8) == 0) {
				char *p = strstr(line, "version ");
				if (p) {
					p += 8;
					metaserver_info.version_major =
					    atoi(p);
					p += strspn(p, "0123456789");
					if (*p == '.')
						metaserver_info.
						    version_minor =
						    atoi(p + 1);
				}
			}
			if (metaserver_info.version_major < 1) {
				log_message(MSG_INFO,
					    _("The metaserver is too old "
					      "to create servers "
					      "(version %d.%d)\n"),
					    metaserver_info.version_major,
					    metaserver_info.version_minor);
			} else {
				net_printf(ses, "version %s\n",
					   META_PROTOCOL_VERSION);
			}

			if ((metaserver_info.version_major > 1) ||
			    (metaserver_info.version_major == 1
			     && metaserver_info.version_minor >= 1)) {
				net_printf(ses, "capability\n");
				meta_mode = MODE_CAPABILITY;
			} else {
				net_printf(ses,
					   metaserver_info.version_major >=
					   1 ? "listservers\n" :
					   "client\n");
				meta_mode = MODE_LIST;
			}
			break;
		case MODE_CAPABILITY:
			if (!strcmp(line, "create games")) {
				metaserver_info.can_create_games = TRUE;
			} else if (!strcmp(line, "end")) {
				net_printf(ses,
					   metaserver_info.version_major >=
					   1 ? "listservers\n" :
					   "client\n");
				meta_mode = MODE_LIST;
			}
			break;
		case MODE_LIST:
			if (strcmp(line, "server") == 0) {
				meta_mode = MODE_SERVER_INFO;
			} else {
				log_message(MSG_ERROR,
					    _("Unexpected data from the "
					      "metaserver: %s\n"), line);
			}
			break;
		case MODE_SERVER_INFO:
			if (strcmp(line, "end") == 0) {
				server_end();
				meta_mode = MODE_LIST;
			} else
			    if (check_str_info(line, "host=", server_host))
				break;
			else if (check_str_info
				 (line, "port=", server_port))
				break;
			else if (check_str_info
				 (line, "version=", server_version))
				break;
			else if (check_int_info(line, "max=", server_max))
				break;
			else if (check_int_info
				 (line, "curr=", server_curr))
				break;
			else if (check_str_info
				 (line, "vpoints=", server_vpoints))
				break;
			else if (check_str_info
				 (line, "sevenrule=", argument)) {
				if (server_sevenrule)
					g_free(server_sevenrule);
				if (!strcmp(argument, "normal")) {
					server_sevenrule =
					    g_strdup(_("Normal"));
				} else
				    if (!strcmp
					(argument, "reroll first 2")) {
					server_sevenrule =
					    g_strdup(_(""
						       "Reroll on 1st 2 turns"));
				} else if (!strcmp(argument, "reroll all")) {
					server_sevenrule =
					    g_strdup(_("Reroll all 7s"));
				} else {
					g_warning("Unknown seven rule: %s",
						  argument);
					server_sevenrule =
					    g_strdup(argument);
				}
				break;
			} else if (check_str_info
				   (line, "terrain=", argument)) {
				if (server_terrain)
					g_free(server_terrain);
				if (!strcmp(argument, "default"))
					server_terrain =
					    g_strdup(_("Default"));
				else if (!strcmp(argument, "random"))
					server_terrain =
					    g_strdup(_("Random"));
				else {
					g_warning
					    ("Unknown terrain type: %s",
					     argument);
					server_terrain =
					    g_strdup(argument);
				}
				break;
			} else
			    if (check_str_info
				(line, "title=", server_title))
				break;
			/* meta-protocol 0 compat */
			else if (check_str_info
				 (line, "map=", server_terrain))
				break;
			else if (check_str_info
				 (line, "comment=", server_title))
				break;
			else
				log_message(MSG_ERROR,
					    _("Unexpected data from the "
					      "metaserver: %s\n"), line);
			break;
		case MODE_DONE:
		case MODE_REDIRECT_OVERFLOW:
			log_message(MSG_ERROR,
				    _("Unexpected data from the "
				      "metaserver: %s\n"), line);
			break;
		}
		break;
	case NET_CLOSE:
		/* During a reconnect, different sessions might co-exist */
		if (ses == metaserver_info.session) {
			metaserver_info.session = NULL;
			switch (meta_mode) {
			case MODE_SIGNON:
			case MODE_REDIRECT:
			case MODE_SERVER_INFO:
			case MODE_CAPABILITY:
				log_message(MSG_ERROR,
					    _("The metaserver closed "
					      "the connection "
					      "unexpectedly.\n"));
				break;
			case MODE_REDIRECT_OVERFLOW:
				if (meta_dlg)
					gtk_widget_destroy(GTK_WIDGET(meta_dlg));	/* Close the dialog */
				if (ses == metaserver_info.session) {
					metaserver_info.session = NULL;
				}
				break;
			case MODE_LIST:
			case MODE_DONE:
				close_waiting_box();
				break;
			}
			if (meta_dlg) {
				gtk_dialog_set_response_sensitive
				    (GTK_DIALOG(meta_dlg),
				     META_RESPONSE_NEW,
				     metaserver_info.can_create_games);
				gtk_dialog_set_response_sensitive
				    (GTK_DIALOG(meta_dlg),
				     META_RESPONSE_REFRESH, TRUE);
			}
		}
		net_free(&ses);
		break;
	case NET_CONNECT:
		break;
	case NET_CONNECT_FAIL:
		/* Can't connect to the metaserver, don't show the GUI */
		if (meta_dlg)
			gtk_widget_destroy(GTK_WIDGET(meta_dlg));
		if (ses == metaserver_info.session) {
			metaserver_info.session = NULL;
		}
		net_free(&ses);
		break;
	}
}

static void query_metaserver(const gchar * server, const gchar * port)
{
	gchar *message;

	if (metaserver_info.num_redirects > 0) {
		if (strcmp(port, PIONEERS_DEFAULT_META_PORT) == 0) {
			message =
			    g_strdup_printf(_("Redirected to the "
					      "metaserver at %s.\n"),
					    server);
		} else {
			message =
			    g_strdup_printf(_("Redirected to the "
					      "metaserver at %s, port %s.\n"),
					    server, port);
		}
	} else
		message =
		    g_strdup(_
			     ("Receiving a list of Pioneers servers "
			      "from the metaserver.\n"));
	show_waiting_box(message, server, port);
	g_free(message);

	g_assert(metaserver_info.session == NULL);
	metaserver_info.session = net_new(meta_notify, NULL);
	if (net_connect(metaserver_info.session, server, port))
		meta_mode = MODE_SIGNON;
	else {
		net_free(&metaserver_info.session);
		close_waiting_box();
	}
}

/* -------------------- create server dialog -------------------- */

static void player_change_cb(GameSettings * gs,
			     G_GNUC_UNUSED gpointer user_data)
{
	guint players;
	guint ai_players;
	GtkSpinButton *ai_spin;

	ai_spin = GTK_SPIN_BUTTON(aiplayers_spin);
	players = game_settings_get_players(gs);
	ai_players = (guint) gtk_spin_button_get_value_as_int(ai_spin);
	gtk_spin_button_set_range(ai_spin, 0, players - 1);
	if (ai_players >= players)
		gtk_spin_button_set_value(ai_spin, players - 1);
}

static void game_select_cb(SelectGame * sg,
			   G_GNUC_UNUSED gpointer user_data)
{
	const GameParams *params;

	params = select_game_get_active_game(sg);

	if (params != NULL) {
		game_settings_set_players(GAMESETTINGS(game_settings),
					  params->num_players);
		game_settings_set_victory_points(GAMESETTINGS
						 (game_settings),
						 params->victory_points);
		game_rules_set_random_terrain(GAMERULES(game_rules),
					      params->random_terrain);
		game_rules_set_sevens_rule(GAMERULES(game_rules),
					   params->sevens_rule);

		player_change_cb(GAMESETTINGS(game_settings), NULL);
	}
}

static GtkWidget *build_create_interface(void)
{
	GtkWidget *vbox;
	GtkWidget *label;
	GtkAdjustment *adj;
	guint row;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);

	select_game = select_game_new();
	gtk_widget_show(select_game);
	gtk_box_pack_start(GTK_BOX(vbox), select_game, FALSE, FALSE, 3);

	g_signal_connect(G_OBJECT(select_game), "activate",
			 G_CALLBACK(game_select_cb), NULL);

	game_settings = game_settings_new(FALSE);
	gtk_widget_show(game_settings);
	gtk_box_pack_start(GTK_BOX(vbox), game_settings, FALSE, FALSE, 3);

	/* Dynamically adjust the maximum number of AI's */
	g_signal_connect(G_OBJECT(game_settings),
			 "change-players",
			 G_CALLBACK(player_change_cb), NULL);

	/* Add the number of computer players at the end of the grid */
	row = 99;
	/* Label */
	label = gtk_label_new(_("Number of computer players"));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(game_settings), label, 0, row, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);

	adj = GTK_ADJUSTMENT(gtk_adjustment_new(0,
						0,
						game_settings_get_players
						(GAMESETTINGS
						 (game_settings))
						- 1, 1, 4, 0));
	aiplayers_spin = gtk_spin_button_new(adj, 1, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(aiplayers_spin), TRUE);
	gtk_widget_show(aiplayers_spin);
	gtk_entry_set_alignment(GTK_ENTRY(aiplayers_spin), 1.0);
	gtk_grid_attach(GTK_GRID(game_settings), aiplayers_spin, 1, row, 1,
			1);
	gtk_widget_set_tooltip_text(aiplayers_spin,
				    /* Tooltip */
				    _("The number of computer players"));

	game_rules = game_rules_new_metaserver();
	gtk_widget_show(game_rules);
	gtk_box_pack_start(GTK_BOX(vbox), game_rules, FALSE, FALSE, 3);

	return vbox;
}

static void create_server_dlg_cb(GtkDialog * dlg, gint arg1,
				 G_GNUC_UNUSED gpointer user_data)
{
	GameSettings *gs = GAMESETTINGS(game_settings);
	SelectGame *sg = SELECTGAME(select_game);
	GameRules *gr = GAMERULES(game_rules);

	switch (arg1) {
	case GTK_RESPONSE_OK:
		gtk_dialog_set_response_sensitive(dlg, GTK_RESPONSE_OK,
						  FALSE);

		if (metaserver_info.session != NULL) {
			log_message(MSG_INFO, _("Canceled.\n"));
			net_close(metaserver_info.session);
		}
		log_message(MSG_INFO, _("Requesting new game server.\n"));

		cfg_terrain = game_rules_get_random_terrain(gr);
		cfg_num_players = game_settings_get_players(gs);
		cfg_victory_points = game_settings_get_victory_points(gs);
		cfg_sevens_rule = game_rules_get_sevens_rule(gr);
		cfg_ai_players = (guint)
		    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
						     (aiplayers_spin));
		cfg_gametype = select_game_get_active_title(sg);

		metaserver_info.session =
		    net_new(meta_create_notify, NULL);
		if (net_connect
		    (metaserver_info.session, metaserver_info.server,
		     metaserver_info.port)) {
			create_mode = CREATE_MODE_SIGNON;
		} else {
			log_message(MSG_ERROR,
				    _("The metaserver is not available "
				      "anymore.\n"));
			net_free(&metaserver_info.session);
			gtk_widget_destroy(GTK_WIDGET(dlg));
		}
		break;
	case GTK_RESPONSE_CANCEL:
	default:		/* For the compiler */
		gtk_widget_destroy(GTK_WIDGET(dlg));
		if (metaserver_info.session != NULL) {
			net_close(metaserver_info.session);
			/* Canceled retrieving information
			 * from the metaserver */
			log_message(MSG_INFO, _("Canceled.\n"));
		}
		break;
	};
}

/** Launch the server gtk. */
static void launch_server_gtk(G_GNUC_UNUSED GtkWidget * widget,
			      G_GNUC_UNUSED GtkWindow * parent)
{
	gchar *child_argv[3];
	GSpawnFlags flags = G_SPAWN_STDOUT_TO_DEV_NULL |
	    G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_SEARCH_PATH;
	GError *error = NULL;
	gint i;

	child_argv[0] = g_strdup(PIONEERS_SERVER_GTK_PROGRAM_NAME);
	child_argv[1] = g_strdup(PIONEERS_SERVER_GTK_PROGRAM_NAME);
	child_argv[2] = NULL;
	if (!g_spawn_async(NULL, child_argv, NULL, flags, NULL, NULL, NULL,
			   &error)) {
		/* Error message when program %1 is started, reason is %2 */
		log_message(MSG_ERROR,
			    _("Error starting %s: %s\n"),
			    child_argv[0], error->message);
		g_error_free(error);
	}
	for (i = 0; child_argv[i] != NULL; i++)
		g_free(child_argv[i]);
}

static void create_server_dlg(G_GNUC_UNUSED GtkWidget * widget,
			      GtkWindow * parent)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;

	if (cserver_dlg) {
		gtk_window_present(GTK_WINDOW(cserver_dlg));
		return;
	}
	set_metaserver_info();

	cserver_dlg =
	    /* Dialog caption */
	    gtk_dialog_new_with_buttons(_("Create a Public Game"), parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					/* Button text */
					_("_Cancel"), GTK_RESPONSE_CANCEL,
					/* Button text */
					_("C_reate"), GTK_RESPONSE_OK,
					NULL);
	g_signal_connect(G_OBJECT(cserver_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &cserver_dlg);
	gtk_widget_realize(cserver_dlg);

	dlg_vbox = gtk_dialog_get_content_area(GTK_DIALOG(cserver_dlg));
	gtk_widget_show(dlg_vbox);

	vbox = build_create_interface();
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	if (cserver_dlg != NULL) {
		gtk_dialog_set_response_sensitive(GTK_DIALOG(cserver_dlg),
						  GTK_RESPONSE_OK, FALSE);
	}
	g_signal_connect(G_OBJECT(cserver_dlg), "response",
			 G_CALLBACK(create_server_dlg_cb), NULL);
	gtk_widget_show(cserver_dlg);

	get_metaserver_games_types(metaserver_info.server,
				   metaserver_info.port);

}

/* -------------------- select server dialog -------------------- */

static gint meta_click_cb(G_GNUC_UNUSED GtkWidget * widget,
			  G_GNUC_UNUSED GdkEventButton * event,
			  G_GNUC_UNUSED gpointer user_data)
{
	if (event->type == GDK_2BUTTON_PRESS) {
		gtk_dialog_response(GTK_DIALOG(meta_dlg), GTK_RESPONSE_OK);
	};
	return FALSE;
}

static void meta_select_cb(G_GNUC_UNUSED GtkTreeSelection * selection,
			   G_GNUC_UNUSED gpointer user_data)
{
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg),
					  GTK_RESPONSE_OK, TRUE);
}

static void meta_dlg_cb(GtkDialog * dlg, gint arg1,
			G_GNUC_UNUSED gpointer userdata)
{
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	gchar *host;
	gchar *port;

	switch (arg1) {
	case META_RESPONSE_REFRESH:	/* Refresh the list */
		gtk_list_store_clear(meta_games_model);
		metaserver_info.num_redirects = 0;
		query_metaserver(metaserver_info.server,
				 metaserver_info.port);
		break;
	case META_RESPONSE_NEW:	/* Add a server */
		create_server_dlg(NULL, GTK_WINDOW(dlg));
		break;
	case GTK_RESPONSE_OK:	/* Select this server */
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(meta_games_view),
					 &path, &column);
		if (path != NULL) {
			gtk_tree_model_get_iter(GTK_TREE_MODEL
						(meta_games_model), &iter,
						path);
			gtk_tree_model_get(GTK_TREE_MODEL
					   (meta_games_model), &iter,
					   C_META_HOST, &host, C_META_PORT,
					   &port, -1);
			connect_set_field(&connect_server, host);
			connect_set_field(&connect_port, port);
			g_free(host);
			g_free(port);
			gtk_tree_path_free(path);
			connect_close_all(TRUE, TRUE);
		}
		break;
	case GTK_RESPONSE_CANCEL:	/* Cancel */
	default:
		gtk_widget_destroy(GTK_WIDGET(dlg));
		if (metaserver_info.session != NULL) {
			net_close(metaserver_info.session);
			/* Canceled retrieving information
			 * from the metaserver */
			log_message(MSG_INFO, _("Canceled.\n"));
		}
		break;
	}
}

static void set_metaserver_info(void)
{
	gchar *meta_tmp;

	g_free(metaserver_info.server);
	g_free(metaserver_info.port);

	meta_tmp = metaserver_get(METASERVER(metaserver_entry));
	metaserver_info.server = meta_tmp;	/* Take-over of the pointer */
	metaserver_info.port = g_strdup(PIONEERS_DEFAULT_META_PORT);
}

static void create_meta_dlg(G_GNUC_UNUSED GtkWidget * widget,
			    GtkWidget * parent)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *scroll_win;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	set_metaserver_info();

	if (meta_dlg != NULL) {
		if (metaserver_info.session == NULL) {
			gtk_list_store_clear(meta_games_model);
			metaserver_info.num_redirects = 0;
			metaserver_info.version_major = 0;
			metaserver_info.version_minor = 0;
			metaserver_info.can_create_games = FALSE;
			gtk_dialog_set_response_sensitive
			    (GTK_DIALOG(meta_dlg),
			     META_RESPONSE_NEW,
			     metaserver_info.can_create_games);

			query_metaserver(metaserver_info.server,
					 metaserver_info.port);
		}
		gtk_window_present(GTK_WINDOW(meta_dlg));
		return;
	}

	/* Dialog caption */
	meta_dlg = gtk_dialog_new_with_buttons(_("Join a Public Game"),
					       GTK_WINDOW(parent), 0,
					       /* Button text */
					       _("_Refresh"),
					       META_RESPONSE_REFRESH,
					       /* Button text */
					       _("_New Remote Game"),
					       META_RESPONSE_NEW,
					       /* Button text */
					       _("_Cancel"),
					       GTK_RESPONSE_CANCEL,
					       /* Button text */
					       _("_Join"),
					       GTK_RESPONSE_OK, NULL);
	g_signal_connect(G_OBJECT(meta_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &meta_dlg);
	g_signal_connect(G_OBJECT(meta_dlg), "response",
			 G_CALLBACK(meta_dlg_cb), NULL);
	gtk_widget_realize(meta_dlg);

	dlg_vbox = gtk_dialog_get_content_area(GTK_DIALOG(meta_dlg));
	gtk_widget_show(dlg_vbox);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	server_status = gtk_label_new("");
	gtk_widget_show(server_status);
	gtk_box_pack_start(GTK_BOX(vbox), server_status, FALSE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (scroll_win), GTK_SHADOW_IN);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);

	meta_games_model = gtk_list_store_new(META_N_COLUMNS,
					      G_TYPE_STRING, G_TYPE_STRING,
					      G_TYPE_STRING, G_TYPE_STRING,
					      G_TYPE_STRING, G_TYPE_STRING,
					      G_TYPE_STRING, G_TYPE_STRING,
					      G_TYPE_STRING,
					      G_TYPE_STRING);

	meta_games_view =
	    gtk_tree_view_new_with_model(GTK_TREE_MODEL(meta_games_model));
	gtk_widget_show(meta_games_view);
	gtk_container_add(GTK_CONTAINER(scroll_win), meta_games_view);
	gtk_widget_set_tooltip_text(meta_games_view,
				    /* Tooltip */
				    _("Select a game to join"));

	column =
	    /* Column name */
	    gtk_tree_view_column_new_with_attributes(_("Map Name"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_MAP, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column, C_META_MAP);
	/* Tooltip for column 'Map Name' */
	set_tooltip_on_column(column, _("Name of the game"));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0f, NULL);
	column =
	    /* Column name */
	    gtk_tree_view_column_new_with_attributes(_("Curr"), renderer,
						     "text", C_META_CUR,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column, C_META_CUR);
	/* Tooltip for column 'Curr' */
	set_tooltip_on_column(column, _("Number of players in the game"));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0f, NULL);
	column =
	    /* Column name */
	    gtk_tree_view_column_new_with_attributes(_("Max"), renderer,
						     "text", C_META_MAX,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column, C_META_MAX);
	/* Tooltip for column 'Max' */
	set_tooltip_on_column(column, _("Maximum players for the game"));

	column =
	    /* Column name */
	    gtk_tree_view_column_new_with_attributes(_("Terrain"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_TERRAIN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column, C_META_TERRAIN);
	/* Tooltip for column 'Terrain' */
	set_tooltip_on_column(column, _("Random of default terrain"));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0f, NULL);
	column =
	    /* Column name */
	    gtk_tree_view_column_new_with_attributes(_("Vic. Points"),
						     renderer, "text",
						     C_META_VICTORY, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column, C_META_VICTORY);
	/* Tooltip for column 'Vic. Points' */
	set_tooltip_on_column(column, _("Points needed to win"));

	column =
	    /* Column name */
	    gtk_tree_view_column_new_with_attributes(_("Sevens Rule"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_SEVENS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column, C_META_SEVENS);
	/* Tooltip for column 'Sevens Rule' */
	set_tooltip_on_column(column, _("Sevens rule"));

	column =
	    /* Column name */
	    gtk_tree_view_column_new_with_attributes(_("Host"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_HOST, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column,
						C_META_HOST_SORTABLE);
	/* Tooltip for column 'Host' */
	set_tooltip_on_column(column, _("Host of the game"));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0f, NULL);
	column =
	    /* Column name */
	    gtk_tree_view_column_new_with_attributes(_("Port"), renderer,
						     "text", C_META_PORT,
						     NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column,
						C_META_HOST_SORTABLE);
	/* Tooltip for column 'Port' */
	set_tooltip_on_column(column, _("Port of the game"));

	column =
	    /* Column name */
	    gtk_tree_view_column_new_with_attributes(_("Version"),
						     gtk_cell_renderer_text_new
						     (), "text",
						     C_META_VERSION, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(meta_games_view),
				    column);
	gtk_tree_view_column_set_sort_column_id(column, C_META_VERSION);
	/* Tooltip for column 'Version' */
	set_tooltip_on_column(column, _("Version of the host"));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE
					     (meta_games_model),
					     C_META_HOST_SORTABLE,
					     GTK_SORT_ASCENDING);

	/* Register double-click */
	g_signal_connect(G_OBJECT(meta_games_view), "button_press_event",
			 G_CALLBACK(meta_click_cb), NULL);

	g_signal_connect(G_OBJECT
			 (gtk_tree_view_get_selection
			  (GTK_TREE_VIEW(meta_games_view))), "changed",
			 G_CALLBACK(meta_select_cb), NULL);

	/* This button will be enabled when a game is selected */
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg),
					  GTK_RESPONSE_OK, FALSE);

	/* These buttons will be enabled when the metaserver has responded */
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg),
					  META_RESPONSE_NEW, FALSE);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(meta_dlg),
					  META_RESPONSE_REFRESH, FALSE);

	gtk_widget_show(meta_dlg);

	metaserver_info.num_redirects = 0;
	query_metaserver(metaserver_info.server, metaserver_info.port);

	/* Workaround: Set the size of the widget as late as possible, to avoid a strange display */
	gtk_widget_set_size_request(scroll_win, -1, 150);
}

static void connect_dlg_cb(G_GNUC_UNUSED GtkDialog * dlg,
			   G_GNUC_UNUSED gint arg1,
			   G_GNUC_UNUSED gpointer userdata)
{
	connect_close_all(FALSE, FALSE);
}

#ifdef HAVE_AVAHI
static void connect_avahi_cb(G_GNUC_UNUSED GtkWidget * widget,
			     GtkWidget * avahibrowser_entry)
{
	gchar *server =
	    avahibrowser_get_server(AVAHIBROWSER(avahibrowser_entry));
	gchar *port =
	    avahibrowser_get_port(AVAHIBROWSER(avahibrowser_entry));

	connect_set_field(&connect_server, server);
	connect_set_field(&connect_port, port);

	g_free(server);
	g_free(port);

	/* connect */
	connect_close_all(TRUE, TRUE);
}
#endif

void connect_create_dlg(void)
{
	GtkWidget *dlg_vbox;
	GtkWidget *grid;
	GtkWidget *lbl;
	GtkWidget *hbox;
	GtkWidget *btn;
	GtkWidget *sep;
#ifdef HAVE_AVAHI
	GtkWidget *avahibrowser_entry;
#endif				/* HAVE_AVAHI */
	gchar *fullname;
	gchar *name;
	guint row;

	if (connect_dlg) {
		gtk_window_present(GTK_WINDOW(connect_dlg));
		return;
	}

	g_signal_connect(requested_name, "changed",
			 G_CALLBACK(connect_name_change_cb), NULL);

	/* Dialog caption */
	connect_dlg = gtk_dialog_new_with_buttons(_("Start a New Game"),
						  GTK_WINDOW(app_window),
						  0,
						  /* Button text */
						  _("_Cancel"),
						  GTK_RESPONSE_CANCEL,
						  NULL);
	g_signal_connect(G_OBJECT(connect_dlg), "response",
			 G_CALLBACK(connect_dlg_cb), NULL);
	g_signal_connect(G_OBJECT(connect_dlg), "destroy",
			 G_CALLBACK(connect_dlg_destroyed), &connect_dlg);

	gtk_widget_realize(connect_dlg);
	gdk_window_set_functions(gtk_widget_get_window(connect_dlg),
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox = gtk_dialog_get_content_area(GTK_DIALOG(connect_dlg));
	gtk_widget_show(dlg_vbox);

	grid = gtk_grid_new();
	row = 0;
	gtk_widget_show(grid);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), grid, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 5);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	/* Label */
	lbl = gtk_label_new(_("Player name"));
	gtk_widget_show(lbl);
	gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

	name = notifying_string_get(requested_name);
	name_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(name_entry), MAX_NAME_LENGTH);
	gtk_widget_show(name_entry);
	gtk_entry_set_text(GTK_ENTRY(name_entry), name);
	g_free(name);

	gtk_grid_attach(GTK_GRID(grid), name_entry, 1, row, 1, 1);
	/* Tooltip */
	gtk_widget_set_tooltip_text(name_entry, _("Enter your name"));

	/* Check button */
	spectator_toggle = gtk_check_button_new_with_label(_("Spectator"));
	gtk_widget_show(spectator_toggle);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(spectator_toggle),
				     connect_spectator);

	gtk_grid_attach(GTK_GRID(grid), spectator_toggle, 2, row, 1, 1);
	gtk_widget_set_tooltip_text(spectator_toggle,
				    /* Tooltip for checkbox Spectator */
				    _
				    ("Check if you want to be a spectator"));
	row++;

	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(sep);
	gtk_grid_attach(GTK_GRID(grid), sep, 0, row, 3, 1);
	row++;

#ifdef HAVE_AVAHI
	/* Label */
	lbl = gtk_label_new(_("Avahi"));
	gtk_widget_show(lbl);
	gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

	/* Button */
	btn = gtk_button_new_with_label(_("Join"));
	gtk_widget_show(btn);
	gtk_widget_set_tooltip_text(btn,
				    /* Tooltip for button Join */
				    _(""
				      "Join an automatically discovered game"));
	gtk_grid_attach(GTK_GRID(grid), btn, 2, row, 1, 1);

	avahibrowser_entry = avahibrowser_new(btn);
	gtk_widget_show(avahibrowser_entry);
	gtk_grid_attach(GTK_GRID(grid), avahibrowser_entry, 1, row, 1, 1);
	row++;

	g_signal_connect(G_OBJECT(btn), "clicked",
			 G_CALLBACK(connect_avahi_cb), avahibrowser_entry);

	/* enable avahi */
	/* storing the pointer to this widget for later use */
	avahi_register(AVAHIBROWSER(avahibrowser_entry));

	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(sep);
	gtk_grid_attach(GTK_GRID(grid), sep, 0, row, 3, 1);
	row++;
#endif				/* HAVE_AVAHI */

	/* Label */
	lbl = gtk_label_new(_("Metaserver"));
	gtk_widget_show(lbl);
	gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

	metaserver_entry = metaserver_new();
	gtk_widget_show(metaserver_entry);
	gtk_grid_attach(GTK_GRID(grid), metaserver_entry, 1, row, 2, 1);
	metaserver_add(METASERVER(metaserver_entry),
		       metaserver_info.server);
	row++;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_widget_show(hbox);
	gtk_grid_attach(GTK_GRID(grid), hbox, 0, row, 3, 1);
	row++;

	/* Button */
	btn = gtk_button_new_with_label(_("Join Public Game"));
	gtk_widget_show(btn);
	gtk_box_pack_start(GTK_BOX(hbox), btn, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(btn), "clicked",
			 G_CALLBACK(create_meta_dlg), app_window);
	/* Tooltip */
	gtk_widget_set_tooltip_text(btn, _("Join a public game"));
	gtk_widget_set_can_default(btn, TRUE);
	gtk_widget_grab_default(btn);

	/* Button */
	btn = gtk_button_new_with_label(_("Create Game"));
	gtk_widget_show(btn);
	gtk_box_pack_start(GTK_BOX(hbox), btn, TRUE, TRUE, 0);
	/* Tooltip */
	gtk_widget_set_tooltip_text(btn, _("Create a game"));
	g_signal_connect(G_OBJECT(btn), "clicked",
			 G_CALLBACK(launch_server_gtk), app_window);
	fullname =
	    g_find_program_in_path(PIONEERS_SERVER_GTK_PROGRAM_NAME);
	gtk_widget_set_sensitive(GTK_WIDGET(btn), fullname != NULL);
	g_free(fullname);

	/* Button */
	btn = gtk_button_new_with_label(_("Join Private Game"));
	gtk_widget_show(btn);
	gtk_box_pack_start(GTK_BOX(hbox), btn, TRUE, TRUE, 0);
	/* Tooltip */
	gtk_widget_set_tooltip_text(btn, _("Join a private game"));
	g_signal_connect(G_OBJECT(btn), "clicked",
			 G_CALLBACK(connect_private_dialog), app_window);

	gtk_entry_set_activates_default(GTK_ENTRY(name_entry), TRUE);

	gtk_widget_show(connect_dlg);

	gtk_widget_grab_focus(name_entry);
}

/* ------------ Join a private game dialog ------------------- */

static void update_recent_servers_list(void)
{
	gchar keyname1[50], keyname2[50];
	gchar *temp_name, *temp_port;
	gchar *cur_name, *cur_port;
	gchar *conn_name, *conn_port;
	gboolean default_used;
	gboolean done;
	gint i;

	done = FALSE;
	i = 0;

	conn_name = g_strdup(connect_get_server());
	conn_port = g_strdup(connect_get_port());

	temp_name = g_strdup(conn_name);
	temp_port = g_strdup(conn_port);

	do {
		sprintf(keyname1, "favorites/server%dname=", i);
		sprintf(keyname2, "favorites/server%dport=", i);
		cur_name =
		    g_strdup(config_get_string(keyname1, &default_used));
		cur_port =
		    g_strdup(config_get_string(keyname2, &default_used));

		if (temp_name) {
			sprintf(keyname1, "favorites/server%dname", i);
			sprintf(keyname2, "favorites/server%dport", i);
			config_set_string(keyname1, temp_name);
			config_set_string(keyname2, temp_port);
		} else {
			g_free(cur_name);
			g_free(cur_port);
			break;
		}

		if (strlen(cur_name) == 0) {
			g_free(cur_name);
			g_free(cur_port);
			break;
		}

		g_free(temp_name);
		g_free(temp_port);
		if (!strcmp(cur_name, conn_name)
		    && !strcmp(cur_port, conn_port)) {
			temp_name = NULL;
			temp_port = NULL;
		} else {
			temp_name = g_strdup(cur_name);
			temp_port = g_strdup(cur_port);
		}

		i++;
		if (i > PRIVATE_GAME_HISTORY_SIZE) {
			done = TRUE;
		}
		g_free(cur_name);
		g_free(cur_port);
	} while (!done);
	g_free(temp_name);
	g_free(temp_port);
	g_free(conn_name);
	g_free(conn_port);
}

static void host_list_select_cb(GtkWidget * widget, gpointer user_data)
{
	GPtrArray *host_entries = user_data;
	gint idx;
	gchar *entry;
	gchar **strs;

	idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	entry = g_ptr_array_index(host_entries, idx);
	strs = g_strsplit(entry, ":", 2);

	connect_set_field(&connect_server, strs[0]);
	connect_set_field(&connect_port, strs[1]);
	gtk_entry_set_text(GTK_ENTRY(host_entry), connect_server);
	gtk_entry_set_text(GTK_ENTRY(port_entry), connect_port);
	g_strfreev(strs);
}


static void connect_private_dlg_cb(GtkDialog * dlg, gint arg1,
				   G_GNUC_UNUSED gpointer user_data)
{
	switch (arg1) {
	case GTK_RESPONSE_OK:
		connect_set_field(&connect_server,
				  gtk_entry_get_text(GTK_ENTRY
						     (host_entry)));
		connect_set_field(&connect_port,
				  gtk_entry_get_text(GTK_ENTRY
						     (port_entry)));
		update_recent_servers_list();
		config_set_string("connect/server", connect_server);
		config_set_string("connect/port", connect_port);
		connect_close_all(TRUE, TRUE);
		break;
	case GTK_RESPONSE_CANCEL:
	default:		/* For the compiler */
		gtk_widget_destroy(GTK_WIDGET(dlg));
		break;
	};
}

static void connect_private_dialog(G_GNUC_UNUSED GtkWidget * widget,
				   GtkWindow * parent)
{
	GtkWidget *dlg_vbox;
	GtkWidget *grid;
	GtkWidget *lbl;
	GtkWidget *hbox;

	GtkWidget *host_list;
	GPtrArray *host_entries;

	gint i;
	gchar *host_name, *host_port, *host_name_port, temp_str[50];
	gboolean default_returned;

	if (connect_private_dlg) {
		gtk_window_present(GTK_WINDOW(connect_private_dlg));
		return;
	}

	connect_private_dlg = gtk_dialog_new_with_buttons(
								 /* Dialog caption */
								 _(""
								   "Join a private game"),
								 GTK_WINDOW
								 (parent),
								 0,
								 /* Button text */
								 _
								 ("_Cancel"),
								 GTK_RESPONSE_CANCEL,
								 /* Button text */
								 _
								 ("_Join"),
								 GTK_RESPONSE_OK,
								 NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(connect_private_dlg),
					GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(connect_private_dlg), "response",
			 G_CALLBACK(connect_private_dlg_cb), NULL);
	g_signal_connect(G_OBJECT(connect_private_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed),
			 &connect_private_dlg);


	gtk_widget_realize(connect_private_dlg);
	gdk_window_set_functions(gtk_widget_get_window
				 (connect_private_dlg),
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox =
	    gtk_dialog_get_content_area(GTK_DIALOG(connect_private_dlg));
	gtk_widget_show(dlg_vbox);

	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), grid, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 5);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	/* Label */
	lbl = gtk_label_new(_("Server host"));
	gtk_widget_show(lbl);
	gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

	host_entry = gtk_entry_new();
	gtk_widget_show(host_entry);
	gtk_grid_attach(GTK_GRID(grid), host_entry, 1, 0, 1, 1);
	gtk_entry_set_text(GTK_ENTRY(host_entry), connect_server);
	gtk_widget_set_tooltip_text(host_entry,
				    /* Tooltip */
				    _("Name of the host of the game"));
	connect_set_field(&connect_server, connect_server);

	/* Label */
	lbl = gtk_label_new(_("Server port"));
	gtk_widget_show(lbl);
	gtk_grid_attach(GTK_GRID(grid), lbl, 0, 1, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_grid_attach(GTK_GRID(grid), hbox, 1, 1, 1, 1);

	port_entry = gtk_entry_new();
	gtk_widget_show(port_entry);
	gtk_box_pack_start(GTK_BOX(hbox), port_entry, FALSE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(port_entry), connect_port);
	gtk_widget_set_tooltip_text(port_entry,
				    /* Tooltip */
				    _("Port of the host of the game"));
	connect_set_field(&connect_port, connect_port);

	host_list = gtk_combo_box_text_new();
	host_entries = g_ptr_array_new();

	gtk_widget_show(host_list);

	for (i = 0; i < PRIVATE_GAME_HISTORY_SIZE; i++) {
		sprintf(temp_str, "favorites/server%dname=", i);
		host_name = config_get_string(temp_str, &default_returned);
		if (default_returned || !strlen(host_name)) {
			g_free(host_name);
			break;
		}

		sprintf(temp_str, "favorites/server%dport=", i);
		host_port = config_get_string(temp_str, &default_returned);
		if (default_returned || !strlen(host_port)) {
			g_free(host_name);
			g_free(host_port);
			break;
		}

		host_name_port =
		    g_strconcat(host_name, ":", host_port, NULL);
		g_free(host_name);
		g_free(host_port);

		g_ptr_array_add(host_entries, host_name_port);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
					       (host_list),
					       host_name_port);
	}
	if (i > 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(host_list), 0);
	g_signal_connect(G_OBJECT(host_list), "changed",
			 G_CALLBACK(host_list_select_cb), host_entries);

	gtk_grid_attach(GTK_GRID(grid), host_list, 1, 2, 1, 1);
	/* Tooltip */
	gtk_widget_set_tooltip_text(host_list, _("Recent games"));

	/* Label */
	lbl = gtk_label_new(_("Recent games"));
	gtk_widget_show(lbl);
	gtk_grid_attach(GTK_GRID(grid), lbl, 0, 2, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

	gtk_entry_set_activates_default(GTK_ENTRY(host_entry), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(port_entry), TRUE);

	gtk_widget_show(connect_private_dlg);
}
