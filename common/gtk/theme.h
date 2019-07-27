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

#ifndef __theme_h
#define __theme_h

#include <gtk/gtk.h>

typedef struct {
	gboolean set;
	gboolean transparent;
	GdkRGBA color;
} TColor;

typedef struct {
	GdkPixbuf *native_image;
	gint native_width;
	gdouble aspect;
} TScaleData;

typedef enum {
	TC_CHIP_BG = 0,
	TC_CHIP_FG,
	TC_CHIP_BD,
	TC_CHIP_H_BG,
	TC_CHIP_H_FG,
	TC_PORT_BG,
	TC_PORT_FG,
	TC_PORT_BD,
	TC_ROBBER_FG,
	TC_ROBBER_BD,
	TC_HEX_BD,
	TC_MAX
} THEME_COLOR;
#define TC_MAX_OVERRIDE (TC_CHIP_H_FG+1)

/* The order of the TERRAIN_TILES enum is EXTREMELY important!  The order
 * must match the resources indicated in enum Terrain.
 */
typedef enum {
	HILL_TILE = 0,
	FIELD_TILE,
	MOUNTAIN_TILE,
	PASTURE_TILE,
	FOREST_TILE,
	DESERT_TILE,
	SEA_TILE,
	GOLD_TILE,
	BOARD_TILE,
	TERRAIN_TILE_MAX
} TERRAIN_TILES;
#define TC_MAX_OVRTILE (GOLD_TILE+1)

typedef enum {
	HILL_PORT_TILE = 0,
	FIELD_PORT_TILE,
	MOUNTAIN_PORT_TILE,
	PASTURE_PORT_TILE,
	FOREST_PORT_TILE,
	ANY_PORT_TILE,
	PORT_TILE_MAX
} PORT_TILES;

typedef enum {
	NEVER,
	ALWAYS,
	ONLY_DOWNSCALE,
	ONLY_UPSCALE
} SCALEMODE;

typedef struct _MapTheme {
	gchar *name;
	SCALEMODE scaling;
	gint current_width;
	const gchar *terrain_tile_names[TERRAIN_TILE_MAX];
	const gchar *port_tile_names[PORT_TILE_MAX];
	GdkPixbuf *terrain_tiles[TERRAIN_TILE_MAX];
	GdkPixbuf *port_tiles[PORT_TILE_MAX];
	gint port_tiles_width[PORT_TILE_MAX];
	gint port_tiles_height[PORT_TILE_MAX];
	TScaleData scaledata[TERRAIN_TILE_MAX];
	TColor colors[TC_MAX];
	TColor ovr_colors[TC_MAX_OVRTILE][TC_MAX_OVERRIDE];
} MapTheme;

void theme_rescale(int radius);
void theme_set_current(MapTheme * t);
MapTheme *theme_get_current(void);
GList *theme_get_list(void);
void themes_init(void);
void theme_register_callback(GCallback callback);
void themes_cleanup(void);

GdkPixbuf *theme_get_terrain_pixbuf(Terrain terrain);

#endif
