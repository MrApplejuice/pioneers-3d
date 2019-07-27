/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2004-2014 Roland Clobus <rclobus@rclobus.nl>
 * Copyright (C) 2013 Micah Bunting <Amnykon@gmail.com>
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

#ifndef __guimap_h
#define __guimap_h

#include "polygon.h"
#include <gtk/gtk.h>

#define MAX_POINTS 32		/* maximum points in a polygon */
#define MIN_HEX_RADIUS 3	/* minimum hex_radius */

typedef enum {
	NO_CURSOR,
	ROAD_CURSOR,
	SHIP_CURSOR,
	BRIDGE_CURSOR,
	SETTLEMENT_CURSOR,
	CITY_CURSOR,
	CITY_WALL_CURSOR,
	STEAL_BUILDING_CURSOR,
	STEAL_SHIP_CURSOR,
	ROBBER_CURSOR
} CursorType;

typedef enum {
	MAP_EDGE,
	MAP_NODE,
	MAP_HEX
} MapElementType;

typedef union {
	const Hex *hex;
	const Node *node;
	const Edge *edge;
	gconstpointer pointer;
} MapElement;

typedef gboolean(*CheckFunc) (const MapElement element, gint owner,
			      const MapElement user_data);
typedef void (*SelectFunc) (const MapElement obj,
			    const MapElement user_data);
typedef void (*CancelFunc) (void);

typedef struct _Mode Mode;
typedef struct {
	GtkWidget *area;	   /**< render map in this drawing area */
	cairo_surface_t *surface;  /**< off screen surface for drawing */
	cairo_t *cr;		   /**< cairo for map drawing */
	PangoLayout *layout;	   /**< layout object for rendering text */
	gint initial_font_size;	   /**< initial font size */

	Map *map;		   /**< map that is displayed */
	gboolean show_nosetup_nodes; /**< show the nosetup nodes */

	CursorType cursor_type;	   /**< current cursor type */
	gint cursor_owner;	   /**< owner of the cursor */
	CheckFunc check_func;	   /**< check object under cursor */
	SelectFunc check_select;   /**< when user selects cursor */
	CancelFunc check_cancel;   /**< when user clicks in illegal position */
	MapElement user_data;	   /**< passed to callback functions */
	MapElement cursor;	   /**< current GUI mode edge/node/hex cursor */
	gdouble last_x;		   /**< last known cursor position */
	gdouble last_y;		   /**< last known cursor position */

	gint highlight_chit;	   /**< chit number to highlight */
	gint chit_radius;	   /**< radius of the chit */

	gint hex_radius;	   /**< size of hex on display */
	gint x_point;		   /**< x offset of node 0 from centre */
	gint y_point;		   /**< y offset of node 0 from centre */

	gboolean is_custom_view;   /**< false if all hexes are shown and centered */
	gdouble x_margin;	   /**< margin to leave empty */
	gdouble y_margin;	   /**< margin to leave empty */
	gint width;		   /**< pixel width of map */
	gint height;		   /**< pixel height of map */
	gint player_num;	   /**< player displaying this map */
} GuiMap;

GuiMap *guimap_new(void);
void guimap_delete(GuiMap * gmap);
void guimap_reset(GuiMap * gmap);
GtkWidget *guimap_build_drawingarea(GuiMap * gmap, gint width,
				    gint height);

void guimap_road_polygon(const GuiMap * gmap, const Edge * edge,
			 Polygon * poly);
void guimap_ship_polygon(const GuiMap * gmap, const Edge * edge,
			 Polygon * poly);
void guimap_bridge_polygon(const GuiMap * gmap, const Edge * edge,
			   Polygon * poly);
void guimap_city_polygon(const GuiMap * gmap, const Node * node,
			 Polygon * poly);
void guimap_settlement_polygon(const GuiMap * gmap, const Node * node,
			       Polygon * poly);
void guimap_city_wall_polygon(const GuiMap * gmap, const Node * node,
			      Polygon * poly);

gint guimap_get_chit_radius(PangoLayout * layout, gboolean show_dots);
void draw_port_indicator(PangoLayout * layout, cairo_t * cr,
			 gdouble x_offset, gdouble y_offset,
			 gdouble chit_radius, gint resource);
void draw_dice_roll(PangoLayout * layout, cairo_t * cr, gdouble x_offset,
		    gdouble y_offset, gdouble radius, gint n, gint terrain,
		    gboolean highlight);

void guimap_scale_with_radius(GuiMap * gmap, gint radius);
void guimap_scale_to_size(GuiMap * gmap, gint width, gint height);
void guimap_display(GuiMap * gmap);
void guimap_zoom_normal(GuiMap * gmap);
void guimap_zoom_center_map(GuiMap * gmap);

void guimap_highlight_chits(GuiMap * gmap, gint roll);
void guimap_draw_edge(GuiMap * gmap, const Edge * edge);
void guimap_draw_node(GuiMap * gmap, const Node * node);
void guimap_draw_hex(GuiMap * gmap, const Hex * hex);

Edge *guimap_get_current_edge(GuiMap * gmap);
Node *guimap_get_current_node(GuiMap * gmap);
Hex *guimap_get_current_hex(GuiMap * gmap);

gint guimap_distance_cursor(const GuiMap * gmap,
			    const MapElement * element,
			    MapElementType type);

void guimap_cursor_set(GuiMap * gmap, CursorType cursor_type, gint owner,
		       CheckFunc check_func, SelectFunc select_func,
		       CancelFunc cancel_func,
		       const MapElement * user_data,
		       gboolean set_by_single_click);

/* Single click building.
 * Single click building is aborted by explicitly setting the cursor
 * CheckFunc: check function for a certain resource type
 * SelectFunc: function to call when the resource is selected
 */
void guimap_single_click_set_functions(CheckFunc road_check_func,
				       SelectFunc road_select_func,
				       CheckFunc ship_check_func,
				       SelectFunc ship_select_func,
				       CheckFunc bridge_check_func,
				       SelectFunc bridge_select_func,
				       CheckFunc
				       settlement_check_func,
				       SelectFunc
				       settlement_select_func,
				       CheckFunc city_check_func,
				       SelectFunc city_select_func,
				       CheckFunc city_wall_check_func,
				       SelectFunc city_wall_select_func,
				       CheckFunc
				       ship_move_check_func,
				       SelectFunc
				       ship_move_select_func,
				       CancelFunc ship_move_cancel_func);

/* guimap_single_click_set_*_mask:
 * mask to determine whether the CheckFunc or SelectFunc can be used
 */
void guimap_single_click_set_road_mask(gboolean mask);
void guimap_single_click_set_ship_mask(gboolean mask);
void guimap_single_click_set_bridge_mask(gboolean mask);
void guimap_single_click_set_settlement_mask(gboolean mask);
void guimap_single_click_set_city_mask(gboolean mask);
void guimap_single_click_set_city_wall_mask(gboolean mask);
void guimap_single_click_set_ship_move_mask(gboolean mask);

void guimap_cursor_select(GuiMap * gmap);
void guimap_set_show_no_setup_nodes(GuiMap * gmap, gboolean show);
#endif
