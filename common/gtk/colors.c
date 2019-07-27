/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2005 Brian Wellington <bwelling@xbill.org>
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
#include <gdk/gdk.h>

#include "colors.h"
#include "game.h"

GdkRGBA black = { 0.0, 0.0, 0.0, 1.0 };
GdkRGBA white = { 1.0, 1.0, 1.0, 1.0 };
GdkRGBA red = { 1.0, 0.0, 0.0, 1.0 };
GdkRGBA green = { 0.0, 1.0, 0.0, 1.0 };
GdkRGBA blue = { 0.0, 0.0, 1.0, 1.0 };
GdkRGBA lightblue = { 0.75, 0.75, 1.0, 1.0 };

GdkRGBA ck_die_red = { 0.53, 0.01, 0.01, 1.0 };
GdkRGBA ck_die_yellow = { 0.67, 0.74, 0.07, 1.0 };

static GdkRGBA token_colors[MAX_PLAYERS] = {
	{0.80, 0.00, 0.00, 1.0},	/* red */
	{0.12, 0.57, 1.00, 1.0},	/* blue */
	{0.91, 0.91, 0.91, 1.0},	/* white */
	{1.00, 0.50, 0.00, 1.0},	/* orange */
	{0.93, 0.93, 0.00, 1.0},	/* yellow */
	{0.55, 0.90, 0.93, 1.0},	/* cyan */
	{0.82, 0.37, 0.93, 1.0},	/* magenta */
	{0.00, 0.93, 0.46, 1.0}	/* green */
};

void colors_init(void)
{
}

GdkRGBA *colors_get_player(gint player_num)
{
	g_assert(player_num >= 0);
	g_assert(player_num < MAX_PLAYERS);
	return &token_colors[player_num];
}
