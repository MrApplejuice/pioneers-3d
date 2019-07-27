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
#include <string.h>
#include "cost.h"
#include "server.h"

gboolean resource_available(Player * player, gint * resources,
			    gint * num_in_bank)
{
	Game *game = player->game;
	gint idx;

	if (num_in_bank != NULL)
		*num_in_bank = 0;
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		if (resources[idx] > game->bank_deck[idx]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR no-cards %r\n", idx);
			return FALSE;
		}
		if (num_in_bank != NULL)
			*num_in_bank += game->bank_deck[idx];
	}

	return TRUE;
}

void resource_start(Game * game)
{
	GList *list;

	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *player = list->data;

		memcpy(player->prev_assets,
		       player->assets, sizeof(player->assets));
		player->gold = 0;
	}
}

void resource_end(Game * game, const gchar * action, gint mult)
{
	GList *list;

	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *player = list->data;
		gint resource[NO_RESOURCE];
		guint idx;
		gboolean send_message = FALSE;

		for (idx = 0; idx < G_N_ELEMENTS(player->assets); idx++) {
			gint num;

			num =
			    player->assets[idx] - player->prev_assets[idx];
			if (game->bank_deck[idx] - num < 0) {
				num = game->bank_deck[idx];
				player->assets[idx]
				    = player->prev_assets[idx] + num;
			}

			resource[idx] = num;
			if (num != 0)
				send_message = TRUE;

			game->bank_deck[idx] -= num;
		}

		if (send_message) {
			for (idx = 0; idx < NO_RESOURCE; idx++)
				resource[idx] *= mult;
			player_broadcast(player, PB_ALL, FIRST_VERSION,
					 LATEST_VERSION, "%s %R\n", action,
					 resource);
		}
	}
}

void resource_spend(Player * player, const gint * cost)
{
	Game *game = player->game;

	resource_start(game);
	cost_buy(cost, player->assets);
	resource_end(game, "spent", -1);
}

void resource_refund(Player * player, const gint * cost)
{
	Game *game = player->game;

	resource_start(game);
	cost_refund(cost, player->assets);
	resource_end(game, "refund", 1);
}
