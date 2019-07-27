/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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
#include <gtk/gtk.h>
#include <string.h>
#include "game.h"
#include "state.h"
#include "common_gtk.h"
#include "cards.h"

static GtkWidget *message_txt;
static GtkWidget *message_container;
static gboolean msg_colors = TRUE;

/* Local function prototypes */
static void gtk_event_cleanup(void);
static void message_window_log_message_string(gint msg_type,
					      const gchar * text);

/* Set the default logging function to write to the message window. */
void log_set_func_message_window(void)
{
	driver->log_write = message_window_log_message_string;
}

void log_set_func_message_color_enable(gboolean enable)
{
	msg_colors = enable;
}

/* Write a message string to the console, setting its color based on its
 *   type.
 */
void message_window_log_message_string(gint msg_type, const gchar * text)
{
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GtkTextMark *end_mark;
	const gchar *tagname;

	if (message_txt == NULL)
		return;		/* No widget set */

	/* First determine if the requested color is for chat.
	 * Chat colors are separately turned on/off
	 */
	switch (msg_type) {
	case MSG_PLAYER1:
		tagname = "player1";
		break;
	case MSG_PLAYER2:
		tagname = "player2";
		break;
	case MSG_PLAYER3:
		tagname = "player3";
		break;
	case MSG_PLAYER4:
		tagname = "player4";
		break;
	case MSG_PLAYER5:
		tagname = "player5";
		break;
	case MSG_PLAYER6:
		tagname = "player6";
		break;
	case MSG_PLAYER7:
		tagname = "player7";
		break;
	case MSG_PLAYER8:
		tagname = "player8";
		break;
	default:
		/* Not chat related, check whether other messages
		 * use color
		 */
		if (!msg_colors)
			tagname = "black";
		else
			switch (msg_type) {
			case MSG_ERROR:
				tagname = "red";
				break;
			case MSG_TIMESTAMP:
			case MSG_INFO:
				tagname = "info";
				break;
			case MSG_CHAT:
				tagname = "chat";
				break;
			case MSG_SPECTATOR_CHAT:
				tagname = "chat";
				break;
			case MSG_RESOURCE:
				tagname = "resource";
				break;
			case MSG_BUILD:
				tagname = "build";
				break;
			case MSG_DICE:
				tagname = "dice";
				break;
			case MSG_STEAL:
				tagname = "steal";
				break;
			case MSG_TRADE:
				tagname = "trade";
				break;
			case MSG_DEVCARD:
				tagname = "devcard";
				break;
			case MSG_LARGESTARMY:
				tagname = "largest";
				break;
			case MSG_LONGESTROAD:
				tagname = "longest";
				break;
			case MSG_BEEP:
				tagname = "beep";
				break;
			default:
				tagname = "green";
			};
		break;
	}

	if (message_container != NULL) {
		gtk_widget_show(message_container);
	}

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_txt));

	/* insert text at the end */
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, text, -1,
						 tagname, NULL);

	/* move cursor to the end */
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_place_cursor(buffer, &iter);

	end_mark = gtk_text_buffer_get_mark(buffer, "end-mark");
	g_assert(end_mark != NULL);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(message_txt), end_mark,
				     0.0, FALSE, 0.0, 0.0);
}

/* set the text widget. */
void message_window_set_text(GtkWidget * textWidget, GtkWidget * container)
{
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	message_txt = textWidget;
	message_container = container;

	/* Prepare all tags */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_txt));
	gtk_text_buffer_create_tag(buffer, "black", "foreground", "black",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "red", "foreground", "red",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "green", "foreground", "green",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "build", "foreground",
				   "#BB0000", NULL);
	gtk_text_buffer_create_tag(buffer, "chat", "foreground", "#0000FF",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "devcard", "foreground",
				   "#C6C613", NULL);
	gtk_text_buffer_create_tag(buffer, "dice", "foreground", "#00AA00",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "info", "foreground", "#000000",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "largest", "foreground",
				   "#1CB5ED", NULL);
	gtk_text_buffer_create_tag(buffer, "longest", "foreground",
				   "#1CB5ED", NULL);
	gtk_text_buffer_create_tag(buffer, "resource", "foreground",
				   "#0000FF", NULL);
	gtk_text_buffer_create_tag(buffer, "steal", "foreground",
				   "#A613C6", NULL);
	gtk_text_buffer_create_tag(buffer, "trade", "foreground",
				   "#006600", NULL);
	gtk_text_buffer_create_tag(buffer, "beep", "foreground", "#B7AE07",
				   NULL);
	gtk_text_buffer_create_tag(buffer, "player1", "foreground",
				   "#CD0000", NULL);
	gtk_text_buffer_create_tag(buffer, "player2", "foreground",
				   "#1E90FF", NULL);
	gtk_text_buffer_create_tag(buffer, "player3", "foreground",
				   "#808080", NULL);
	gtk_text_buffer_create_tag(buffer, "player4", "foreground",
				   "#FF7F00", NULL);
	gtk_text_buffer_create_tag(buffer, "player5", "foreground",
				   "#AEAE00", NULL);
	gtk_text_buffer_create_tag(buffer, "player6", "foreground",
				   "#8EB5BE", NULL);
	gtk_text_buffer_create_tag(buffer, "player7", "foreground",
				   "#D15FBE", NULL);
	gtk_text_buffer_create_tag(buffer, "player8", "foreground",
				   "#00BE76", NULL);
	/* Set the mark that will mark the end of the text */
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_create_mark(buffer, "end-mark", &iter, FALSE);
}

static void gtk_event_cleanup(void)
{
	while (gtk_events_pending())
		gtk_main_iteration();
}

UIDriver GTK_Driver = {
	gtk_event_cleanup,

	/* initially log to the console; change it to the message window after
	 *   the message window is created. */
	log_message_string_console,

	/* callbacks for the server; NULL for now -- let the server set them */
	NULL,			/* player added */
	NULL,			/* player renamed */
	NULL,			/* player removed */
	NULL			/* player renamed */
};

struct TFindData {
	GtkTreeIter iter;
	enum TFindResult result;
	gint column;
	gint number;
};

static gboolean find_integer_cb(GtkTreeModel * model,
				G_GNUC_UNUSED GtkTreePath * path,
				GtkTreeIter * iter, gpointer user_data)
{
	struct TFindData *data = (struct TFindData *) user_data;

	int wanted = data->number;
	int current;
	gtk_tree_model_get(model, iter, data->column, &current, -1);
	if (current > wanted) {
		data->result = FIND_MATCH_INSERT_BEFORE;
		data->iter = *iter;
		return TRUE;
	} else if (current == wanted) {
		data->result = FIND_MATCH_EXACT;
		data->iter = *iter;
		return TRUE;
	}
	return FALSE;
}

enum TFindResult find_integer_in_tree(GtkTreeModel * model,
				      GtkTreeIter * iter, gint column,
				      gint number)
{
	struct TFindData data;
	data.column = column;
	data.number = number;
	data.result = FIND_NO_MATCH;
	gtk_tree_model_foreach(model, find_integer_cb, &data);
	if (data.result != FIND_NO_MATCH)
		*iter = data.iter;
	return data.result;
}

void check_victory_points(GameParams * params, GtkWindow * main_window)
{
	gchar *win_message;
	gchar *point_specification;
	WinnableState state;
	GtkMessageType message_type;
	GtkWidget *dialog;

	state =
	    params_check_winnable_state(params, &win_message,
					&point_specification);

	switch (state) {
	case PARAMS_WINNABLE:
		message_type = GTK_MESSAGE_INFO;
		break;
	case PARAMS_WIN_BUILD_ALL:
		message_type = GTK_MESSAGE_INFO;
		break;
	case PARAMS_WIN_PERHAPS:
		message_type = GTK_MESSAGE_WARNING;
		break;
	case PARAMS_NO_WIN:
		message_type = GTK_MESSAGE_ERROR;
		break;
	default:
		message_type = GTK_MESSAGE_ERROR;
		break;		/* Avoid a GCC warning about message_type being possibly uninitialized */
	}
	dialog = gtk_message_dialog_new(main_window,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					message_type,
					GTK_BUTTONS_OK, "%s", win_message);
	gtk_window_set_title(GTK_WINDOW(dialog),
			     /* Caption for result of checking victory points */
			     _("Victory Point Analysis"));

	gtk_message_dialog_format_secondary_text
	    (GTK_MESSAGE_DIALOG(dialog), "%s", point_specification);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	g_free(win_message);
	g_free(point_specification);
}

GtkWidget *create_label_with_close_button(const gchar * label_text,
					  const gchar * tooltip_text,
					  GtkWidget ** button)
{
	GtkWidget *hbox;
	GtkWidget *lbl;
	GtkWidget *close_image;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	lbl = gtk_label_new(label_text);
	gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 0);

	close_image =
	    gtk_image_new_from_icon_name("window-close",
					 GTK_ICON_SIZE_MENU);
	*button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(*button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(*button), FALSE);
	gtk_container_add(GTK_CONTAINER(*button), close_image);
	gtk_widget_set_tooltip_text(*button, tooltip_text);
	gtk_box_pack_start(GTK_BOX(hbox), *button, FALSE, FALSE, 0);

	gtk_widget_show_all(hbox);
	return hbox;
}

void build_frame(GtkWidget * parent, const gchar * title,
		 GtkWidget * element, gboolean extend)
{
	/* vbox */
	/*       label */
	/*       hbox */
	/*              fix */
	/*              element */

	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *fix;
	gchar *title_with_markup;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_box_pack_start(GTK_BOX(parent), vbox, extend, TRUE, 0);

	label = gtk_label_new(NULL);
	title_with_markup = g_strdup_printf("<b>%s</b>", title);
	gtk_label_set_markup(GTK_LABEL(label), title_with_markup);
	g_free(title_with_markup);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	fix = gtk_fixed_new();
	gtk_box_pack_start(GTK_BOX(hbox), fix, FALSE, TRUE, 6);

	gtk_box_pack_start(GTK_BOX(hbox), element, TRUE, TRUE, 0);
}

void set_tooltip_on_column(GtkTreeViewColumn * column,
			   const gchar * tooltip)
{
	GtkWidget *label;

	label = gtk_label_new(gtk_tree_view_column_get_title(column));
	gtk_widget_set_tooltip_text(label, tooltip);
	gtk_widget_show(label);
	gtk_tree_view_column_set_widget(column, label);
}

/* Code based on gtk_widget_get_pointer in gtkwidget.c */
void get_mouse_position(GtkWidget * widget, gdouble * x, gdouble * y)
{
	gint xi;
	gint yi;

	g_return_if_fail(GTK_IS_WIDGET(widget));

	xi = -1;
	yi = -1;

	if (gtk_widget_get_realized(widget)) {
		gdk_window_get_device_position(gtk_widget_get_window
					       (widget),
					       gdk_device_manager_get_client_pointer
					       (gdk_display_get_device_manager
						(gtk_widget_get_display
						 (widget))), &xi, &yi,
					       NULL);

	}

	if (x)
		*x = xi;
	if (y)
		*y = yi;
}
