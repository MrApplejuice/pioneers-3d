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
#include "server.h"
#include "glib-driver.h"

/* callbacks for the server */
void srv_glib_player_added(G_GNUC_UNUSED void *data)
{
#ifdef PRINT_INFO
	Player *player = (Player *) data;
	g_print("Player %d added: %s (from host %s)\n",
		player->num, player->name, player->location);
#endif
}

void srv_glib_player_renamed(G_GNUC_UNUSED void *data)
{
#ifdef PRINT_INFO
	Player *player = (Player *) data;
	g_print("Player %d renamed to %s (at host %s)\n",
		player->num, player->name, player->location);
#endif
}

void srv_player_removed(G_GNUC_UNUSED void *data)
{
#ifdef PRINT_INFO
	Player *player = (Player *) data;
	g_print("Player %d removed: %s (at host %s)\n",
		player->num, player->name, player->location);
#endif
}


void srv_player_change(G_GNUC_UNUSED void *data)
{
#ifdef PRINT_INFO
	GList *current;
	Game *game = (Game *) data;
	g_print("Players connected:\n");
	playerlist_inc_use_count(game);
	for (current = game->player_list; current != NULL;
	     current = g_list_next(current)) {
		Player *p = (Player *) current->data;
		g_print("Player %d: %s (at host %s) is %s connected\n",
			p->num, p->name, p->location,
			p->disconnected ? "not" : "");
	}
	playerlist_dec_use_count(game);
#endif
}
