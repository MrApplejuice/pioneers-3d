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
#include <math.h>
#include <ctype.h>

#include "game.h"
#include "map.h"
#include "client.h"
#include "callback.h"

static gint num_roads;		/* number of roads available */
static gint num_ships;		/* number of ships available */
static gint num_bridges;	/* number of bridges available */
static gint num_settlements;	/* settlements available */
static gint num_cities;		/* cities available */
static gint num_city_walls;	/* city walls available */
static guint num_develop;	/* development cards left */

void stock_init(void)
{
	guint idx;

	num_roads = game_params->num_build_type[BUILD_ROAD];
	num_ships = game_params->num_build_type[BUILD_SHIP];
	num_bridges = game_params->num_build_type[BUILD_BRIDGE];
	num_settlements = game_params->num_build_type[BUILD_SETTLEMENT];
	num_cities = game_params->num_build_type[BUILD_CITY];
	num_city_walls = game_params->num_build_type[BUILD_CITY_WALL];

	num_develop = 0;
	for (idx = 0; idx < G_N_ELEMENTS(game_params->num_develop_type);
	     idx++)
		num_develop += game_params->num_develop_type[idx];
}

gint stock_num_roads(void)
{
	return num_roads;
}

void stock_use_road(void)
{
	num_roads--;
	callbacks.update_stock();
}

void stock_replace_road(void)
{
	num_roads++;
	callbacks.update_stock();
}

gint stock_num_ships(void)
{
	return num_ships;
}

void stock_use_ship(void)
{
	num_ships--;
	callbacks.update_stock();
}

void stock_replace_ship(void)
{
	num_ships++;
	callbacks.update_stock();
}

gint stock_num_bridges(void)
{
	return num_bridges;
}

void stock_use_bridge(void)
{
	num_bridges--;
	callbacks.update_stock();
}

void stock_replace_bridge(void)
{
	num_bridges++;
	callbacks.update_stock();
}

gint stock_num_settlements(void)
{
	return num_settlements;
}

void stock_use_settlement(void)
{
	num_settlements--;
	callbacks.update_stock();
}

void stock_replace_settlement(void)
{
	num_settlements++;
	callbacks.update_stock();
}

gint stock_num_cities(void)
{
	return num_cities;
}

void stock_use_city(void)
{
	num_cities--;
	callbacks.update_stock();
}

void stock_replace_city(void)
{
	num_cities++;
	callbacks.update_stock();
}

gint stock_num_city_walls(void)
{
	return num_city_walls;
}

void stock_use_city_wall(void)
{
	num_city_walls--;
	callbacks.update_stock();
}

void stock_replace_city_wall(void)
{
	num_city_walls++;
	callbacks.update_stock();
}


guint stock_num_develop(void)
{
	return num_develop;
}

void stock_use_develop(void)
{
	num_develop--;
}

/** Do not count the next development card.
*/
void stock_dont_count_develop(void)
{
	num_develop++;
}
