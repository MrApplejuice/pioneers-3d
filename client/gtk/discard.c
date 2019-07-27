/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2004 Roland Clobus <rclobus@bigfoot.com>
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
#include "frontend.h"
#include "resource-table.h"
#include "common_gtk.h"

enum {
	DISCARD_COLUMN_PLAYER_ICON, /**< Player icon */
	DISCARD_COLUMN_PLAYER_NUM,  /**< Internal: player number */
	DISCARD_COLUMN_PLAYER_NAME, /**< Player name */
	DISCARD_COLUMN_AMOUNT,	    /**< The amount to discard */
	DISCARD_COLUMN_LAST
};

static GtkListStore *discard_store; /**< the discard data */
static GtkWidget *discard_widget;  /**< the discard widget */

static struct {
	GtkWidget *dlg;
	GtkWidget *resource_widget;
} discard;

/* Local function prototypes */
static GtkWidget *discard_create_dlg(gint num);

gboolean can_discard(void)
{
	if (discard.dlg == NULL)
		return FALSE;

	return
	    resource_table_is_total_reached(RESOURCETABLE
					    (discard.resource_widget));
}

static void amount_changed_cb(G_GNUC_UNUSED ResourceTable * rt,
			      G_GNUC_UNUSED gpointer user_data)
{
	frontend_gui_update();
}

static void button_destroyed(G_GNUC_UNUSED GtkWidget * w, gpointer num)
{
	if (callback_mode == MODE_DISCARD)
		discard_create_dlg(GPOINTER_TO_INT(num));
}

static GtkWidget *discard_create_dlg(gint num)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	gchar *text;

	discard.dlg = gtk_dialog_new_with_buttons(
							 /* Dialog caption */
							 _(""
							   "Discard Resources"),
							 GTK_WINDOW
							 (app_window),
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 /* Button text */
							 _("_OK"),
							 GTK_RESPONSE_OK,
							 NULL);
	g_signal_connect(G_OBJECT(discard.dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &discard.dlg);
	gtk_widget_realize(discard.dlg);
	/* Disable close */
	gdk_window_set_functions(gtk_widget_get_window(discard.dlg),
				 GDK_FUNC_ALL | GDK_FUNC_CLOSE);

	dlg_vbox = gtk_dialog_get_content_area(GTK_DIALOG(discard.dlg));
	gtk_widget_show(dlg_vbox);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	text = g_strdup_printf(ngettext("You must discard %d resource",
					"You must discard %d resources",
					num), num);
	discard.resource_widget =
	    resource_table_new(text, RESOURCE_TABLE_LESS_IN_HAND, FALSE);
	g_free(text);
	resource_table_set_total(RESOURCETABLE(discard.resource_widget),
				 /* Label */
				 _("Total discards"), num);
	gtk_widget_show(discard.resource_widget);
	gtk_box_pack_start(GTK_BOX(vbox), discard.resource_widget, FALSE,
			   TRUE, 0);
	g_signal_connect(G_OBJECT(discard.resource_widget), "change",
			 G_CALLBACK(amount_changed_cb), NULL);

	frontend_gui_register(gui_get_dialog_button
			      (GTK_DIALOG(discard.dlg), 0), GUI_DISCARD,
			      "clicked");
	/* This _must_ be after frontend_gui_register, otherwise the
	 * regeneration of the button happens before the destruction, which
	 * results in an incorrectly sensitive OK button. */
	g_signal_connect(gui_get_dialog_button(GTK_DIALOG(discard.dlg), 0),
			 "destroy", G_CALLBACK(button_destroyed),
			 GINT_TO_POINTER(num));
	gtk_widget_show(discard.dlg);
	frontend_gui_update();

	return discard.dlg;
}

void discard_get_list(gint * discards)
{
	if (discard.dlg != NULL)
		resource_table_get_amount(RESOURCETABLE
					  (discard.resource_widget),
					  discards);
	else
		memset(discards, 0, NO_RESOURCE * sizeof(*discards));
}

void discard_player_did(gint player_num)
{
	GtkTreeIter iter;
	enum TFindResult found;
	/* check if the player was in the list.  If not, it is not an error.
	 * That happens if the player auto-discards. */
	found =
	    find_integer_in_tree(GTK_TREE_MODEL(discard_store), &iter,
				 DISCARD_COLUMN_PLAYER_NUM, player_num);

	if (found == FIND_MATCH_EXACT) {
		gtk_list_store_remove(discard_store, &iter);
		if (player_num == my_player_num()) {
			gtk_widget_destroy(discard.dlg);
			discard.dlg = NULL;
		}
	}
}

void discard_player_must(gint player_num, gint num)
{
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	enum TFindResult found;

	/* Search for a place to add information about the player */
	found =
	    find_integer_in_tree(GTK_TREE_MODEL(discard_store), &iter,
				 DISCARD_COLUMN_PLAYER_NUM, player_num);
	switch (found) {
	case FIND_NO_MATCH:
		gtk_list_store_append(discard_store, &iter);
		break;
	case FIND_MATCH_INSERT_BEFORE:
		gtk_list_store_insert_before(discard_store, &iter, &iter);
		break;
	case FIND_MATCH_EXACT:
		break;
	default:
		g_error("unknown case in discard_player_must");
	};

	pixbuf = player_create_icon(player_num, TRUE);
	gtk_list_store_set(discard_store, &iter,
			   DISCARD_COLUMN_PLAYER_ICON, pixbuf,
			   DISCARD_COLUMN_PLAYER_NUM, player_num,
			   DISCARD_COLUMN_PLAYER_NAME,
			   player_name(player_num, TRUE),
			   DISCARD_COLUMN_AMOUNT, num, -1);
	g_object_unref(pixbuf);

	if (player_num != my_player_num())
		return;

	discard_create_dlg(num);
}

void discard_begin(void)
{
	gtk_list_store_clear(GTK_LIST_STORE(discard_store));
	gui_discard_show();
}

void discard_end(void)
{
	if (discard.dlg) {
		gtk_widget_destroy(discard.dlg);
		discard.dlg = NULL;
	}

	gui_discard_hide();
}

GtkWidget *discard_build_page(void)
{
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *alignment;
	GtkWidget *scroll_win;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show(vbox);

	alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 3, 3);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
			     /* Caption for list of player that must discard cards */
			     _("<b>Waiting for players to discard</b>"));
	gtk_widget_show(label);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_container_add(GTK_CONTAINER(alignment), label);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (scroll_win), GTK_SHADOW_IN);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	discard_store = gtk_list_store_new(DISCARD_COLUMN_LAST, GDK_TYPE_PIXBUF,	/* player icon */
					   G_TYPE_INT,	/* player number */
					   G_TYPE_STRING,	/* text */
					   G_TYPE_INT);	/* amount to discard */
	discard_widget =
	    gtk_tree_view_new_with_model(GTK_TREE_MODEL(discard_store));

	column = gtk_tree_view_column_new_with_attributes("",
							  gtk_cell_renderer_pixbuf_new
							  (), "pixbuf",
							  DISCARD_COLUMN_PLAYER_ICON,
							  NULL);
	gtk_tree_view_column_set_sizing(column,
					GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(discard_widget), column);

	column = gtk_tree_view_column_new_with_attributes("",
							  gtk_cell_renderer_text_new
							  (), "text",
							  DISCARD_COLUMN_PLAYER_NAME,
							  NULL);
	gtk_tree_view_column_set_sizing(column,
					GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(discard_widget), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("",
							  renderer,
							  "text",
							  DISCARD_COLUMN_AMOUNT,
							  NULL);
	g_object_set(renderer, "xalign", 1.0f, NULL);
	gtk_tree_view_column_set_sizing(column,
					GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(discard_widget), column);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(discard_widget),
					  FALSE);
	gtk_widget_show(discard_widget);
	gtk_container_add(GTK_CONTAINER(scroll_win), discard_widget);

	return vbox;
}
