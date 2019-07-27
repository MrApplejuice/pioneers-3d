/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2004 Roland Clobus <rclobus@bigfoot.com>
 * Copyright (C) 2013 Micah Bunting <amnykon@gmail.com>
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
#include <math.h>

#define IDENTITY_HEIGHT 40
#define IDENTITY_BORDER 5
#define FIRST_BUILD_TYPE BUILD_ROAD
#define LAST_BUILD_TYPE BUILD_CITY_WALL

static GtkWidget *identity_area;
static GuiMap bogus_map;

static int die_num[2];

typedef struct {
	gint row;
	gint left;
	gint right;
} IdentityPosition;

static IdentityPosition identity_positions[LAST_BUILD_TYPE -
					   FIRST_BUILD_TYPE + 1];

typedef struct {
	void (*get_polygon) (const GuiMap * gmap, Polygon * poly);
	 gint(*stock_num) (void);
	GuiEvent gui_event;
} TypeData;

static void identity_get_road_polygon(const GuiMap * gmap, Polygon * poly)
{
	guimap_road_polygon(gmap, NULL, poly);
}

static void identity_get_bridge_polygon(const GuiMap * gmap,
					Polygon * poly)
{
	guimap_bridge_polygon(gmap, NULL, poly);
}

static void identity_get_ship_polygon(const GuiMap * gmap, Polygon * poly)
{
	guimap_ship_polygon(gmap, NULL, poly);
}

static void identity_get_settlement_polygon(const GuiMap * gmap,
					    Polygon * poly)
{
	guimap_settlement_polygon(gmap, NULL, poly);
}

static void identity_get_city_polygon(const GuiMap * gmap, Polygon * poly)
{
	guimap_city_polygon(gmap, NULL, poly);
}

static void identity_get_city_wall_polygon(const GuiMap * gmap,
					   Polygon * poly)
{
	guimap_city_wall_polygon(gmap, NULL, poly);
}

static const TypeData TYPE_DATA[LAST_BUILD_TYPE - FIRST_BUILD_TYPE + 1] = {
	{identity_get_road_polygon, stock_num_roads, GUI_ROAD},
	{identity_get_bridge_polygon, stock_num_bridges, GUI_BRIDGE},
	{identity_get_ship_polygon, stock_num_ships, GUI_SHIP},
	{identity_get_settlement_polygon, stock_num_settlements,
	 GUI_SETTLEMENT},
	{identity_get_city_polygon, stock_num_cities, GUI_CITY},
	{identity_get_city_wall_polygon, stock_num_city_walls,
	 GUI_CITY_WALL},
};

static gint calculate_width(GtkWidget * area, BuildType build_type)
{
	const GameParams *game_params = get_game_params();
	if (game_params->num_build_type[build_type] <= 0) {
		return 0;
	}
	/* calculate poly width */
	GdkPoint points[MAX_POINTS];
	Polygon poly;
	GdkRectangle rect;
	const TypeData *data = &TYPE_DATA[build_type - FIRST_BUILD_TYPE];
	poly.points = points;
	poly.num_points = MAX_POINTS;
	data->get_polygon(&bogus_map, &poly);
	poly_bound_rect(&poly, 0, &rect);

	/* calculate num width */
	char buff[10];
	gint width, height;
	PangoLayout *layout;

	sprintf(buff, "%d", data->stock_num());
	layout = gtk_widget_create_pango_layout(area, buff);
	pango_layout_get_pixel_size(layout, &width, &height);
	g_object_unref(layout);

	return width + rect.width + IDENTITY_BORDER * 3;
}

static void calculate_identity_positions(GtkWidget * area)
{
	const GameParams *game_params = get_game_params();
	gint x_pos = 0;
	gint y_pos = 0;

	GtkAllocation allocation;
	gtk_widget_get_allocation(area, &allocation);

	if (game_params == NULL)
		return;

	BuildType build_type;
	for (build_type = FIRST_BUILD_TYPE; build_type <= LAST_BUILD_TYPE;
	     build_type++) {
		gint width = calculate_width(area, build_type);
		if (x_pos + width > allocation.width - (y_pos ? 0 : 75)) {
			y_pos++;
			x_pos = 0;
		}
		IdentityPosition *position =
		    &identity_positions[build_type - FIRST_BUILD_TYPE];
		position->row = y_pos;
		position->left = x_pos;
		x_pos += width;
		position->right = x_pos;
	}

	if ((y_pos + 1) * IDENTITY_HEIGHT != allocation.height) {
		gtk_widget_set_size_request(area, -1,
					    (y_pos + 1) * IDENTITY_HEIGHT);
	}
}

static void draw_building_and_count(cairo_t * cr, GtkWidget * area,
				    BuildType build_type)
{
	const GameParams *game_params = get_game_params();
	if (game_params->num_build_type[build_type] <= 0) {
		return;
	}
	gint offset_x;
	gint offset_y;

	IdentityPosition *position =
	    &identity_positions[build_type - FIRST_BUILD_TYPE];
	const TypeData *data = &TYPE_DATA[build_type - FIRST_BUILD_TYPE];
	/* draw building */
	GdkPoint points[MAX_POINTS];
	Polygon poly;
	GdkRectangle rect;

	poly.points = points;
	poly.num_points = MAX_POINTS;
	data->get_polygon(&bogus_map, &poly);
	poly_bound_rect(&poly, 0, &rect);
	offset_x = position->left - rect.x + IDENTITY_BORDER;
	offset_y = (1 + position->row) * IDENTITY_HEIGHT
	    - IDENTITY_BORDER - rect.y - rect.height;
	poly_offset(&poly, offset_x, offset_y);
	poly_draw(cr, FALSE, &poly);

	/* draw count */
	char buff[10];
	gint width, height;
	PangoLayout *layout;
	sprintf(buff, "%d", data->stock_num());
	layout = gtk_widget_create_pango_layout(area, buff);
	pango_layout_get_pixel_size(layout, &width, &height);
	offset_x = position->left + IDENTITY_BORDER * 2 + rect.width;
	offset_y = (1 + position->row) * IDENTITY_HEIGHT
	    - IDENTITY_BORDER - height;
	cairo_move_to(cr, offset_x, offset_y);
	pango_cairo_show_layout(cr, layout);
	g_object_unref(layout);
}

static void show_die(cairo_t * cr, gint x_offset, gint num,
		     GdkRGBA * die_border_color, GdkRGBA * die_color,
		     GdkRGBA * die_dots_color)
{
	static GdkPoint die_points[4] = {
		{0, 0}, {30, 0}, {30, 30}, {0, 30}
	};
	static Polygon die_shape =
	    { die_points, G_N_ELEMENTS(die_points) };
	static GdkPoint dot_pos[7] = {
		{7, 7}, {22, 7},
		{7, 15}, {15, 15}, {22, 15},
		{7, 22}, {22, 22}
	};
	static gint draw_list[6][7] = {
		{0, 0, 0, 1, 0, 0, 0},
		{0, 1, 0, 0, 0, 1, 0},
		{1, 0, 0, 1, 0, 0, 1},
		{1, 1, 0, 0, 0, 1, 1},
		{1, 1, 0, 1, 0, 1, 1},
		{1, 1, 1, 0, 1, 1, 1}
	};
	gint y_offset;
	gint *list = draw_list[num - 1];
	gint idx;
	y_offset = IDENTITY_BORDER;
	poly_offset(&die_shape, x_offset, y_offset);
	gdk_cairo_set_source_rgba(cr, die_color);
	poly_draw(cr, TRUE, &die_shape);
	gdk_cairo_set_source_rgba(cr, die_border_color);
	poly_draw(cr, FALSE, &die_shape);
	poly_offset(&die_shape, -x_offset, -y_offset);
	gdk_cairo_set_source_rgba(cr, die_dots_color);
	for (idx = 0; idx < 7; idx++) {
		if (list[idx] == 0)
			continue;
		cairo_move_to(cr, x_offset + dot_pos[idx].x - 3,
			      y_offset + dot_pos[idx].y - 3);
		cairo_arc(cr, x_offset + dot_pos[idx].x,
			  y_offset + dot_pos[idx].y, 3, 0.0, 2 * M_PI);
		cairo_fill(cr);
	}
}

static void identity_resize_cb(GtkWidget * area,
			       G_GNUC_UNUSED GtkAllocation * allocation,
			       G_GNUC_UNUSED gpointer user_data)
{
	calculate_identity_positions(area);
}

static gint draw_identity_area_cb(GtkWidget * widget, cairo_t * cr,
				  G_GNUC_UNUSED gpointer user_data)
{
	GdkRGBA *colour;
	const GameParams *game_params;
	gint i;
	GtkAllocation allocation;
	BuildType build_type;

	if (my_player_num() < 0)
		return FALSE;

	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_width(cr, 1.0);
	colour = player_or_spectator_color(my_player_num());
	gdk_cairo_set_source_rgba(cr, colour);
	gtk_widget_get_allocation(widget, &allocation);
	cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
	cairo_fill(cr);
	if (my_player_spectator())
		colour = &white;
	else
		colour = &black;
	gdk_cairo_set_source_rgba(cr, colour);
	game_params = get_game_params();
	if (game_params == NULL)
		return TRUE;
	for (build_type = FIRST_BUILD_TYPE; build_type <= LAST_BUILD_TYPE;
	     build_type++) {
		draw_building_and_count(cr, widget, build_type);
	}

	if (die_num[0] > 0 && die_num[1] > 0) {
		{
			/* original dice */
			for (i = 0; i < 2; i++)
				show_die(cr,
					 allocation.width - 70 +
					 35 * i, die_num[i], &black,
					 &white, &black);
		}
	}
	return TRUE;
}

void identity_draw(void)
{
	gtk_widget_queue_draw(identity_area);
}

void identity_set_dice(gint die1, gint die2)
{
	die_num[0] = die1;
	die_num[1] = die2;
	gtk_widget_queue_draw(identity_area);
}

static gint button_press_identity_cb(G_GNUC_UNUSED GtkWidget * area,
				     GdkEventButton * event,
				     G_GNUC_UNUSED gpointer user_data)
{
	GtkAllocation allocation;
	gtk_widget_get_allocation(area, &allocation);
	gint row;
	row = event->y / IDENTITY_HEIGHT;
	BuildType build_type;
	for (build_type = FIRST_BUILD_TYPE; build_type <= LAST_BUILD_TYPE;
	     build_type++) {
		IdentityPosition *position =
		    &identity_positions[build_type - FIRST_BUILD_TYPE];
		const TypeData *data =
		    &TYPE_DATA[build_type - FIRST_BUILD_TYPE];
		if (position->row == row && position->left <= event->x
		    && position->right > event->x) {
			if (frontend_gui_get_sensitive(data->gui_event)) {
				route_gui_event(data->gui_event);
			}
			return TRUE;
		}
	}
	if (row == 0 && event->x > allocation.width - 70) {
		if (!have_rolled_dice()) {
			route_gui_event(GUI_ROLL);
		}
		return TRUE;
	}
	return FALSE;
}

GtkWidget *identity_build_panel(void)
{
	identity_area = gtk_drawing_area_new();
	g_signal_connect(G_OBJECT(identity_area), "draw",
			 G_CALLBACK(draw_identity_area_cb), NULL);
	g_signal_connect(G_OBJECT(identity_area), "size-allocate",
			 G_CALLBACK(identity_resize_cb), NULL);
	gtk_widget_add_events(identity_area, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(G_OBJECT(identity_area), "button_press_event",
			 G_CALLBACK(button_press_identity_cb), NULL);
	gtk_widget_set_size_request(identity_area, -1, IDENTITY_HEIGHT);

	guimap_scale_with_radius(&bogus_map, IDENTITY_HEIGHT);

	identity_reset();
	gtk_widget_show(identity_area);
	return identity_area;
}

void identity_reset(void)
{
	gint i;
	for (i = 0; i < 2; i++) {
		die_num[i] = 0;
	};
	if (identity_area != NULL)
		calculate_identity_positions(identity_area);
}
