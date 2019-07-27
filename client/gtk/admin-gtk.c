/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
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

/* This file has not been maintained for a long time */

#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <gnome.h>

#include "driver.h"
#include "game.h"
#include "cards.h"
#include "map.h"
#include "network.h"
#include "log.h"
#include "buildrec.h"
#include "common_gtk.h"

static GtkWidget *game_combo;	/* select game type */
static GtkWidget *terrain_toggle;	/* random terrain Yes/No */
static GtkWidget *victory_spin;	/* victory point target */
static GtkWidget *players_spin;	/* number of players */
static GtkWidget *register_toggle;	/* register with metaserver? */
static GtkWidget *port_spin;	/* server port */

static GtkWidget *clist;	/* currently connected players */

static Session *_admin_session = 0;

/* this really needs to be eliminated from here */
static gchar *server_port = "5556";
static gint server_port_int = 5556;

/* network event handler, just like the one in meta.c, state.c, etc. */
void admin_net_event(NetEvent event, Session * admin_session, gchar * line)
{
#ifdef PRINT_INFO
	g_print
	    ("admin_event: event = %#x, admin_session = %p, line = %s\n",
	     event, admin_session, line);
#endif

	switch (event) {
	case NET_READ:
		/* there is data to be read */

#ifdef PRINT_INFO
		g_print("admin_event: NET_READ: line = '%s'\n", line);
#endif
		break;

	case NET_CONNECT:
		/* connect() succeeded */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CONNECT\n");
#endif
		break;

	case NET_CONNECT_FAIL:
		/* connect() failed */

#ifdef PRINT_INFO
		g_print
		    ("admin_event: NET_CONNECT_FAIL (falling through to NET_CLOSE...)\n");
#endif

		/* fall through to NET_CLOSE */

	case NET_CLOSE:
		/* connection has been closed */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CLOSE\n");
#endif
		net_free(&admin_session);
		break;

	default:
	}
}

static void admin_open_session(void)
{
	_admin_session = net_new(admin_net_event, NULL);
	_admin_session->user_data = _admin_session;

	/* TODO: tie the connect dialog to this */
	net_connect(_admin_session, "localhost", 5555);
}

static void admin_close_session(void)
{
	if (_admin_session) {
		net_close(_admin_session);
		_admin_session = 0;
	}
}

#define ADMIN_BUFSIZE	4096
#define ADMIN_PREFIX_LEN	6
static void admin_write(gchar * fmt, ...)
{
	char buff[ADMIN_BUFSIZE];
	va_list ap;

	strncpy(buff, "admin ", ADMIN_PREFIX_LEN);

	if (!_admin_session) {
		admin_open_session();
	}

	va_start(ap, fmt);
	g_vsnprintf(&buff[ADMIN_PREFIX_LEN],
		    ADMIN_BUFSIZE - ADMIN_PREFIX_LEN, fmt, ap);
	va_end(ap);

	net_write(_admin_session, buff);
}


static void port_spin_changed_cb(GtkWidget * widget, gpointer user_data)
{
	gint server_port_int =
	    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	snprintf(server_port, sizeof(server_port), "%d", server_port_int);

	admin_write("set-port %s\n", server_port);
}

static void register_toggle_cb(GtkToggleButton * toggle,
			       gpointer user_data)
{
	GtkWidget *label = GTK_BIN(toggle)->child;

	gint register_server = gtk_toggle_button_get_active(toggle);
	gtk_label_set_text(GTK_LABEL(label),
			   register_server ? _("Yes") : _("No"));

	admin_write("set-register-server %d\n", register_server);
}

static void players_spin_changed_cb(GtkWidget * widget, gpointer user_data)
{
	admin_write("set-num-players %d\n",
		    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
						     (widget)));
}

static void victory_spin_changed_cb(GtkWidget * widget, gpointer user_data)
{
	admin_write("set-victory-points %d\n",
		    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
						     (widget)));
}

static void game_select_cb(GtkWidget * list, gpointer user_data)
{
}

static void terrain_toggle_cb(GtkToggleButton * toggle, gpointer user_data)
{
	GtkWidget *label = GTK_BIN(toggle)->child;

	gint random_terrain = gtk_toggle_button_get_active(toggle);
	gtk_label_set_text(GTK_LABEL(label),
			   random_terrain ? _("Random") : _("Default"));

	admin_write("set-random-terrain %d\n", random_terrain);
}

static void start_clicked_cb(GtkWidget * start_btn, gpointer user_data)
{
	admin_write("start-server\n");
}

void show_admin_interface()
{
	GtkWidget *admin_if;
	GtkDialog *dialog;

	dialog = gtk_dialog_new();
	gtk_window_set_title("Administration");
	admin_if = build_admin_interface(dialog->vbox);
	gtk_widget_show_all(dialog);
	admin_open_session();
}

GtkWidget *build_admin_interface(GtkWidget * vbox)
{
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label;
	GtkObject *adj;
	GtkWidget *start_btn;
	GtkWidget *scroll_win;
	GtkWidget *message_text;

	static gchar *titles[2];

	if (!vbox)
		vbox = gtk_vbox_new(FALSE, 0);

	if (!titles[0]) {
		titles[0] = _("Name");
		titles[1] = _("Location");
	}

	gtk_widget_show(vbox);
	gtk_container_border_width(GTK_CONTAINER(vbox), 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("Server Parameters"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);

	table = gtk_table_new(6, 3, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 3);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	label = gtk_label_new(_("Game Name"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	game_combo = gtk_combo_new();
	gtk_editable_set_editable(GTK_EDITABLE
				  (GTK_COMBO(game_combo)->entry), FALSE);
	gtk_widget_set_usize(game_combo, 100, -1);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(game_combo)->list),
			   "select_child",
			   GTK_SIGNAL_FUNC(game_select_cb), NULL);
	gtk_widget_show(game_combo);
	gtk_table_attach(GTK_TABLE(table), game_combo, 1, 3, 0, 1,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

	label = gtk_label_new(_("Map Terrain"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	terrain_toggle = gtk_toggle_button_new_with_label("");
	gtk_widget_show(terrain_toggle);
	gtk_table_attach(GTK_TABLE(table), terrain_toggle, 1, 2, 1, 2,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(terrain_toggle), "toggled",
			   GTK_SIGNAL_FUNC(terrain_toggle_cb), NULL);

	label = gtk_label_new(_("Number of Players"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(0, 2, MAX_PLAYERS, 1, 1, 0);
	players_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(players_spin);
	gtk_table_attach(GTK_TABLE(table), players_spin, 1, 2, 2, 3,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(players_spin), "changed",
			   GTK_SIGNAL_FUNC(players_spin_changed_cb), NULL);

	label = gtk_label_new(_("Victory Point Target"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(10, 5, 20, 1, 5, 0);
	victory_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(victory_spin);
	gtk_table_attach(GTK_TABLE(table), victory_spin, 1, 2, 3, 4,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(victory_spin), "changed",
			   GTK_SIGNAL_FUNC(victory_spin_changed_cb), NULL);

	label = gtk_label_new(_("Register Server"));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	register_toggle = gtk_toggle_button_new_with_label(_("No"));
	gtk_widget_show(register_toggle);
	gtk_table_attach(GTK_TABLE(table), register_toggle, 1, 2, 4, 5,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(register_toggle), "toggled",
			   GTK_SIGNAL_FUNC(register_toggle_cb), NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(register_toggle),
				     TRUE);
	/* gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(register_toggle)); */

	label = gtk_label_new("Server Port");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	adj = gtk_adjustment_new(server_port_int, 1024, 32767, 1, 10, 0);
	port_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(port_spin);
	gtk_table_attach(GTK_TABLE(table), port_spin, 1, 2, 5, 6,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(port_spin, 60, -1);
	gtk_signal_connect(GTK_OBJECT(port_spin), "changed",
			   GTK_SIGNAL_FUNC(port_spin_changed_cb), NULL);

	start_btn = gtk_button_new_with_label(_("Start Server"));
	gtk_widget_show(start_btn);
	gtk_table_attach(GTK_TABLE(table), start_btn, 0, 2, 6, 7,
			 (GtkAttachOptions) GTK_FILL,
			 (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(start_btn), "clicked",
			   GTK_SIGNAL_FUNC(start_clicked_cb), NULL);

	frame = gtk_frame_new(_("Players Connected"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
	gtk_widget_set_usize(frame, 250, -1);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_container_border_width(GTK_CONTAINER(scroll_win), 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	clist = gtk_clist_new_with_titles(2, titles);
	gtk_widget_show(clist);
	gtk_container_add(GTK_CONTAINER(scroll_win), clist);
	gtk_clist_set_column_width(GTK_CLIST(clist), 0, 80);
	gtk_clist_set_column_width(GTK_CLIST(clist), 1, 80);
	gtk_clist_column_titles_show(GTK_CLIST(clist));
	gtk_clist_column_titles_passive(GTK_CLIST(clist));

	frame = gtk_frame_new(_("Messages"));
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(frame), scroll_win);
	gtk_container_border_width(GTK_CONTAINER(scroll_win), 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	return vbox;
}
