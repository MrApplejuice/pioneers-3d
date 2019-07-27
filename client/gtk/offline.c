/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003,2006 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2004,2006 Roland Clobus <rclobus@bigfoot.com>
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
#include "frontend.h"
#include "common_gtk.h"
#include "config-gnome.h"
#include "theme.h"
#include "histogram.h"
#include "version.h"
#include "notification.h"
#include "client.h"
#include "game-list.h"

static gboolean have_dlg = FALSE;
static gboolean connectable = FALSE;

static const gchar *name = NULL;
static gboolean spectator = FALSE;
static const gchar *server = NULL;
static const gchar *port = NULL;
static const gchar *metaserver = NULL;
static gboolean server_from_commandline = FALSE;
static gboolean quit_when_offline = FALSE;
static gboolean mainloop_started = FALSE;
static gboolean enable_debug = FALSE;
static gboolean show_version = FALSE;

static GOptionEntry commandline_entries[] = {
	/* Commandline option of client: hostname of the server */
	{"server", 's', 0, G_OPTION_ARG_STRING, &server, N_("Server host"),
	 PIONEERS_DEFAULT_GAME_HOST},
	/* Commandline option of client: port of the server */
	{"port", 'p', 0, G_OPTION_ARG_STRING, &port, N_("Server port"),
	 PIONEERS_DEFAULT_GAME_PORT},
	/* Commandline option of client: name of the player */
	{"name", 'n', 0, G_OPTION_ARG_STRING, &name, N_("Player name"),
	 NULL},
	/* Commandline option of client: do we want to be a spectator */
	{"spectator", 'v', 0, G_OPTION_ARG_NONE, &spectator,
	 N_("Connect as a spectator"), NULL},
	/* Commandline option of client: hostname of the metaserver */
	{"metaserver", 'm', 0, G_OPTION_ARG_STRING, &metaserver,
	 N_("Metaserver Host"), PIONEERS_DEFAULT_METASERVER},
	{"debug", '\0', 0, G_OPTION_ARG_NONE, &enable_debug,
	 /* Commandline option of client: enable debug logging */
	 N_("Enable debug messages"), NULL},
	{"version", '\0', 0, G_OPTION_ARG_NONE, &show_version,
	 /* Commandline option of client: version */
	 N_("Show version information"), NULL},
	{NULL, '\0', 0, 0, NULL, NULL, NULL}
};

static void frontend_offline_start_connect_cb(void)
{
	connect_create_dlg();
	have_dlg = TRUE;
	gui_set_instructions(_("Select a game to join."));
	frontend_gui_update();
}

/* gui function to handle gui events in offline mode */
static void frontend_offline_gui(GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_CONNECT_TRY, TRUE);
		frontend_gui_check(GUI_CONNECT, !have_dlg && connectable);
		frontend_gui_check(GUI_DISCONNECT, !have_dlg
				   && !connectable);
		break;
	case GUI_CONNECT_TRY:
		gui_show_splash_page(FALSE, NULL);
		gui_set_net_status(_("Connecting"));

		connectable = FALSE;
		have_dlg = FALSE;
		cb_connect(connect_get_server(), connect_get_port(),
			   connect_get_spectator());
		frontend_gui_update();
		break;
	case GUI_CONNECT:
		frontend_offline_start_connect_cb();
		break;
	case GUI_CONNECT_CANCEL:
		have_dlg = FALSE;
		gui_set_instructions(_("Welcome to Pioneers!"));
		frontend_gui_update();
		break;
	default:
		break;
	}
}

void frontend_disconnect(void)
{
	quit_when_offline = FALSE;
	cb_disconnect();
}

/* this function is called when offline mode is entered. */
void frontend_offline(void)
{
	connectable = TRUE;

	if (have_dlg)
		return;

	gui_cursor_none();	/* Clear possible cursor */
	frontend_discard_done();
	frontend_gold_done();

	/* set the callback for gui events */
	set_gui_state(frontend_offline_gui);

	if (quit_when_offline) {
		route_gui_event(GUI_QUIT);
	}

	/* Commandline overrides the dialog */
	if (server_from_commandline) {
		server_from_commandline = FALSE;
		quit_when_offline = TRUE;
		route_gui_event(GUI_CONNECT_TRY);
	} else {
		frontend_offline_start_connect_cb();
	}
}

static void frontend_main(void)
{
	mainloop_started = TRUE;
	gtk_main();
	notification_cleanup();
	themes_cleanup();
	game_list_cleanup();
}

void frontend_quit(void)
{
	if (mainloop_started) {
		gtk_main_quit();
	} else {
		/* The main loop did not start yet.
		 * Do not quit, so the user can read the log */
		quit_when_offline = FALSE;
	}
}

/* this function is called to let the frontend initialize itself. */
void frontend_init_gtk_et_al(int argc, char **argv)
{
	GOptionContext *context;
	GError *error = NULL;

	frontend_gui_register_init();

	set_ui_driver(&GTK_Driver);

	config_init("pioneers");

	/* Long description in the commandline for pioneers: help */
	context = g_option_context_new(_("- Play a game of Pioneers"));
	g_option_context_add_main_entries(context, commandline_entries,
					  PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_parse(context, &argc, &argv, &error);
	g_option_context_free(context);

	if (error != NULL) {
		g_print("%s\n", error->message);
		g_error_free(error);
		exit(1);
	}
	if (show_version) {
		g_print(_("Pioneers version:"));
		g_print(" ");
		g_print(FULL_VERSION);
		g_print("\n");
		exit(0);
	}
#if ENABLE_NLS
	/* Gtk+ handles the locale, we must bind the translations */
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif
	/* Name of the application */
	g_set_application_name(_("Pioneers"));

	callbacks.mainloop = &frontend_main;
	callbacks.quit = &frontend_quit;
}

static void frontend_change_name_cb(NotifyingString * name)
{
	gchar *nm = notifying_string_get(name);
	config_set_string("connect/name", nm);
	if (callback_mode != MODE_INIT) {
		cb_name_change(nm);
	}
	g_free(nm);
}

static void frontend_change_style_cb(NotifyingString * style)
{
	gchar *st = notifying_string_get(style);
	config_set_string("connect/style", st);
	if (callback_mode != MODE_INIT) {
		cb_style_change(st);
	}
	g_free(st);
}

/* this function is called to let the frontend initialize itself. */
void frontend_init(void)
{
	gboolean default_returned;
	gchar *style;

	set_enable_debug(enable_debug);

	/* save the new settings when changed */
	g_signal_connect(requested_name, "changed",
			 G_CALLBACK(frontend_change_name_cb), NULL);
	g_signal_connect(requested_style, "changed",
			 G_CALLBACK(frontend_change_style_cb), NULL);

	/* Create the application window
	 */
	themes_init();
	settings_init();
	histogram_init();
	notification_init();
	gui_build_interface();

	/* in theory, all windows are created now...
	 *   set logging to message window */
	log_set_func_message_window();

	if (!name) {
		name =
		    config_get_string("connect/name", &default_returned);
		if (default_returned) {
			name = g_strdup(g_get_user_name());
		}
		/* If --spectator is used, we are now a spectator.  If not, get the
		 * correct value from the config file.
		 * To allow specifying "don't be a spectator", only check the
		 * config file if --name is not used.  */
		if (!spectator) {
			spectator =
			    config_get_int_with_default
			    ("connect/spectator", 0) ? TRUE : FALSE;
		}
	}
	style = config_get_string("connect/style", &default_returned);
	if (default_returned) {
		style = g_strdup(default_player_style);
	}

	notifying_string_set(requested_name, name);
	connect_set_spectator(spectator);
	notifying_string_set(requested_style, style);
	g_free(style);

	if (server && port) {
		server_from_commandline = TRUE;
	} else {
		if (server || port)
			g_warning("Only server or port set, "
				  "ignoring command line");
		server_from_commandline = FALSE;
		server = config_get_string("connect/server="
					   PIONEERS_DEFAULT_GAME_HOST,
					   &default_returned);
		port = config_get_string("connect/port="
					 PIONEERS_DEFAULT_GAME_PORT,
					 &default_returned);
	}
	connect_set_server(server);
	connect_set_port(port);

	if (!metaserver)
		metaserver = config_get_string("connect/metaserver="
					       PIONEERS_DEFAULT_METASERVER,
					       &default_returned);
	connect_set_metaserver(metaserver);
}
