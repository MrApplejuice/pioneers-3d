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
#include "common_gtk.h"

/** Reorder the development types:
 *  Road building
 *  Monopoly
 *  Year of Plenty
 *  Soldier
 *  Victory points
 */
static gint develtype_to_sortorder(DevelType type)
{
	switch (type) {
	case DEVEL_ROAD_BUILDING:
		return 0;
	case DEVEL_MONOPOLY:
		return 1;
	case DEVEL_YEAR_OF_PLENTY:
		return 2;
	case DEVEL_SOLDIER:
		return 10;
	case DEVEL_CHAPEL:
		return 20;
	case DEVEL_UNIVERSITY:
		return 21;
	case DEVEL_GOVERNORS_HOUSE:
		return 22;
	case DEVEL_LIBRARY:
		return 23;
	case DEVEL_MARKET:
		return 24;
	default:
		g_assert_not_reached();
		return 99;
	};
};

enum {
	DEVELOP_COLUMN_TYPE, /**< Development card type */
	DEVELOP_COLUMN_ORDER, /**< Sort order */
	DEVELOP_COLUMN_NAME, /**< Name of the card */
	DEVELOP_COLUMN_DESCRIPTION, /**< Description of the card */
	DEVELOP_COLUMN_AMOUNT, /**< Amount of the cards */
	DEVELOP_COLUMN_LAST
};

static GtkListStore *store;    /**< The data for the GUI */
static gint selected_card_idx; /**< currently selected development card */

gint develop_current_idx(void)
{
	return selected_card_idx;
}

static gint develop_click_cb(G_GNUC_UNUSED GtkWidget * widget,
			     G_GNUC_UNUSED GdkEventButton * event,
			     gpointer play_develop_btn)
{
	if (event->type == GDK_2BUTTON_PRESS) {
		if (can_play_develop(develop_current_idx()))
			gtk_button_clicked(GTK_BUTTON(play_develop_btn));
	};
	return FALSE;
}

static void develop_select_cb(GtkTreeSelection * selection,
			      G_GNUC_UNUSED gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	g_assert(selection != NULL);
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		DevelType type;
		gtk_tree_model_get(model, &iter, DEVELOP_COLUMN_TYPE,
				   &type, -1);
		selected_card_idx =
		    deck_card_oldest_card(get_devel_deck(), type);
	} else
		selected_card_idx = -1;
	frontend_gui_update();
}

GtkWidget *develop_build_page(void)
{
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *scroll_win;
	GtkWidget *bbox;
	GtkWidget *alignment;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *play_develop_btn;
	GtkWidget *develop_list;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_widget_show(vbox);

	alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 3, 3);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
			     /* Caption for list of bought development cards */
			     _("<b>Development cards</b>"));
	gtk_widget_show(label);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_container_add(GTK_CONTAINER(alignment), label);

	/* Create model */
	store = gtk_list_store_new(DEVELOP_COLUMN_LAST,
				   G_TYPE_INT,
				   G_TYPE_INT,
				   G_TYPE_STRING, G_TYPE_STRING,
				   G_TYPE_STRING);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scroll_win, -1, 100);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (scroll_win), GTK_SHADOW_IN);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_AUTOMATIC);

	/* Create graphical representation of the model */
	develop_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(develop_list),
					 DEVELOP_COLUMN_DESCRIPTION);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(develop_list),
					  FALSE);
	gtk_container_add(GTK_CONTAINER(scroll_win), develop_list);

	/* First create the button, it is used as user_data for the listview */
	play_develop_btn = gtk_button_new_with_label(
							    /* Button text: play development card */
							    _(""
							      "Play Card"));

	/* Register double-click */
	g_signal_connect(G_OBJECT(develop_list), "button_press_event",
			 G_CALLBACK(develop_click_cb), play_develop_btn);

	g_signal_connect(G_OBJECT
			 (gtk_tree_view_get_selection
			  (GTK_TREE_VIEW(develop_list))), "changed",
			 G_CALLBACK(develop_select_cb), NULL);

	/* Now create columns */
	column = gtk_tree_view_column_new_with_attributes(
								 /* Not translated: it is not visible */
								 "Development Cards",
								 gtk_cell_renderer_text_new
								 (),
								 "text",
								 DEVELOP_COLUMN_NAME,
								 NULL);
	gtk_tree_view_column_set_sizing(column,
					GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(develop_list), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
								 /* Not translated: it is not visible */
								 "Amount",
								 renderer,
								 "text",
								 DEVELOP_COLUMN_AMOUNT,
								 NULL);
	g_object_set(renderer, "xalign", 1.0f, NULL);
	gtk_tree_view_column_set_sizing(column,
					GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(develop_list), column);

	gtk_widget_show(develop_list);

	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);

	frontend_gui_register(play_develop_btn, GUI_PLAY_DEVELOP,
			      "clicked");
	gtk_widget_show(play_develop_btn);
	gtk_container_add(GTK_CONTAINER(bbox), play_develop_btn);

	selected_card_idx = -1;
	return vbox;
}

static void update_model(GtkTreeIter * iter, DevelType type)
{
	gchar amount_string[16];
	gint amount;

	/* Only show the amount when you have more than one */
	amount = deck_card_amount(get_devel_deck(), type);
	if (amount == 1)
		amount_string[0] = '\0';
	else
		snprintf(amount_string, sizeof(amount_string), "%d",
			 amount);

	gtk_list_store_set(store, iter,
			   DEVELOP_COLUMN_NAME,
			   get_devel_name(type),
			   DEVELOP_COLUMN_DESCRIPTION,
			   get_devel_description(type),
			   DEVELOP_COLUMN_AMOUNT, amount_string,
			   DEVELOP_COLUMN_TYPE, type,
			   DEVELOP_COLUMN_ORDER,
			   develtype_to_sortorder(type), -1);
}

void frontend_bought_develop(DevelType type)
{
	GtkTreeIter iter;
	enum TFindResult found;

	found =
	    find_integer_in_tree(GTK_TREE_MODEL(store), &iter,
				 DEVELOP_COLUMN_ORDER,
				 develtype_to_sortorder(type));

	switch (found) {
	case FIND_MATCH_EXACT:
		/* Don't add new items */
		break;
	case FIND_MATCH_INSERT_BEFORE:
		gtk_list_store_insert_before(store, &iter, &iter);
		break;
	case FIND_NO_MATCH:
		gtk_list_store_append(store, &iter);
	};

	update_model(&iter, type);
}

void frontend_played_develop(gint player_num, G_GNUC_UNUSED guint card_idx,
			     DevelType type)
{
	GtkTreeIter iter;
	enum TFindResult found;

	if (player_num == my_player_num()) {
		found =
		    find_integer_in_tree(GTK_TREE_MODEL(store), &iter,
					 DEVELOP_COLUMN_ORDER,
					 develtype_to_sortorder(type));
		g_assert(found == FIND_MATCH_EXACT);
		if (deck_card_amount(get_devel_deck(), type) == 0) {
			gtk_list_store_remove(store, &iter);
			/* Do not update selected_card_idx, it was updated
			 * by the callback develop_select_cb */
		} else {
			update_model(&iter, type);
			selected_card_idx =
			    deck_card_oldest_card(get_devel_deck(), type);
		}
	};
}

void develop_reset(void)
{
	selected_card_idx = -1;
	gtk_list_store_clear(store);
}
