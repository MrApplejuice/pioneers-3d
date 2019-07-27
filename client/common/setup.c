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
#include <ctype.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "network.h"
#include "log.h"
#include "client.h"

static gboolean double_setup;

gboolean is_setup_double(void)
{
	return double_setup;
}

gboolean setup_can_build_road(void)
{
	if (game_params->num_build_type[BUILD_ROAD] == 0)
		return FALSE;
	if (double_setup) {
		if (build_count_edges() == 2)
			return FALSE;
		return build_count_settlements() < 2
		    || map_can_place_road(callbacks.get_map(),
					  my_player_num());
	} else {
		if (build_count_edges() == 1)
			return FALSE;
		return build_count_settlements() < 1
		    || map_can_place_road(callbacks.get_map(),
					  my_player_num());
	}
}

gboolean setup_can_build_ship(void)
{
	if (game_params->num_build_type[BUILD_SHIP] == 0)
		return FALSE;
	if (double_setup) {
		if (build_count_edges() == 2)
			return FALSE;
		return build_count_settlements() < 2
		    || map_can_place_ship(callbacks.get_map(),
					  my_player_num());
	} else {
		if (build_count_edges() == 1)
			return FALSE;
		return build_count_settlements() < 1
		    || map_can_place_ship(callbacks.get_map(),
					  my_player_num());
	}
}

gboolean setup_can_build_bridge(void)
{
	if (game_params->num_build_type[BUILD_BRIDGE] == 0)
		return FALSE;
	if (double_setup) {
		if (build_count_edges() == 2)
			return FALSE;
		return build_count_settlements() < 2
		    || map_can_place_bridge(callbacks.get_map(),
					    my_player_num());
	} else {
		if (build_count_edges() == 1)
			return FALSE;
		return build_count_settlements() < 1
		    || map_can_place_bridge(callbacks.get_map(),
					    my_player_num());
	}
}

gboolean setup_can_build_settlement(void)
{
	if (game_params->num_build_type[BUILD_SETTLEMENT] == 0)
		return FALSE;
	if (double_setup)
		return build_count_settlements() < 2;
	else
		return build_count_settlements() < 1;
}

gboolean setup_can_finish(void)
{
	if (double_setup)
		return build_count_edges() == 2
		    && build_count_settlements() == 2 && build_is_valid();
	else
		return build_count_edges() == 1
		    && build_count_settlements() == 1 && build_is_valid();
}

/* Place some restrictions on road placement during setup phase
 */
gboolean setup_check_road(const Edge * edge)
{
	return build_can_setup_road(edge, double_setup);
}

/* Place some restrictions on ship placement during setup phase
 */
gboolean setup_check_ship(const Edge * edge)
{
	return build_can_setup_ship(edge, double_setup);
}

/* Place some restrictions on bridge placement during setup phase
 */
gboolean setup_check_bridge(const Edge * edge)
{
	return build_can_setup_bridge(edge, double_setup);
}

/* Place some restrictions on settlement placement during setup phase
 */
gboolean setup_check_settlement(const Node * node)
{
	return build_can_setup_settlement(node, double_setup);
}

void setup_begin(gint player_num)
{
	log_message(MSG_INFO, _("Setup for %s.\n"),
		    player_name(player_num, FALSE));
	player_set_current(player_num);
	if (player_num != my_player_num())
		return;

	double_setup = FALSE;
	build_clear();
}

void setup_begin_double(gint player_num)
{
	log_message(MSG_INFO, _("Double setup for %s.\n"),
		    player_name(player_num, FALSE));
	player_set_current(player_num);
	if (player_num != my_player_num())
		return;

	double_setup = TRUE;
	build_clear();
}
