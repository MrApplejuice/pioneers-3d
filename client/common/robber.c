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
#include <stdio.h>

#include "game.h"
#include "map.h"
#include "client.h"
#include "log.h"
#include "callback.h"

void robber_move_on_map(gint x, gint y)
{
	Map *map = callbacks.get_map();
	Hex *hex = map_hex(map, x, y);
	Hex *old_robber = map_robber_hex(map);

	map_move_robber(map, x, y);

	callbacks.draw_hex(old_robber);
	callbacks.draw_hex(hex);
	callbacks.robber_moved(old_robber, hex);
}

void pirate_move_on_map(gint x, gint y)
{
	Map *map = callbacks.get_map();
	Hex *hex = map_hex(map, x, y);
	Hex *old_pirate = map_pirate_hex(map);

	map_move_pirate(map, x, y);

	callbacks.draw_hex(old_pirate);
	callbacks.draw_hex(hex);
	callbacks.robber_moved(old_pirate, hex);
}

void robber_moved(gint player_num, gint x, gint y, gboolean is_undo)
{
	robber_move_on_map(x, y);
	if (is_undo)
		log_message(MSG_STEAL,
			    _("%s has undone the robber movement.\n"),
			    player_name(player_num, TRUE));
	else
		log_message(MSG_STEAL, _("%s moved the robber.\n"),
			    player_name(player_num, TRUE));
}


void pirate_moved(gint player_num, gint x, gint y, gboolean is_undo)
{
	pirate_move_on_map(x, y);
	if (is_undo)
		log_message(MSG_STEAL,
			    _("%s has undone the pirate movement.\n"),
			    player_name(player_num, TRUE));
	else
		log_message(MSG_STEAL, _("%s moved the pirate.\n"),
			    player_name(player_num, TRUE));
}

void robber_begin_move(gint player_num)
{
	gchar *buffer;
	buffer = g_strdup_printf(_("%s must move the robber."),
				 player_name(player_num, TRUE));
	callbacks.instructions(buffer);
	g_free(buffer);
}
