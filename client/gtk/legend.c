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
#include "theme.h"
#include "map-icons.h"
#include "cost.h"
#include "resource-view.h"

/* The order of the terrain_names is EXTREMELY important!  The order
 * must match the enum Terrain.
 */
static const gchar *terrain_names[] = {
	N_("Hill"),
	N_("Field"),
	N_("Mountain"),
	N_("Pasture"),
	N_("Forest"),
	N_("Desert"),
	N_("Sea"),
	N_("Gold")
};

static GtkWidget *legend_dlg = NULL;
static gboolean legend_did_connect = FALSE;

static void legend_theme_changed(void);
static void legend_rules_changed(void);

static void add_legend_terrain(GtkWidget * grid, guint row, guint col,
			       Terrain terrain, Resource resource)
{
	GtkWidget *label;

	gtk_grid_attach(GTK_GRID(grid), terrain_icon_new(terrain), col,
			row, 1, 1);

	label = gtk_label_new(_(terrain_names[terrain]));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, col + 1, row, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);

	if (resource < NO_RESOURCE) {
		label = resource_view_new_single_resource(resource);
		gtk_widget_show(label);
		gtk_grid_attach(GTK_GRID(grid), label, col + 2, row, 1, 1);

		label = gtk_label_new(resource_name(resource, TRUE));
		gtk_widget_show(label);
		gtk_grid_attach(GTK_GRID(grid), label, col + 3, row, 1, 1);
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	}
}

static void add_legend_cost(GtkWidget * grid, guint row,
			    const gchar * iconname, const gchar * item,
			    const gint * cost)
{
	GtkWidget *label;
	GtkWidget *icon;

	icon = gtk_image_new_from_stock(iconname, GTK_ICON_SIZE_MENU);
	gtk_widget_show(icon);
	gtk_grid_attach(GTK_GRID(grid), icon, 0, row, 1, 1);
	label = gtk_label_new(item);
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 1, row, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);

	label = resource_view_new();
	resource_view_set(RESOURCE_VIEW(label), cost);
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 2, row, 1, 1);
}

static GtkWidget *legend_create_content_with_scrolling(gboolean
						       enable_scrolling)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *grid;
	GtkWidget *vsep;
	GtkWidget *alignment;
	guint num_rows;
	GtkWidget *viewport;
	GtkWidget *scrolled_window;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	label = gtk_label_new(NULL);
	/* Label */
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Terrain yield</b>"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

	alignment = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_container_add(GTK_CONTAINER(alignment), grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	add_legend_terrain(grid, 0, 0, HILL_TERRAIN, BRICK_RESOURCE);
	add_legend_terrain(grid, 1, 0, FIELD_TERRAIN, GRAIN_RESOURCE);
	add_legend_terrain(grid, 2, 0, MOUNTAIN_TERRAIN, ORE_RESOURCE);
	add_legend_terrain(grid, 3, 0, PASTURE_TERRAIN, WOOL_RESOURCE);

	vsep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	gtk_widget_show(vsep);
	gtk_grid_attach(GTK_GRID(grid), vsep, 4, 0, 1, 4);

	add_legend_terrain(grid, 0, 5, FOREST_TERRAIN, LUMBER_RESOURCE);
	add_legend_terrain(grid, 1, 5, GOLD_TERRAIN, GOLD_RESOURCE);
	add_legend_terrain(grid, 2, 5, DESERT_TERRAIN, NO_RESOURCE);
	add_legend_terrain(grid, 3, 5, SEA_TERRAIN, NO_RESOURCE);

	label = gtk_label_new(NULL);
	/* Label */
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Building costs</b>"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

	alignment = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_container_add(GTK_CONTAINER(alignment), grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

	num_rows = 0;
	add_legend_cost(grid, num_rows++, PIONEERS_PIXMAP_ROAD, _("Road"),
			cost_road());
	if (have_ships())
		add_legend_cost(grid, num_rows++, PIONEERS_PIXMAP_SHIP,
				_("Ship"), cost_ship());
	if (have_bridges())
		add_legend_cost(grid, num_rows++, PIONEERS_PIXMAP_BRIDGE,
				_("Bridge"), cost_bridge());
	add_legend_cost(grid, num_rows++, PIONEERS_PIXMAP_SETTLEMENT,
			_("Settlement"), cost_settlement());
	add_legend_cost(grid, num_rows++, PIONEERS_PIXMAP_CITY, _("City"),
			cost_upgrade_settlement());
	if (have_city_walls())
		add_legend_cost(grid, num_rows++,
				PIONEERS_PIXMAP_CITY_WALL, _("City wall"),
				cost_city_wall());
	add_legend_cost(grid, num_rows++, PIONEERS_PIXMAP_DEVELOP,
			_("Development card"), cost_development());

	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	if (!enable_scrolling) {
		return hbox;
	}

	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	gtk_container_add(GTK_CONTAINER(viewport), hbox);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
				       (scrolled_window),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (scrolled_window),
					    GTK_SHADOW_NONE);
	gtk_widget_show(scrolled_window);
	gtk_container_add(GTK_CONTAINER(scrolled_window), viewport);
	return scrolled_window;
}

GtkWidget *legend_create_content(void)
{
	return legend_create_content_with_scrolling(TRUE);
}

GtkWidget *legend_create_dlg(void)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;

	if (legend_dlg != NULL) {
		gtk_window_present(GTK_WINDOW(legend_dlg));
		return legend_dlg;
	}

	/* Dialog caption */
	legend_dlg = gtk_dialog_new_with_buttons(_("Legend"),
						 GTK_WINDOW(app_window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 /* Button text */
						 _("_Close"),
						 GTK_RESPONSE_CLOSE, NULL);
	g_signal_connect(G_OBJECT(legend_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &legend_dlg);

	dlg_vbox = gtk_dialog_get_content_area(GTK_DIALOG(legend_dlg));
	gtk_widget_show(dlg_vbox);

	vbox = legend_create_content_with_scrolling(FALSE);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);

	gtk_widget_show(legend_dlg);

	if (!legend_did_connect) {
		theme_register_callback(G_CALLBACK(legend_theme_changed));
		gui_rules_register_callback(G_CALLBACK
					    (legend_rules_changed));
		legend_did_connect = TRUE;
	}

	/* destroy dialog when OK is clicked */
	g_signal_connect(legend_dlg, "response",
			 G_CALLBACK(gtk_widget_destroy), NULL);

	return legend_dlg;
}

static void legend_theme_changed(void)
{
	if (legend_dlg)
		gtk_widget_queue_draw(legend_dlg);
}

static void legend_rules_changed(void)
{
	if (legend_dlg) {
		GtkWidget *dlg_vbox =
		    gtk_dialog_get_content_area(GTK_DIALOG(legend_dlg));
		GtkWidget *vbox;
		GList *list =
		    gtk_container_get_children(GTK_CONTAINER(dlg_vbox));

		if (g_list_length(list) > 0)
			gtk_widget_destroy(GTK_WIDGET(list->data));

		vbox = legend_create_content();
		gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, TRUE, TRUE, 0);

		g_list_free(list);
	}
}
