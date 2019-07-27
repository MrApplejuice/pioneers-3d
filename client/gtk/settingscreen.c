/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2005 Roland Clobus <rclobus@bigfoot.com>
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
#include "gui.h"

static GtkWidget *settings_dlg = NULL;

enum { TYPE_NUM,
	TYPE_BOOL,
	TYPE_STRING
};

static void add_setting_desc(GtkWidget * grid, guint row, guint col,
			     const gchar * desc)
{
	GtkWidget *label;

	label = gtk_label_new(desc);
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, col, row, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
}

static void add_setting_desc_with_image(GtkWidget * grid, guint row,
					guint col, const gchar * desc,
					const gchar * iconname)
{
	GtkWidget *icon;

	icon = gtk_image_new_from_stock(iconname, GTK_ICON_SIZE_MENU);
	gtk_widget_show(icon);
	gtk_grid_attach(GTK_GRID(grid), icon, col, row, 1, 1);

	add_setting_desc(grid, row, col + 1, desc);
}

static void add_setting_val(GtkWidget * grid, guint row, guint col,
			    gint type, gint int_val,
			    const gchar * char_val, gboolean right_aligned)
{
	GtkWidget *label;
	gchar *label_var;

	switch (type) {
	case TYPE_NUM:
		label_var = g_strdup_printf("%i", int_val);
		break;

	case TYPE_BOOL:
		if (int_val != 0) {
			label_var = g_strdup(_("Yes"));
		} else {
			label_var = g_strdup(_("No"));
		}
		break;
	case TYPE_STRING:
		if (char_val == NULL) {
			char_val = " ";
		}
		label_var = g_strdup_printf("%s", char_val);
		break;

	default:
		label_var = g_strdup(_("Unknown"));
		break;
	}

	label = gtk_label_new(label_var);
	g_free(label_var);
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, col, row, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(label),
			     (right_aligned ? 1.0 : 0.0));
}

static GtkWidget *settings_create_content(void)
{
	const GameParams *game_params;
	GtkWidget *dlg_vbox;
	GtkWidget *alignment;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *grid;
	GtkWidget *label;
	gchar *sevens_desc;
	gchar *island_bonus;
	guint row;

	/* Create some space inside the dialog */
	dlg_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width(GTK_CONTAINER(dlg_vbox), 6);
	gtk_widget_show(dlg_vbox);

	game_params = get_game_params();
	if (game_params == NULL) {
		label = gtk_label_new(_("No game in progress..."));
		gtk_box_pack_start(GTK_BOX(dlg_vbox), label, TRUE, TRUE,
				   6);
		gtk_widget_show(label);

		return dlg_vbox;
	}

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
			     /* Label */
			     _("<b>General settings</b>"));
	gtk_widget_show(label);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), label, FALSE, TRUE, 0);

	alignment = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), alignment, FALSE, FALSE, 0);

	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_container_add(GTK_CONTAINER(alignment), grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	row = 0;
	add_setting_desc(grid, row, 0, _("Number of players:"));
	add_setting_val(grid, row, 1, TYPE_NUM, game_params->num_players,
			NULL, FALSE);
	row++;
	add_setting_desc(grid, row, 0, _("Victory point target:"));
	add_setting_val(grid, row, 1, TYPE_NUM,
			game_params->victory_points, NULL, FALSE);
	row++;
	add_setting_desc(grid, row, 0, _("Random terrain:"));
	add_setting_val(grid, row, 1, TYPE_BOOL,
			game_params->random_terrain, NULL, FALSE);
	row++;
	add_setting_desc(grid, row, 0, _("Allow trade between players:"));
	add_setting_val(grid, row, 1, TYPE_BOOL,
			game_params->domestic_trade, NULL, FALSE);
	row++;
	add_setting_desc(grid, row, 0,
			 _("Allow trade only before building or buying:"));
	add_setting_val(grid, row, 1, TYPE_BOOL, game_params->strict_trade,
			NULL, FALSE);
	row++;
	add_setting_desc(grid, row, 0,
			 _("Check victory only at end of turn:"));
	add_setting_val(grid, row, 1, TYPE_BOOL,
			game_params->check_victory_at_end_of_turn, NULL,
			FALSE);
	row++;
	add_setting_desc(grid, row, 0, _("Amount of each resource:"));
	add_setting_val(grid, row, 1, TYPE_NUM,
			game_params->resource_count, NULL, FALSE);

	if (game_params->sevens_rule == 0) {
		sevens_desc = g_strdup(_("Normal"));
	} else if (game_params->sevens_rule == 1) {
		sevens_desc = g_strdup(_("Reroll on 1st 2 turns"));
	} else if (game_params->sevens_rule == 2) {
		sevens_desc = g_strdup(_("Reroll all 7s"));
	} else {
		sevens_desc = g_strdup(_("Unknown"));
	}
	row++;
	add_setting_desc(grid, row, 0, _("Sevens rule:"));
	add_setting_val(grid, row, 1, TYPE_STRING, 0, sevens_desc, FALSE);
	g_free(sevens_desc);

	row++;
	add_setting_desc(grid, row, 0,
			 _("Use dice deck instead of dice:"));
	add_setting_val(grid, row, 1, TYPE_BOOL,
			game_params->use_dice_deck, NULL, FALSE);

	row++;
	add_setting_desc(grid, row, 0, _("Number of dice decks:"));
	add_setting_val(grid, row, 1, TYPE_NUM,
			game_params->num_dice_decks, NULL, FALSE);

	row++;
	add_setting_desc(grid, row, 0,
			 _(""
			   "Number of dice cards removed after shuffling:"));
	add_setting_val(grid, row, 1, TYPE_NUM,
			game_params->num_removed_dice_cards, NULL, FALSE);

	row++;
	add_setting_desc(grid, row, 0,
			 _("Use the pirate to block ships:"));
	add_setting_val(grid, row, 1, TYPE_BOOL, game_params->use_pirate,
			NULL, FALSE);

	if (game_params->island_discovery_bonus) {
		guint idx;
		island_bonus =
		    g_strdup_printf("%d",
				    g_array_index
				    (game_params->island_discovery_bonus,
				     gint, 0));
		for (idx = 1;
		     idx < game_params->island_discovery_bonus->len;
		     idx++) {
			gchar *old = island_bonus;
			gchar *number = g_strdup_printf("%d",
							g_array_index
							(game_params->
							 island_discovery_bonus,
							 gint, idx));
			island_bonus =
			    g_strconcat(island_bonus, ", ", number, NULL);
			g_free(old);
			g_free(number);
		}
	} else {
		island_bonus = g_strdup(_("No"));
	}
	row++;
	add_setting_desc(grid, row, 0, _("Island discovery bonuses:"));
	add_setting_val(grid, row, 1, TYPE_STRING, 0, island_bonus, FALSE);
	g_free(island_bonus);

	/* Double space, otherwise the columns are too close */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 24);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), hbox, TRUE, FALSE, 0);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
			     /* Label */
			     _("<b>Building quotas</b>"));
	gtk_widget_show(label);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

	alignment = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, TRUE, 0);

	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_container_add(GTK_CONTAINER(alignment), grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
	row = 0;

	add_setting_desc_with_image(grid, row, 0, _("Roads:"),
				    PIONEERS_PIXMAP_ROAD);
	add_setting_val(grid, row, 2, TYPE_NUM,
			game_params->num_build_type[BUILD_ROAD], NULL,
			TRUE);
	row++;
	add_setting_desc_with_image(grid, row, 0, _("Settlements:"),
				    PIONEERS_PIXMAP_SETTLEMENT);
	add_setting_val(grid, row, 2, TYPE_NUM,
			game_params->num_build_type[BUILD_SETTLEMENT],
			NULL, TRUE);
	row++;
	add_setting_desc_with_image(grid, row, 0, _("Cities:"),
				    PIONEERS_PIXMAP_CITY);
	add_setting_val(grid, row, 2, TYPE_NUM,
			game_params->num_build_type[BUILD_CITY], NULL,
			TRUE);
	row++;
	add_setting_desc_with_image(grid, row, 0, _("City walls:"),
				    PIONEERS_PIXMAP_CITY_WALL);
	add_setting_val(grid, row, 2, TYPE_NUM,
			game_params->num_build_type[BUILD_CITY_WALL], NULL,
			TRUE);
	row++;
	add_setting_desc_with_image(grid, row, 0, _("Ships:"),
				    PIONEERS_PIXMAP_SHIP);
	add_setting_val(grid, row, 2, TYPE_NUM,
			game_params->num_build_type[BUILD_SHIP], NULL,
			TRUE);
	row++;
	add_setting_desc_with_image(grid, row, 0, _("Bridges:"),
				    PIONEERS_PIXMAP_BRIDGE);
	add_setting_val(grid, row, 2, TYPE_NUM,
			game_params->num_build_type[BUILD_BRIDGE], NULL,
			TRUE);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
			     /* Label */
			     _("<b>Development card deck</b>"));
	gtk_widget_show(label);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

	alignment = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, TRUE, 0);

	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_container_add(GTK_CONTAINER(alignment), grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	add_setting_desc(grid, 0, 0, _("Road building cards:"));
	add_setting_val(grid, 0, 1, TYPE_NUM,
			game_params->num_develop_type[DEVEL_ROAD_BUILDING],
			NULL, TRUE);
	add_setting_desc(grid, 1, 0, _("Monopoly cards:"));
	add_setting_val(grid, 1, 1, TYPE_NUM,
			game_params->num_develop_type[DEVEL_MONOPOLY],
			NULL, TRUE);
	add_setting_desc(grid, 2, 0, _("Year of plenty cards:"));
	add_setting_val(grid, 2, 1, TYPE_NUM,
			game_params->num_develop_type
			[DEVEL_YEAR_OF_PLENTY], NULL, TRUE);
	add_setting_desc(grid, 3, 0, _("Chapel cards:"));
	add_setting_val(grid, 3, 1, TYPE_NUM,
			game_params->num_develop_type[DEVEL_CHAPEL], NULL,
			TRUE);
	add_setting_desc(grid, 4, 0, _("Pioneer university cards:"));
	add_setting_val(grid, 4, 1, TYPE_NUM,
			game_params->num_develop_type[DEVEL_UNIVERSITY],
			NULL, TRUE);
	add_setting_desc(grid, 5, 0, _("Governor's house cards:"));
	add_setting_val(grid, 5, 1, TYPE_NUM,
			game_params->num_develop_type
			[DEVEL_GOVERNORS_HOUSE], NULL, TRUE);
	add_setting_desc(grid, 6, 0, _("Library cards:"));
	add_setting_val(grid, 6, 1, TYPE_NUM,
			game_params->num_develop_type[DEVEL_LIBRARY], NULL,
			TRUE);
	add_setting_desc(grid, 7, 0, _("Market cards:"));
	add_setting_val(grid, 7, 1, TYPE_NUM,
			game_params->num_develop_type[DEVEL_MARKET], NULL,
			TRUE);
	add_setting_desc(grid, 8, 0, _("Soldier cards:"));
	add_setting_val(grid, 8, 1, TYPE_NUM,
			game_params->num_develop_type[DEVEL_SOLDIER], NULL,
			TRUE);

	return dlg_vbox;
}

static void settings_rules_changed(void)
{
	if (settings_dlg) {
		GtkWidget *vbox;
		GtkWidget *dlg_vbox;
		GList *list;

		dlg_vbox =
		    gtk_dialog_get_content_area(GTK_DIALOG(settings_dlg));
		list = gtk_container_get_children(GTK_CONTAINER(dlg_vbox));

		if (g_list_length(list) > 0)
			gtk_widget_destroy(GTK_WIDGET(list->data));
		g_list_free(list);

		vbox = settings_create_content();
		gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, FALSE,
				   0);
	}
}

GtkWidget *settings_create_dlg(void)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;

	if (settings_dlg != NULL)
		return settings_dlg;

	settings_dlg = gtk_dialog_new_with_buttons(
							  /* Dialog caption */
							  _(""
							    "Current Game Settings"),
							  GTK_WINDOW
							  (app_window),
							  GTK_DIALOG_DESTROY_WITH_PARENT,
							  /* Button text */
							  _("_Close"),
							  GTK_RESPONSE_CLOSE,
							  NULL);
	g_signal_connect(G_OBJECT(settings_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &settings_dlg);

	dlg_vbox = gtk_dialog_get_content_area(GTK_DIALOG(settings_dlg));
	gtk_widget_show(dlg_vbox);

	vbox = settings_create_content();
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(settings_dlg), "response",
			 G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_widget_show(settings_dlg);

	return settings_dlg;
}

void settings_init(void)
{
	gui_rules_register_callback(G_CALLBACK(settings_rules_changed));
}
