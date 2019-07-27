/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2005 Brian Wellington
 * Copyright (C) 2005,2011 Roland Clobus
 * Copyright (C) 2005,2006 Bas Wijnen
 * Copyright (C) 2011-2013 Micah Bunting <Amnykon@gmail.com>
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
#include "version.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <string.h>
#include "aboutbox.h"
#include "config-gnome.h"
#include "game.h"
#include "game-settings.h"
#include "game-rules.h"
#include "game-devcards.h"
#include "game-buildings.h"
#include "game-resources.h"
#include "scrollable-text-view.h"
#include "guimap.h"
#include "theme.h"
#include "map-icons.h"
#include "colors.h"
#include "common_gtk.h"
#include "cards.h"
#include "network.h"

#define MAINICON_FILE "pioneers-editor.png"

#define MAP_WIDTH 550		/* default map width */
#define MAP_HEIGHT 400		/* default map height */

#define BUTTON_HEIGHT 24	/* height of the toolbar buttons */
#define TERRAIN_BUTTON_WIDTH 27	/* width of the terrain toolbar buttons */

static GtkWidget *toplevel;
static gchar *default_game;
static gchar *window_title;
static gchar *open_filename;
static GameSettings *game_settings;
static GameRules *game_rules;
static GameDevCards *game_devcards;
static GameBuildings *game_buildings;
static GameResources *game_resources;
static GtkWidget *game_title;
static GtkWidget *game_description;
static GtkWidget *game_comments;
static GtkWidget *terrain_menu;
static GtkWidget *roll_menu;
static GtkWidget *roll_numbers[12 + 1];
static GtkCheckMenuItem *shuffle_tile;
static GtkWidget *port_menu;
static GtkWidget *port_directions[6];
static GtkWidget *hresize_buttons[4];
static GtkWidget *vresize_buttons[4];
typedef enum {
	RESIZE_INSERT_LEFT,
	RESIZE_REMOVE_LEFT,
	RESIZE_INSERT_RIGHT,
	RESIZE_REMOVE_RIGHT
} hresize_type;
/* order of vresize must match order of hresize */
typedef enum {
	RESIZE_INSERT_TOP,
	RESIZE_REMOVE_TOP,
	RESIZE_INSERT_BOTTOM,
	RESIZE_REMOVE_BOTTOM
} vresize_type;

static GuiMap *gmap;
static Hex *current_hex;

typedef enum {
	TERRAIN_TOOLBAR_BUTTON_TYPE,
	CHIT_TOOLBAR_BUTTON_TYPE,
	PORT_TOOLBAR_BUTTON_TYPE,
	NO_TOOLBAR_BUTTON_TYPE
} ToolbarButtonType;

typedef struct {
	ToolbarButtonType type;
	union {
		Terrain terrain;
		gint chit;
		Resource port;
	} element;
} ToolbarButtonData;

static ToolbarButtonData selected_toolbar_button;

static const gchar *terrain_names[] = {
	/* Use an unique shortcut key for each resource */
	N_("_Hill"),
	/* Use an unique shortcut key for each resource */
	N_("_Field"),
	/* Use an unique shortcut key for each resource */
	N_("_Mountain"),
	/* Use an unique shortcut key for each resource */
	N_("_Pasture"),
	/* Use an unique shortcut key for each resource */
	N_("F_orest"),
	/* Use an unique shortcut key for each resource */
	N_("_Desert"),
	/* Use an unique shortcut key for each resource */
	N_("_Sea"),
	/* Use an unique shortcut key for each resource */
	N_("_Gold"),
	/* Use an unique shortcut key for each resource */
	N_("_None")
};

static const gchar *port_names[] = {
	/* Use an unique shortcut key for each port type */
	N_("_Brick (2:1)"),
	/* Use an unique shortcut key for each port type */
	N_("_Grain (2:1)"),
	/* Use an unique shortcut key for each port type */
	N_("_Ore (2:1)"),
	/* Use an unique shortcut key for each port type */
	N_("_Wool (2:1)"),
	/* Use an unique shortcut key for each port type */
	N_("_Lumber (2:1)"),
	/* Use an unique shortcut key for each port type */
	N_("_None"),
	/* Use an unique shortcut key for each port type */
	N_("_Any (3:1)")
};

static const gchar *port_direction_names[] = {
	/* East */
	N_("East|E"),
	/* North east */
	N_("North East|NE"),
	/* North west */
	N_("North West|NW"),
	/* West */
	N_("West|W"),
	/* South west */
	N_("South West|SW"),
	/* South east */
	N_("South East|SE")
};

static void check_vp_cb(GObject * caller, gpointer main_window);
static void set_window_title(const gchar * title);

static void error_dialog(const char *fmt, ...)
{
	GtkWidget *dialog;
	gchar *buf;
	va_list ap;

	va_start(ap, fmt);
	buf = g_strdup_vprintf(fmt, ap);
	va_end(ap);

	dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
					GTK_DIALOG_MODAL |
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE, "%s", buf);
	g_free(buf);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void fill_map(Map * map)
{
	gint x, y;

	for (y = 0; y < map->y_size; y++) {
		for (x = 0; x < map->x_size; x++) {
			if (x == 0 && y % 2 == 0 && map->shrink_left) {
				continue;
			}
			if (x == map->x_size - 1 && y % 2 == 1
			    && map->shrink_right) {
				continue;
			}
			if (map->grid[y][x] != NULL) {
				continue;
			}
			/* Add a default hex on the empty spot */
			map_reset_hex(map, x, y);
		}
	}
}

static void canonicalize_map(Map * map)
{
	Hex *hex;
	gint x, y;
	gint sequence_number;

	if (map->chits != NULL)
		g_array_free(map->chits, TRUE);
	map->chits = g_array_new(FALSE, FALSE, sizeof(gint));
	sequence_number = 0;
	for (y = 0; y < map->y_size; y++) {
		for (x = 0; x < map->x_size; x++) {
			hex = map->grid[y][x];
			if (hex == NULL)
				continue;
			if (hex->roll > 0) {
				g_array_append_val(map->chits, hex->roll);
				hex->chit_pos = sequence_number++;
			} else if (hex->terrain == DESERT_TERRAIN) {
				hex->chit_pos = sequence_number++;
			}
		}
	}
}

static gboolean terrain_has_chit(Terrain terrain)
{
	if (terrain == HILL_TERRAIN || terrain == FIELD_TERRAIN ||
	    terrain == MOUNTAIN_TERRAIN || terrain == PASTURE_TERRAIN ||
	    terrain == FOREST_TERRAIN || terrain == GOLD_TERRAIN)
		return TRUE;
	return FALSE;
}

/** Changes the terrain of a hex to terrain.
 * @param hex The hex to change the terrain of.
 * @param terrain The terrain to change the hex terrain to.
 */
static void change_terrain(Hex * hex, Terrain terrain)
{
	Hex *adjacent;
	gint i;

	if (terrain == hex->terrain)
		return;

	hex->terrain = terrain;
	if (terrain_has_chit(terrain)) {
		if (hex->roll == 0)
			hex->roll = 2;
	} else
		hex->roll = 0;

	if (terrain != SEA_TERRAIN)
		hex->resource = NO_RESOURCE;

	/* If terrain is not land,
	 * remove all ports on adjacent hexes that point to hex */
	if (terrain == SEA_TERRAIN || terrain == LAST_TERRAIN) {
		for (i = 0; i < 6; i++) {
			adjacent = hex_in_direction(hex, i);
			if (adjacent != NULL
			    && adjacent->resource != NO_RESOURCE
			    && adjacent->facing == (i + 3) % 6) {
				adjacent->resource = NO_RESOURCE;
				adjacent->facing = 0;
				guimap_draw_hex(gmap, adjacent);
			}
		}
	}
	guimap_draw_hex(gmap, hex);
	return;
}

/** Draws the chit in the exposed area.
 * @param widget The widget to draw on
 * @param cr The cairo context
 * @param chip_number The chit number of the hex.
 * @return TRUE if event is handled.
 */
static gboolean draw_chit_cb(GtkWidget * widget, cairo_t * cr,
			     gpointer chip_number)
{
	PangoLayout *layout;

	layout = gtk_widget_create_pango_layout(widget, "");
	draw_dice_roll(layout, cr,
		       BUTTON_HEIGHT / 2, BUTTON_HEIGHT / 2, BUTTON_HEIGHT,
		       GPOINTER_TO_INT(chip_number), SEA_TERRAIN, FALSE);
	g_object_unref(layout);
	return TRUE;
}

/** Draws the port in the exposed area.
 * @param widget The widget to draw on
 * @param cr The cairo context
 * @param port_type The type of the port.
 * @return TRUE if event is handled.
 */
static gboolean draw_port_cb(GtkWidget * widget, cairo_t * cr,
			     gpointer port_type)
{
	PangoLayout *layout;
	MapTheme *theme;
	gint width;
	gint height;

	layout = gtk_widget_create_pango_layout(widget, "");

	theme = theme_get_current();
	width = gdk_pixbuf_get_width(theme->terrain_tiles[SEA_TERRAIN]);
	height = gdk_pixbuf_get_height(theme->terrain_tiles[SEA_TERRAIN]);
	gdk_cairo_set_source_pixbuf(cr, theme->terrain_tiles[SEA_TERRAIN],
				    gtk_widget_get_allocated_width(widget)
				    / 2 - width / 2,
				    gtk_widget_get_allocated_height(widget)
				    / 2 - height / 2);
	cairo_pattern_set_extend(cairo_get_source(cr),
				 CAIRO_EXTEND_REPEAT);
	cairo_paint(cr);

	draw_port_indicator(layout, cr,
			    gtk_widget_get_allocated_width(widget) / 2,
			    gtk_widget_get_allocated_height(widget) / 2,
			    11, GPOINTER_TO_INT(port_type));
	g_object_unref(layout);

	return TRUE;
}

/** Draws the unselect button.
 * @param widget The widget to draw on
 * @param cr The cairo context
 * @param user_data Not used
 * @return TRUE if event is handled
 */
static gboolean draw_unselect_cb(G_GNUC_UNUSED GtkWidget * widget,
				 cairo_t * cr,
				 G_GNUC_UNUSED gpointer user_data)
{
	gdk_cairo_set_source_rgba(cr, &white);
	cairo_paint(cr);

	return TRUE;
}

/** Selects the toolbar button that was clicked.
 * @param button The GtkButton that was clicked.
 * @param user_data A ToolbarButtonData to set selected_toolbar_button to.
 * @return TRUE if event is handled.
 */
static gboolean toolbar_click(G_GNUC_UNUSED GtkButton * button,
			      G_GNUC_UNUSED gpointer user_data)
{
	ToolbarButtonData *toolbar_data = user_data;
	selected_toolbar_button = *toolbar_data;
	return TRUE;
}

/** The tooltips for the terrain toolbar buttons.
 */
static const gchar *TERRAIN_TOOLBAR_TOOLTIP[] = {
	N_("Place a hill hex"),
	N_("Place a field hex"),
	N_("Place a mountain hex"),
	N_("Place a pasture hex"),
	N_("Place a forest hex"),
	N_("Place a desert hex"),
	N_("Place a sea hex"),
	N_("Place a gold hex"),
	N_("Clear the hex")
};

/** The tooltips for the chit toolbar buttons.
 */
static const gchar *CHIT_TOOLBAR_TOOLTIP[] = {
	NULL,
	NULL,
	N_("Place a 2 chit"),
	N_("Place a 3 chit"),
	N_("Place a 4 chit"),
	N_("Place a 5 chit"),
	N_("Place a 6 chit"),
	NULL,
	N_("Place an 8 chit"),
	N_("Place a 9 chit"),
	N_("Place a 10 chit"),
	N_("Place an 11 chit"),
	N_("Place a 12 chit")
};

/** The tooltips for the port toolbar buttons.
 */
static const gchar *PORT_TOOLBAR_TOOLTIP[] = {
	N_("Place a brick 2:1 port"),
	N_("Place a grain 2:1 port"),
	N_("Place an ore 2:1 port"),
	N_("Place a wool 2:1 port"),
	N_("Place a lumber 2:1 port"),
	/* Tooltip to remove a port tile in the editor */
	N_("Remove a port"),
	N_("Place a 3:1 port")
};

/** Builds toolbars for selecting terrains, chits, and ports. Places toolbar in
 * the grid at the top.
 * @param grid The grid to place the toolbar in.
 */
static void build_select_bars(GtkWidget * grid)
{
	GtkWidget *flowbox;
	GtkWidget *box;
	GtkWidget *area;
	GtkWidget *button;
	GSList *group;
	gint i;
	ToolbarButtonData *toolbar_button_data;

	flowbox = gtk_flow_box_new();
	gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), 10);
	gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(flowbox), FALSE);
	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flowbox),
					GTK_SELECTION_NONE);

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	area = gtk_drawing_area_new();
	gtk_widget_show(area);
	gtk_widget_set_size_request(area, BUTTON_HEIGHT,
				    TERRAIN_BUTTON_WIDTH);
	g_signal_connect(G_OBJECT(area), "draw",
			 G_CALLBACK(draw_unselect_cb), GINT_TO_POINTER(1));

	button = GTK_WIDGET(gtk_radio_tool_button_new(NULL));
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(button), area);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
	gtk_widget_set_margin_right(button, 10);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(button), _
				       ("Select the terrain type with the context "
					"menu on the left click or toggle a node to "
					"exclude it from being used during the setup "
					"phase"));

	toolbar_button_data = g_malloc(sizeof(*toolbar_button_data));
	toolbar_button_data->type = NO_TOOLBAR_BUTTON_TYPE;

	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(toolbar_click), toolbar_button_data);

	/* initialize selected toolbar button to not selected. */
	selected_toolbar_button.type = NO_TOOLBAR_BUTTON_TYPE;
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(button),
					  TRUE);

	/* terrain toolbar buttons */
	for (i = 0; i <= LAST_TERRAIN; i++) {
		area = terrain_icon_new(i);
		group =
		    gtk_radio_tool_button_get_group(GTK_RADIO_TOOL_BUTTON
						    (button));
		button = GTK_WIDGET(gtk_radio_tool_button_new(group));
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(button),
						area);
		gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(button),
					       _(TERRAIN_TOOLBAR_TOOLTIP
						 [i]));

		toolbar_button_data =
		    g_malloc(sizeof(*toolbar_button_data));
		toolbar_button_data->type = TERRAIN_TOOLBAR_BUTTON_TYPE;
		toolbar_button_data->element.terrain = i;

		g_signal_connect(G_OBJECT(button), "clicked",
				 G_CALLBACK(toolbar_click),
				 toolbar_button_data);

	}

	gtk_container_add(GTK_CONTAINER(flowbox), box);

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	/* chit toolbar buttons */
	for (i = 2; i <= 12; i++) {
		if (i == 7) {
			continue;
		}
		area = gtk_drawing_area_new();
		gtk_widget_show(area);
		gtk_widget_set_size_request(area, BUTTON_HEIGHT,
					    BUTTON_HEIGHT);
		g_signal_connect(G_OBJECT(area), "draw",
				 G_CALLBACK(draw_chit_cb),
				 GINT_TO_POINTER(i));

		group =
		    gtk_radio_tool_button_get_group(GTK_RADIO_TOOL_BUTTON
						    (button));
		button = GTK_WIDGET(gtk_radio_tool_button_new(group));
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(button),
						area);
		gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(button),
					       _(CHIT_TOOLBAR_TOOLTIP[i]));

		toolbar_button_data =
		    g_malloc(sizeof(*toolbar_button_data));
		toolbar_button_data->type = CHIT_TOOLBAR_BUTTON_TYPE;
		toolbar_button_data->element.chit = i;

		g_signal_connect(G_OBJECT(button), "clicked",
				 G_CALLBACK(toolbar_click),
				 toolbar_button_data);
	}

	gtk_container_add(GTK_CONTAINER(flowbox), box);

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	/* port toolbar buttons */
	for (i = 0; i <= ANY_RESOURCE; i++) {
		area = gtk_drawing_area_new();

		gtk_widget_show(area);
		gtk_widget_set_size_request(area, BUTTON_HEIGHT,
					    BUTTON_HEIGHT);
		g_signal_connect(G_OBJECT(area), "draw",
				 G_CALLBACK(draw_port_cb),
				 GINT_TO_POINTER(i));

		group =
		    gtk_radio_tool_button_get_group(GTK_RADIO_TOOL_BUTTON
						    (button));
		button = GTK_WIDGET(gtk_radio_tool_button_new(group));
		gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(button),
						area);
		gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(button),
					       _(PORT_TOOLBAR_TOOLTIP[i]));

		toolbar_button_data =
		    g_malloc(sizeof(*toolbar_button_data));
		toolbar_button_data->type = PORT_TOOLBAR_BUTTON_TYPE;
		toolbar_button_data->element.port = i;

		g_signal_connect(G_OBJECT(button), "clicked",
				 G_CALLBACK(toolbar_click),
				 toolbar_button_data);
	}

	gtk_widget_set_hexpand(box, TRUE);	/* Fill remaining space */
	gtk_container_add(GTK_CONTAINER(flowbox), box);

	gtk_widget_set_hexpand(flowbox, TRUE);	/* Fill remaining space */
	gtk_grid_attach(GTK_GRID(grid), flowbox, 0, 0, 2, 1);
}

static void build_map_resize(GtkWidget * grid, guint col, guint row,
			     GtkOrientation dir, GtkWidget ** buttons,
			     GCallback resize_callback)
{
	/* symbols[] must match order of hresize_type and vresize_type; */
	static const gchar *symbols[] =
	    { "list-add", "list-remove", "list-add", "list-remove" };
	/* The order must match hresize_type and vresize_type, and also depends on the orientation */
	static const gchar *tooltip[] = {
		N_("Insert a row"),
		N_("Delete a row"),
		N_("Insert a column"),
		N_("Delete a column")
	};

	GtkWidget *box;
	gint i;

	box = gtk_box_new(dir, 0);
	for (i = 0; i < 4; i++) {
		buttons[i] = GTK_WIDGET(gtk_tool_button_new(NULL, NULL));
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(buttons[i]),
					      symbols[i]);
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(buttons[i]),
					       _(tooltip
						 [i % 2 +
						  (dir ==
						   GTK_ORIENTATION_VERTICAL
						   ? 0 : 2)]));

		if (i < 2) {
			gtk_box_pack_start(GTK_BOX(box), buttons[i],
					   FALSE, TRUE, 0);
		} else {
			gtk_box_pack_end(GTK_BOX(box), buttons[i],
					 FALSE, TRUE, 0);
		}
		g_signal_connect(G_OBJECT(buttons[i]), "clicked",
				 resize_callback, GINT_TO_POINTER(i));
	}
	gtk_box_pack_start(GTK_BOX(box), gtk_fixed_new(), TRUE, TRUE, 0);

	gtk_grid_attach(GTK_GRID(grid), box, col, row, 1, 1);
}

static void scale_map(GuiMap * gmap)
{
	GtkAllocation allocation;

	gtk_widget_get_allocation(gmap->area, &allocation);
	guimap_scale_to_size(gmap, allocation.width, allocation.height);
	gtk_widget_queue_draw(gmap->area);
}

static gboolean enter_notify_map_cb(GtkWidget * widget,
				    GdkEventCrossing * event,
				    G_GNUC_UNUSED gpointer user_data)
{
	if (event->mode != GDK_CROSSING_GTK_UNGRAB) {
		/* Do not grab the focus when a ScrollableTextView
		 * is focussed on another tab page */
		gtk_widget_grab_focus(widget);
	}
	return FALSE;
}

static gint button_press_map_cb(GtkWidget * area,
				GdkEventButton * event, gpointer user_data)
{
	GuiMap *gmap = user_data;
	GtkWidget *menu;
	const Hex *adjacent;
	gboolean port_ok;
	gint num_ports;
	gint i;
	guint idx;

	if (gtk_widget_get_window(area) == NULL || gmap->map == NULL)
		return FALSE;

	gmap->last_x = event->x;
	gmap->last_y = event->y;
	current_hex = guimap_get_current_hex(gmap);
	if (current_hex == NULL)
		return TRUE;

	menu = NULL;
	if (event->button == 1) {
		MapElement element;
		Node *current_node;
		gint distance_node;
		gint distance_hex;

		switch (selected_toolbar_button.type) {
		case TERRAIN_TOOLBAR_BUTTON_TYPE:
			change_terrain(current_hex,
				       selected_toolbar_button.
				       element.terrain);
			return TRUE;
		case CHIT_TOOLBAR_BUTTON_TYPE:
			if (terrain_has_chit(current_hex->terrain)) {
				current_hex->roll =
				    selected_toolbar_button.element.chit;
				guimap_draw_hex(gmap, current_hex);
			}
			return TRUE;
		case PORT_TOOLBAR_BUTTON_TYPE:
			if (current_hex->terrain == SEA_TERRAIN) {
				/* If selected resource is NO_RESOURCE,
				 * remove port from hex regardless of
				 * nearest edge.
				 */
				if (selected_toolbar_button.element.port ==
				    NO_RESOURCE) {
					current_hex->resource =
					    NO_RESOURCE;
					guimap_draw_hex(gmap, current_hex);
					return TRUE;
				}

				/* Find nearest edge. This determines the
				 * direction of the port.
				 */
				const Edge *edge;
				edge = guimap_get_current_edge(gmap);
				gint i;
				for (i = 0; i < 6; i++)
					if (current_hex->edges[i] == edge)
						break;

				/* Verify adjacent hex is a land hex */
				adjacent =
				    hex_in_direction(current_hex, i);
				if (adjacent != NULL
				    && adjacent->terrain != LAST_TERRAIN
				    && adjacent->terrain != SEA_TERRAIN) {
					/* change hex resources and direction */
					current_hex->facing = i;
					current_hex->resource =
					    selected_toolbar_button.
					    element.port;
					guimap_draw_hex(gmap, current_hex);
				}
			}
			return TRUE;
		case NO_TOOLBAR_BUTTON_TYPE:
			break;
		}

		current_node = guimap_get_current_node(gmap);
		element.node = current_node;
		distance_node =
		    guimap_distance_cursor(gmap, &element, MAP_NODE);
		element.hex = current_hex;
		distance_hex =
		    guimap_distance_cursor(gmap, &element, MAP_HEX);

		if (distance_node < distance_hex) {
			current_node->no_setup = !current_node->no_setup;
			for (idx = 0;
			     idx < G_N_ELEMENTS(current_node->hexes);
			     idx++) {
				guimap_draw_hex(gmap,
						current_node->hexes[idx]);
			}
			return TRUE;
		} else {
			menu = terrain_menu;
		}
	} else if (event->button == 3 && current_hex->roll > 0) {
		menu = roll_menu;
		for (i = 2; i <= 12; i++) {
			if (roll_numbers[i] != NULL)
				gtk_check_menu_item_set_active
				    (GTK_CHECK_MENU_ITEM(roll_numbers[i]),
				     current_hex->roll == i);
		}
		gtk_check_menu_item_set_active(shuffle_tile,
					       current_hex->shuffle);
	} else if (event->button == 3
		   && current_hex->terrain == SEA_TERRAIN) {
		num_ports = 0;
		for (i = 0; i < 6; i++) {
			adjacent = hex_in_direction(current_hex, i);
			port_ok = FALSE;
			if (adjacent != NULL &&
			    adjacent->terrain != LAST_TERRAIN &&
			    adjacent->terrain != SEA_TERRAIN) {
				num_ports++;
				if (current_hex->resource != NO_RESOURCE)
					port_ok = TRUE;
			}
			gtk_widget_set_sensitive(port_directions[i],
						 port_ok);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
						       (port_directions
							[i]),
						       current_hex->facing
						       == i && port_ok);
		}

		if (num_ports > 0)
			menu = port_menu;
	}

	if (menu != NULL) {
		gtk_menu_popup_at_pointer(GTK_MENU(menu),
					  (GdkEvent *) event);
	}

	return TRUE;
}

static gint key_press_map_cb(GtkWidget * area, GdkEventKey * event,
			     gpointer user_data)
{
	static Hex *last_hex = NULL;
	static gchar *last_key = NULL;
	GuiMap *gmap = user_data;
	gboolean plus10;

	if (gtk_widget_get_window(area) == NULL || gmap->map == NULL)
		return FALSE;

	get_mouse_position(area, &gmap->last_x, &gmap->last_y);

	last_hex = current_hex;
	current_hex = guimap_get_current_hex(gmap);
	if (current_hex == NULL || !terrain_has_chit(current_hex->terrain))
		return TRUE;

	if (last_hex == current_hex && g_strcmp0(last_key, "1") == 0)
		plus10 = TRUE;
	else
		plus10 = FALSE;

	if (!plus10 && strcmp(event->string, "2") == 0)
		current_hex->roll = 2;
	else if (strcmp(event->string, "3") == 0)
		current_hex->roll = 3;
	else if (strcmp(event->string, "4") == 0)
		current_hex->roll = 4;
	else if (strcmp(event->string, "5") == 0)
		current_hex->roll = 5;
	else if (strcmp(event->string, "6") == 0)
		current_hex->roll = 6;
	else if (strcmp(event->string, "8") == 0)
		current_hex->roll = 8;
	else if (strcmp(event->string, "9") == 0)
		current_hex->roll = 9;
	else if (plus10 && strcmp(event->string, "0") == 0)
		current_hex->roll = 10;
	else if (plus10 && strcmp(event->string, "1") == 0)
		current_hex->roll = 11;
	else if (plus10 && strcmp(event->string, "2") == 0)
		current_hex->roll = 12;
	guimap_draw_hex(gmap, current_hex);
	g_free(last_key);
	last_key = g_strdup(event->string);
	return TRUE;
}

static void update_resize_buttons(void)
{
	gboolean can_insert_left;
	gboolean can_insert_right;

	gtk_widget_set_sensitive(hresize_buttons[RESIZE_REMOVE_LEFT],
				 gmap->map->x_size > 1 ||
				 !(gmap->map->shrink_right ||
				   gmap->map->shrink_left));
	gtk_widget_set_sensitive(hresize_buttons[RESIZE_REMOVE_RIGHT],
				 gmap->map->x_size > 1 ||
				 !(gmap->map->shrink_right ||
				   gmap->map->shrink_left));
	/* Logic table:
	 * x ? SIZE shrink  result
	 *   <        X       T (not max)
	 *   =        F       F (completely full)
	 *   =        T       shrink (only on one side)
	 */
	if (gmap->map->x_size < MAP_SIZE) {
		/* Not at maximum size */
		can_insert_left = TRUE;
		can_insert_right = TRUE;
	} else if (!(gmap->map->shrink_left || gmap->map->shrink_right)) {
		/* Completely full */
		can_insert_left = FALSE;
		can_insert_right = FALSE;
	} else {
		/* Only room on one side */
		can_insert_left = gmap->map->shrink_left;
		can_insert_right = gmap->map->shrink_right;
	}
	gtk_widget_set_sensitive(hresize_buttons[RESIZE_INSERT_LEFT],
				 can_insert_left);
	gtk_widget_set_sensitive(hresize_buttons[RESIZE_INSERT_RIGHT],
				 can_insert_right);

	gtk_widget_set_sensitive(vresize_buttons[RESIZE_REMOVE_TOP],
				 gmap->map->y_size > 1 &&
				 (gmap->map->x_size < MAP_SIZE ||
				  gmap->map->shrink_left));
	gtk_widget_set_sensitive(vresize_buttons[RESIZE_REMOVE_BOTTOM],
				 gmap->map->y_size > 1);
	gtk_widget_set_sensitive(vresize_buttons[RESIZE_INSERT_TOP],
				 gmap->map->y_size < MAP_SIZE
				 && (gmap->map->x_size < MAP_SIZE
				     || gmap->map->shrink_left));
	gtk_widget_set_sensitive(vresize_buttons[RESIZE_INSERT_BOTTOM],
				 gmap->map->y_size < MAP_SIZE);

	scale_map(gmap);
	guimap_display(gmap);
}

static void change_height(G_GNUC_UNUSED GtkWidget * menu,
			  gpointer user_data)
{
	switch (GPOINTER_TO_INT(user_data)) {
	case RESIZE_REMOVE_BOTTOM:
		map_modify_row_count(gmap->map, MAP_MODIFY_REMOVE,
				     MAP_MODIFY_ROW_BOTTOM);
		break;
	case RESIZE_INSERT_BOTTOM:
		map_modify_row_count(gmap->map, MAP_MODIFY_INSERT,
				     MAP_MODIFY_ROW_BOTTOM);
		break;
	case RESIZE_REMOVE_TOP:
		map_modify_row_count(gmap->map, MAP_MODIFY_REMOVE,
				     MAP_MODIFY_ROW_TOP);
		break;
	case RESIZE_INSERT_TOP:
		map_modify_row_count(gmap->map, MAP_MODIFY_INSERT,
				     MAP_MODIFY_ROW_TOP);
		break;
	}
	update_resize_buttons();
}

static void change_width(G_GNUC_UNUSED GtkWidget * menu,
			 gpointer user_data)
{
	switch (GPOINTER_TO_INT(user_data)) {
	case RESIZE_REMOVE_RIGHT:
		map_modify_column_count(gmap->map, MAP_MODIFY_REMOVE,
					MAP_MODIFY_COLUMN_RIGHT);
		break;
	case RESIZE_INSERT_RIGHT:
		map_modify_column_count(gmap->map, MAP_MODIFY_INSERT,
					MAP_MODIFY_COLUMN_RIGHT);
		break;
	case RESIZE_REMOVE_LEFT:
		map_modify_column_count(gmap->map, MAP_MODIFY_REMOVE,
					MAP_MODIFY_COLUMN_LEFT);
		break;
	case RESIZE_INSERT_LEFT:
		map_modify_column_count(gmap->map, MAP_MODIFY_INSERT,
					MAP_MODIFY_COLUMN_LEFT);
		break;
	}
	update_resize_buttons();

}

static GtkWidget *build_map(void)
{
	GtkWidget *grid;
	GtkWidget *area;

	grid = gtk_grid_new();

	gmap = guimap_new();
	guimap_set_show_no_setup_nodes(gmap, TRUE);
	area = guimap_build_drawingarea(gmap, MAP_WIDTH, MAP_HEIGHT);

	gtk_widget_set_can_focus(area, TRUE);
	gtk_widget_add_events(gmap->area, GDK_ENTER_NOTIFY_MASK
			      | GDK_BUTTON_PRESS_MASK
			      | GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(gmap->area), "enter_notify_event",
			 G_CALLBACK(enter_notify_map_cb), gmap);
	g_signal_connect(G_OBJECT(gmap->area), "button_press_event",
			 G_CALLBACK(button_press_map_cb), gmap);
	g_signal_connect(G_OBJECT(gmap->area), "key_press_event",
			 G_CALLBACK(key_press_map_cb), gmap);
	gtk_grid_attach(GTK_GRID(grid), gmap->area, 0, 1, 1, 1);
	gtk_widget_set_hexpand(gmap->area, TRUE);
	gtk_widget_set_vexpand(gmap->area, TRUE);

	build_select_bars(grid);
	build_map_resize(grid, 1, 1, GTK_ORIENTATION_VERTICAL,
			 vresize_buttons, G_CALLBACK(change_height));
	build_map_resize(grid, 0, 2, GTK_ORIENTATION_HORIZONTAL,
			 hresize_buttons, G_CALLBACK(change_width));

	return grid;
}

static gboolean update_title(GtkWidget * widget,
			     G_GNUC_UNUSED GdkEvent event,
			     G_GNUC_UNUSED gpointer user_data)
{
	set_window_title(gtk_entry_get_text(GTK_ENTRY(widget)));
	return FALSE;
}

/** Builds the comments tab.
 * @return The comments tab.
 */
static GtkWidget *build_comments(void)
{
	GtkWidget *vbox;
	GtkWidget *widget;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	/* Label */
	widget = gtk_label_new_with_mnemonic(_("_Title"));
	gtk_widget_show(widget);
	gtk_label_set_xalign(GTK_LABEL(widget), 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

	game_title = gtk_entry_new();
	gtk_widget_show(game_title);
	gtk_box_pack_start(GTK_BOX(vbox), game_title, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(widget), game_title);
	g_signal_connect(G_OBJECT(game_title), "focus-out-event",
			 G_CALLBACK(update_title), NULL);

	/* Label */
	widget = gtk_label_new_with_mnemonic(_("_Description"));
	gtk_widget_show(widget);
	gtk_label_set_xalign(GTK_LABEL(widget), 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

	game_description = scrollable_text_view_new();
	gtk_widget_show(game_description);
	gtk_box_pack_start(GTK_BOX(vbox), game_description, FALSE, FALSE,
			   0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(widget),
				      scrollable_text_view_get_for_mnemonic
				      (SCROLLABLE_TEXT_VIEW
				       (game_description)));

	scrollable_text_view_set_text(SCROLLABLE_TEXT_VIEW
				      (game_description), "");

	/* Label */
	widget = gtk_label_new_with_mnemonic(_("_Comments"));
	gtk_widget_show(widget);
	gtk_label_set_xalign(GTK_LABEL(widget), 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

	game_comments = scrollable_text_view_new();
	gtk_widget_show(game_comments);
	gtk_box_pack_start(GTK_BOX(vbox), game_comments, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(widget),
				      scrollable_text_view_get_for_mnemonic
				      (SCROLLABLE_TEXT_VIEW
				       (game_comments)));

	return vbox;
}

static gint select_terrain_cb(G_GNUC_UNUSED GtkWidget * menu,
			      gpointer user_data)
{
	Terrain terrain = GPOINTER_TO_INT(user_data);
	change_terrain(current_hex, terrain);
	return TRUE;
}

static GtkWidget *build_terrain_menu(void)
{
	gint i;
	GtkWidget *menu;
	GtkWidget *item;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	gint width;
	gint height;
	MapTheme *theme = theme_get_current();

	menu = gtk_menu_new();

	for (i = 0; i <= LAST_TERRAIN; i++) {
		item =
		    gtk_image_menu_item_new_with_mnemonic(gettext
							  (terrain_names
							   [i]));

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				 G_CALLBACK(select_terrain_cb),
				 GINT_TO_POINTER(i));

		if (i == LAST_TERRAIN)
			continue;

		/* Use the default size, or smaller */
		gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);
		if (height > width / theme->scaledata[i].aspect) {
			height = width / theme->scaledata[i].aspect;
		} else {
			width = height * theme->scaledata[i].aspect;
		}

		pixbuf =
		    gdk_pixbuf_scale_simple(theme->
					    scaledata[i].native_image,
					    width, height,
					    GDK_INTERP_BILINEAR);

		image = gtk_image_new_from_pixbuf(pixbuf);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					      image);
	}

	gtk_widget_show_all(menu);
	return menu;
}

static void select_roll_cb(GtkCheckMenuItem * menu_item,
			   gpointer user_data)
{
	if (gtk_check_menu_item_get_active(menu_item)) {
		current_hex->roll = GPOINTER_TO_INT(user_data);
		guimap_draw_hex(gmap, current_hex);
	}
}

static void select_shuffle_cb(GtkCheckMenuItem * menu_item,
			      G_GNUC_UNUSED gpointer user_data)
{
	current_hex->shuffle = gtk_check_menu_item_get_active(menu_item);
}

static GtkWidget *build_roll_menu(void)
{
	GtkWidget *menu;
	gint i;
	gchar buffer[128];
	MapTheme *theme = theme_get_current();
	THEME_COLOR tcolor;
	GdkRGBA *colorFG;
	GdkRGBA *colorBG;
	GtkWidget *item;
	GtkWidget *label;

	menu = gtk_menu_new();

	for (i = 2; i <= 12; i++) {
		if (i == 7)
			continue;

		tcolor = (i == 6 || i == 8) ? TC_CHIP_H_FG : TC_CHIP_FG;
		colorFG =
		    theme->ovr_colors[SEA_TERRAIN][tcolor].
		    set ? &theme->ovr_colors[SEA_TERRAIN][tcolor].
		    color : &theme->colors[tcolor].color;
		tcolor = TC_CHIP_BG;
		colorBG =
		    theme->ovr_colors[SEA_TERRAIN][tcolor].
		    set ? &theme->ovr_colors[SEA_TERRAIN][tcolor].
		    color : &theme->colors[tcolor].color;
		sprintf(buffer,
			"<span foreground=\"#%04x%04x%04x\" background=\"#%04x%04x%04x\">%d</span>",
			(unsigned) (colorFG->red * 255 * 256),
			(unsigned) (colorFG->green * 255 * 256),
			(unsigned) (colorFG->blue * 255 * 256),
			(unsigned) (colorBG->red * 255 * 256),
			(unsigned) (colorBG->green * 255 * 256),
			(unsigned) (colorBG->blue * 255 * 256), i);

		item = gtk_check_menu_item_new();
		label = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(label), buffer);
		gtk_container_add(GTK_CONTAINER(item), label);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "toggled",
				 G_CALLBACK(select_roll_cb),
				 GINT_TO_POINTER(i));
		gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM
						      (item), TRUE);
		roll_numbers[i] = item;
	}

	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      gtk_separator_menu_item_new());

	/* Menu item */
	item = gtk_check_menu_item_new_with_label(_("Shuffle"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "toggled",
			 G_CALLBACK(select_shuffle_cb), NULL);
	shuffle_tile = GTK_CHECK_MENU_ITEM(item);

	gtk_widget_show_all(menu);
	return menu;
}

static gint select_port_resource_cb(G_GNUC_UNUSED GtkWidget * menu,
				    gpointer user_data)
{
	gint i;

	if (current_hex->resource == NO_RESOURCE) {
		for (i = 0; i < 6; i++) {
			const Hex *adjacent;

			adjacent = hex_in_direction(current_hex, i);
			if (adjacent != NULL &&
			    adjacent->terrain != LAST_TERRAIN &&
			    adjacent->terrain != SEA_TERRAIN) {
				current_hex->facing = i;
				break;
			}
		}
	}
	current_hex->resource = GPOINTER_TO_INT(user_data);
	guimap_draw_hex(gmap, current_hex);
	return TRUE;
}

static void select_port_direction_cb(GtkCheckMenuItem * menu_item,
				     gpointer user_data)
{
	if (gtk_check_menu_item_get_active(menu_item)) {
		current_hex->facing = GPOINTER_TO_INT(user_data);
		guimap_draw_hex(gmap, current_hex);
	}
}

static GtkWidget *build_port_menu(void)
{
	gint i;
	GtkWidget *item;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	GtkWidget *menu;
	MapTheme *theme = theme_get_current();

	menu = gtk_menu_new();

	for (i = 0; i <= ANY_RESOURCE; i++) {
		item =
		    gtk_image_menu_item_new_with_mnemonic(gettext
							  (port_names[i]));

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				 G_CALLBACK(select_port_resource_cb),
				 GINT_TO_POINTER(i));

		if (i >= NO_RESOURCE)
			continue;
		pixbuf = theme->port_tiles[i];
		if (pixbuf == NULL)
			continue;

		image = gtk_image_new_from_pixbuf(pixbuf);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					      image);
	}
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      gtk_separator_menu_item_new());
	for (i = 0; i < 6; i++) {
		item =
		    gtk_check_menu_item_new_with_label(Q_
						       (port_direction_names
							[i]));
		gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM
						      (item), TRUE);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "toggled",
				 G_CALLBACK(select_port_direction_cb),
				 GINT_TO_POINTER(i));
		port_directions[i] = item;
	}
	gtk_widget_show_all(menu);

	return menu;
}

static GtkWidget *build_settings(GtkWindow * main_window)
{
	/* vbox */
	/*      outer_hbox */
	/*              inner_hbox */
	/*                      lvbox */
	/*                      fix */
	/*              inner_hbox */
	/*                      vsep */
	/*                      rvbox */

	GtkWidget *vbox;
	GtkWidget *outer_hbox;
	GtkWidget *inner_hbox;
	GtkWidget *lvbox;
	GtkWidget *fix;
	GtkWidget *vsep;
	GtkWidget *rvbox;


	/* vbox */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	/*outer_hbox */
	outer_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(outer_hbox), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), outer_hbox, TRUE, TRUE, 0);

	/* inner_hbox */
	inner_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(outer_hbox), inner_hbox, TRUE, TRUE, 0);

	/* lvbox */
	lvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_box_pack_start(GTK_BOX(inner_hbox), lvbox, TRUE, TRUE, 0);

	/* fix */
	fix = gtk_fixed_new();
	gtk_box_pack_start(GTK_BOX(inner_hbox), fix, FALSE, TRUE, 0);

	/* inner_hbox */
	inner_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(outer_hbox), inner_hbox, TRUE, TRUE, 0);

	/* vsep */
	vsep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	gtk_widget_show(vsep);
	gtk_box_pack_start(GTK_BOX(inner_hbox), vsep, FALSE, FALSE, 0);

	/* rvbox */
	rvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_box_pack_start(GTK_BOX(inner_hbox), rvbox, TRUE, TRUE, 0);

	/* get elements */
	game_settings = GAMESETTINGS(game_settings_new(TRUE));
	game_rules = GAMERULES(game_rules_new());
	game_resources = GAMERESOURCES(game_resources_new());
	game_devcards = GAMEDEVCARDS(game_devcards_new());
	game_buildings = GAMEBUILDINGS(game_buildings_new());

	/* Caption */
	build_frame(lvbox, _("Game parameters"),
		    GTK_WIDGET(game_settings), FALSE);
	/* Caption */
	build_frame(lvbox, _("Rules"), GTK_WIDGET(game_rules), FALSE);
	/* Caption */
	build_frame(lvbox, _("Resources"), GTK_WIDGET(game_resources),
		    FALSE);
	/* Caption */
	build_frame(rvbox, _("Buildings"), GTK_WIDGET(game_buildings),
		    FALSE);
	/* Caption */
	build_frame(rvbox, _("Development cards"),
		    GTK_WIDGET(game_devcards), FALSE);

	g_signal_connect(G_OBJECT(game_settings), "check",
			 G_CALLBACK(check_vp_cb), main_window);
	return vbox;
}

void set_window_title(const gchar * title)
{
	gchar *str;
	g_free(window_title);
	if (title == NULL) {
		/* The game in the editor does not have a name yet */
		title = _("Untitled");
		window_title = NULL;
	} else
		window_title = g_strdup(title);
	/* Application caption */
	str = g_strdup_printf("%s: %s", _("Pioneers Editor"), title);

	gtk_window_set_title(GTK_WINDOW(toplevel), str);
	g_free(str);

	gtk_entry_set_text(GTK_ENTRY(game_title), window_title);
}

static void apply_params(const GameParams * params)
{
	gint i;

	set_window_title(params->title);

	game_rules_set_random_terrain(game_rules, params->random_terrain);
	game_rules_set_strict_trade(game_rules, params->strict_trade);
	game_rules_set_domestic_trade(game_rules, params->domestic_trade);
	game_settings_set_players(game_settings, params->num_players);
	game_rules_set_sevens_rule(game_rules, params->sevens_rule);
	game_rules_set_use_dice_deck(game_rules, params->use_dice_deck);
	game_rules_set_num_dice_decks(game_rules, params->num_dice_decks);
	game_rules_set_num_removed_dice_cards(game_rules,
					      params->num_removed_dice_cards);
	game_settings_set_victory_points(game_settings,
					 params->victory_points);
	/* check_victory_at_end_of_turn not needed in the editor */
	for (i = 1; i < NUM_BUILD_TYPES; i++)
		game_buildings_set_num_buildings(game_buildings, i,
						 params->num_build_type
						 [i]);
	game_resources_set_num_resources(game_resources,
					 params->resource_count);

	for (i = 0; i < NUM_DEVEL_TYPES; i++)
		game_devcards_set_num_cards(game_devcards, i,
					    params->num_develop_type[i]);

	/* Do not disable the pirate rule if currently no ships are present */
	game_rules_set_use_pirate(game_rules, params->use_pirate, 1);
	game_rules_set_island_discovery_bonus(game_rules,
					      params->island_discovery_bonus);
	scrollable_text_view_set_text(SCROLLABLE_TEXT_VIEW(game_comments),
				      params->comments);
	scrollable_text_view_set_text(SCROLLABLE_TEXT_VIEW
				      (game_description),
				      params->description);

	map_free(gmap->map);
	gmap->map = map_copy(params->map);
}

/** Returns params found in game_rules, game_settings, and other settings.
 *  @return Params found in settings.
 */
static GameParams *get_params(void)
{
	GameParams *params = params_new();
	gint i;

	params->title = g_strdup(window_title);

	params->random_terrain = game_rules_get_random_terrain(game_rules);
	params->strict_trade = game_rules_get_strict_trade(game_rules);
	params->domestic_trade = game_rules_get_domestic_trade(game_rules);
	params->num_players = game_settings_get_players(game_settings);
	params->sevens_rule = game_rules_get_sevens_rule(game_rules);
	params->use_dice_deck = game_rules_get_use_dice_deck(game_rules);
	params->num_dice_decks = game_rules_get_num_dice_decks(game_rules);
	params->num_removed_dice_cards =
	    game_rules_get_num_removed_dice_cards(game_rules);
	params->victory_points =
	    game_settings_get_victory_points(game_settings);

	/* check_victory_at_end_of_turn not needed in the editor */
	for (i = 1; i < NUM_BUILD_TYPES; i++)
		params->num_build_type[i] =
		    game_buildings_get_num_buildings(game_buildings, i);
	params->resource_count =
	    game_resources_get_num_resources(game_resources);
	for (i = 0; i < NUM_DEVEL_TYPES; i++)
		params->num_develop_type[i] =
		    game_devcards_get_num_cards(game_devcards, i);
	params->use_pirate = game_rules_get_use_pirate(game_rules);
	params->island_discovery_bonus =
	    game_rules_get_island_discovery_bonus(game_rules);
	params->comments =
	    scrollable_text_view_get_text(SCROLLABLE_TEXT_VIEW
					  (game_comments));
	params->description =
	    scrollable_text_view_get_text(SCROLLABLE_TEXT_VIEW
					  (game_description));

	params->map = map_copy(gmap->map);
	return params;
}

static void load_game(const gchar * file, gboolean is_reload)
{
	const gchar *gamefile;
	GameParams *params;
	gchar *new_filename;
	gint i;

	if (file == NULL)
		gamefile = default_game;
	else
		gamefile = file;

	params = params_load_file(gamefile);
	if (params == NULL) {
		error_dialog(_("Failed to load '%s'"), file);
		return;
	}

	if (file == NULL) {
		g_free(params->title);
		params->title = g_strdup("Untitled");
		map_free(params->map);
		params->map = map_new();
		for (i = 0; i < 6; i++) {
			map_modify_row_count(params->map,
					     MAP_MODIFY_INSERT,
					     MAP_MODIFY_ROW_BOTTOM);
		}
		for (i = 0; i < 11; i++) {
			map_modify_column_count(params->map,
						MAP_MODIFY_INSERT,
						MAP_MODIFY_COLUMN_RIGHT);
		}
		/* Chits array will be constructed later */
		params->map->chits = NULL;
		new_filename = NULL;
	} else {
		new_filename = g_strdup(file);
		config_set_string("editor/last-game", new_filename);
	}

	guimap_reset(gmap);
	apply_params(params);
	params_free(params);

	if (open_filename != NULL)
		g_free(open_filename);
	open_filename = new_filename;
	map_move_robber(gmap->map, -1, -1);
	fill_map(gmap->map);
	if (is_reload) {
		scale_map(gmap);
		guimap_display(gmap);
	}
	update_resize_buttons();
}

static void save_game(const gchar * file)
{
	GameParams *params = get_params();
	canonicalize_map(params->map);
	if (!params_write_file(params, file))
		error_dialog(_("Failed to save to '%s'"), file);
	else
		config_set_string("editor/last-game", file);
	params_free(params);
}

static void new_game_menu_cb(void)
{
	load_game(NULL, TRUE);
}

static void add_file_filter(GtkFileChooser * file_chooser)
{
	GtkFileFilter *filter;

	filter = gtk_file_filter_new();
	/* Name of the file filter: show only games */
	gtk_file_filter_set_name(filter, _("Games"));
	gtk_file_filter_add_pattern(filter, "*.game");
	gtk_file_chooser_add_filter(file_chooser, filter);

	filter = gtk_file_filter_new();
	/* Name of the file filter: show all files */
	gtk_file_filter_set_name(filter, _("Unfiltered"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(file_chooser, filter);
}

static void load_game_menu_cb(void)
{
	GtkWidget *dialog;
	gchar *directory;

	dialog = gtk_file_chooser_dialog_new(
						    /* Dialog caption */
						    _("Open Game"),
						    GTK_WINDOW(toplevel),
						    GTK_FILE_CHOOSER_ACTION_OPEN,
						    /* Button text */
						    _("_Cancel"),
						    GTK_RESPONSE_CANCEL,
						    /* Button text */
						    _("_Open"),
						    GTK_RESPONSE_OK, NULL);
	directory =
	    g_build_filename(g_get_user_data_dir(), "pioneers", NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
					    directory);
	gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog),
					     directory, NULL);
	gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog),
					     get_pioneers_dir(), NULL);
	g_free(directory);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
				      default_game);
	add_file_filter(GTK_FILE_CHOOSER(dialog));
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		char *file;
		file =
		    gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
						  (dialog));
		load_game(file, TRUE);
		g_free(file);
		scale_map(gmap);
		guimap_display(gmap);
	}
	gtk_widget_destroy(dialog);
}

static void save_as_menu_cb(void)
{
	GtkWidget *dialog;
	gchar *directory;

	dialog = gtk_file_chooser_dialog_new(
						    /* Dialog caption */
						    _("Save As..."),
						    GTK_WINDOW(toplevel),
						    GTK_FILE_CHOOSER_ACTION_SAVE,
						    /* Button text */
						    _("_Cancel"),
						    GTK_RESPONSE_CANCEL,
						    /* Button text */
						    _("Save _As"),
						    GTK_RESPONSE_ACCEPT,
						    NULL);
	directory =
	    g_build_filename(g_get_user_data_dir(), "pioneers", NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
					    directory);
	gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog),
					     directory, NULL);
	g_free(directory);
	add_file_filter(GTK_FILE_CHOOSER(dialog));
	gtk_dialog_set_default_response(GTK_DIALOG(dialog),
					GTK_RESPONSE_ACCEPT);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *file;
		file =
		    gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
						  (dialog));
		save_game(file);
		if (open_filename == NULL)
			open_filename = file;
		else
			g_free(file);
	}
	gtk_widget_destroy(dialog);
}

static void save_game_menu_cb(void)
{
	if (open_filename == NULL)
		save_as_menu_cb();
	else
		save_game(open_filename);
}

static void change_title_menu_cb(void)
{
	GtkWidget *dialog, *vbox, *hbox, *label, *entry;

	dialog = gtk_dialog_new_with_buttons(
						    /* Dialog caption */
						    _("Change Title"),
						    GTK_WINDOW(toplevel),
						    GTK_DIALOG_MODAL |
						    GTK_DIALOG_DESTROY_WITH_PARENT,
						    /* Button text */
						    _("_Cancel"),
						    GTK_RESPONSE_CANCEL,
						    /* Button text */
						    _("_OK"),
						    GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog),
					GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &dialog);
	gtk_widget_realize(dialog);
	gdk_window_set_functions(gtk_widget_get_window(dialog),
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

	/* Label */
	label = gtk_label_new(_("New title:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_label_set_xalign(GTK_LABEL(label), 1.0);

	/* Update the window_title before is it shown */
	set_window_title(gtk_entry_get_text(GTK_ENTRY(game_title)));

	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 60);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(entry), window_title);

	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

	gtk_widget_show_all(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
		set_window_title(gtk_entry_get_text(GTK_ENTRY(entry)));
	gtk_widget_destroy(dialog);
}

static void check_vp_cb(G_GNUC_UNUSED GObject * caller,
			gpointer main_window)
{
	GameParams *params;

	params = get_params();
	check_victory_points(params, main_window);
	params_free(params);
}

static void exit_cb(void)
{
	gtk_main_quit();
}

#ifdef HAVE_HELP
/* Commented out, until the help is written
static void contents_menu_cb(void)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
					GTK_DIALOG_MODAL |
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_WARNING,
					GTK_BUTTONS_CLOSE,
					_("There is no help"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
*/
#endif

static void about_menu_cb(void)
{
	aboutbox_display(GTK_WINDOW(toplevel),
			 /* Caption of about box */
			 _("About Pioneers Game Editor"));
}

/** Toggles full screen mode.
 * @param GtkToggleAction The calling action.
 * @param main_window The window to toggle full screen mode.
*/
static void toggle_full_screen_cb(GtkToggleAction * caller,
				  gpointer main_window)
{
	if (gtk_toggle_action_get_active(caller)) {
		gtk_window_fullscreen(GTK_WINDOW(main_window));
	} else {
		gtk_window_unfullscreen(GTK_WINDOW(main_window));
	}
}

static void zoom_normal_cb(void)
{
	guimap_zoom_normal(gmap);
}

static void zoom_center_map_cb(void)
{
	guimap_zoom_center_map(gmap);
}

static GtkActionEntry entries[] = {
	{"FileMenu", NULL,
	 /* Menu entry */
	 N_("_File"), NULL, NULL, NULL},
	{"ViewMenu", NULL,
	 /* Menu entry */
	 N_("_View"), NULL, NULL, NULL},
	{"HelpMenu", NULL,
	 /* Menu entry */
	 N_("_Help"), NULL, NULL, NULL},

	{"New", NULL,
	 /* Menu entry */
	 N_("_New"), "<control>N",
	 N_("Create a new game"), new_game_menu_cb},
	{"Open", NULL,
	 /* Menu entry */
	 N_("_Open..."), "<control>O",
	 /* Tooltip for Open menu entry */
	 N_("Open an existing game"), load_game_menu_cb},
	{"Save", NULL,
	 /* Menu entry */
	 N_("_Save"), "<control>S",
	 /* Tooltip for Save menu entry */
	 N_("Save game"), save_game_menu_cb},
	{"SaveAs", NULL,
	 /* Menu entry */
	 N_("Save _As..."),
	 "<control><shift>S",
	 /* Tooltip for Save As menu entry */
	 N_("Save as"), save_as_menu_cb},
	{"ChangeTitle", NULL,
	 /* Menu entry */
	 N_("Change _Title"), "<control>T",
	 /* Tooltip for Change Title menu entry */
	 N_("Change game title"), change_title_menu_cb},
	{"CheckVP", NULL,
	 /* Menu entry */
	 N_("_Check Victory Point Target"),
	 NULL,
	 /* Tooltip for Check Victory Point Target menu entry */
	 N_("Check whether the game can be won"), G_CALLBACK(check_vp_cb)},
	{"Quit", NULL,
	 /* Menu entry */
	 N_("_Quit"), "<control>Q",
	 /* Tooltip for Quit menu entry */
	 N_("Quit"), exit_cb},

	{"Full", NULL,
	 /* Menu entry */
	 N_("_Reset"),
	 "<control>0",
	 /* Tooltip for Reset menu entry */
	 N_("View the full map"), zoom_normal_cb},
	{"Center", NULL,
	 /* Menu entry */
	 N_("_Center"), NULL,
	 /* Tooltip for Center menu entry */
	 N_("Center the map"), zoom_center_map_cb},

#ifdef HAVE_HELP
	/* Disable this item, until the help is written
	   {"Contents", NULL, N_("_Contents"), "F1",
	   N_("Contents"), contents_menu_cb},
	 */
#endif
	{"About", NULL,
	 /* Menu entry */
	 N_("_About Pioneers Editor"), NULL,
	 /* Tooltip for About Pioneers Editor menu entry */
	 N_("Information about Pioneers Editor"), about_menu_cb},
};

static GtkToggleActionEntry toggle_entries[] = {
	{"FullScreen", NULL,
	 /* Menu entry */
	 N_("_Fullscreen"),
	 "<alt>Return",
	 /* Tooltip for Fullscreen menu entry */
	 N_("Set window to full screen mode"),
	 G_CALLBACK(toggle_full_screen_cb), FALSE}
};

/* *INDENT-OFF* */
static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='New'/>"
"      <menuitem action='Open'/>"
"      <menuitem action='Save'/>"
"      <menuitem action='SaveAs'/>"
"      <separator/>"
"      <menuitem action='CheckVP'/>"
"      <menuitem action='ChangeTitle'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='FullScreen'/>"
"      <menuitem action='Full'/>"
"      <menuitem action='Center'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
#ifdef HAVE_HELP
/* Disable this menu item, until the help is written
"      <menuitem action='Contents'/>"
"      <separator/>"
*/
#endif
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";
/* *INDENT-ON* */

gchar **filenames;
gboolean show_version = FALSE;

static GOptionEntry commandline_entries[] = {
	{G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY,
	 &filenames,
	 /* Long help for commandline option (editor): filename */
	 N_("Open this file"),
	 /* Commandline option for editor: filename */
	 N_("filename")},
	{"version", '\0', 0, G_OPTION_ARG_NONE, &show_version,
	 /* Commandline option of editor: version */
	 N_("Show version information"), NULL},
	{NULL, '\0', 0, 0, NULL, NULL, NULL}
};

int main(int argc, char *argv[])
{
	gchar *filename;
	gboolean default_used;
	GtkWidget *notebook;
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkAccelGroup *accel_group;
	GError *error = NULL;
	gchar *icon_file;
	GOptionContext *context;

	default_game = g_build_filename(get_pioneers_dir(), "default.game",
					NULL);

	/* set the UI driver to GTK_Driver, since we're using gtk */
	set_ui_driver(&GTK_Driver);

#if ENABLE_NLS
	/* Gtk+ handles the locale, we must bind the translations */
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif

	context =
	    /* Long description in the command line: --help */
	    g_option_context_new(_("- Editor for games of Pioneers"));
	g_option_context_add_main_entries(context, commandline_entries,
					  PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_parse(context, &argc, &argv, &error);
	if (error != NULL) {
		g_print("%s\n", error->message);
		g_error_free(error);
		return 1;
	}
	if (show_version) {
		g_print(_("Pioneers version:"));
		g_print(" ");
		g_print(FULL_VERSION);
		g_print("\n");
		return 0;
	}

	if (filenames != NULL)
		filename = g_strdup(filenames[0]);
	else
		filename = NULL;

	toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(toplevel), "delete_event",
			 G_CALLBACK(exit_cb), NULL);

	action_group = gtk_action_group_new("MenuActions");
	gtk_action_group_set_translation_domain(action_group, PACKAGE);
	gtk_action_group_add_actions(action_group, entries,
				     G_N_ELEMENTS(entries), toplevel);
	gtk_action_group_add_toggle_actions(action_group, toggle_entries,
					    G_N_ELEMENTS(toggle_entries),
					    toplevel);

	ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW(toplevel), accel_group);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_description,
					       -1, &error)) {
		g_message(_("Building menus failed: %s"), error->message);
		g_error_free(error);
		return 1;
	}

	config_init("pioneers-editor");

	icon_file =
	    g_build_filename(DATADIR, "pixmaps", MAINICON_FILE, NULL);
	if (g_file_test(icon_file, G_FILE_TEST_EXISTS)) {
		gtk_window_set_default_icon_from_file(icon_file, NULL);
	} else {
		/* Missing pixmap, main icon file */
		g_warning("Pixmap not found: %s", icon_file);
	}
	g_free(icon_file);

	themes_init();
	colors_init();

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_map(),
				 /* Tab page name */
				 gtk_label_new(_("Map")));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				 build_settings(GTK_WINDOW(toplevel)),
				 /* Tab page name */
				 gtk_label_new(_("Settings")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_comments(),
				 /* Tab page name */
				 gtk_label_new(_("Comments")));

	terrain_menu = build_terrain_menu();
	roll_menu = build_roll_menu();
	port_menu = build_port_menu();

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(toplevel), vbox);

	menubar = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	if (filename == NULL) {
		filename =
		    config_get_string("editor/last-game", &default_used);
		if (default_used
		    || !g_file_test(filename, G_FILE_TEST_EXISTS)) {
			g_free(filename);
			filename = NULL;
		}
	}

	load_game(filename, FALSE);
	g_free(filename);

	gtk_widget_show_all(toplevel);

	gtk_main();

	config_finish();
	guimap_delete(gmap);
	g_free(default_game);

	g_option_context_free(context);
	themes_cleanup();
	return 0;
}
