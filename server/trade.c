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

void trade_perform_maritime(Player * player,
			    gint ratio, Resource supply, Resource receive)
{
	Game *game = player->game;
	const Map *map = game->params->map;
	gint check[NO_RESOURCE];
	MaritimeInfo info;

	if ((!game->rolled_dice) ||
	    ((player->build_list != NULL || game->bought_develop) &&
	     game->params->strict_trade)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR wrong-time\n");
		return;
	}

	if (ratio < 2 || ratio > 4) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR trade bad-ratio\n");
		return;
	}

	/* Now check that the trade can actually be performed.
	 */
	map_maritime_info(map, &info, player->num);
	switch (ratio) {
	case 4:
		if (player->assets[supply] < 4) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR maritime trade, not 4 resources\n");
			return;
		}
		break;
	case 3:
		if (!info.any_resource || player->assets[supply] < 3) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR maritime trade, not 3 resources\n");
			return;
		}
		break;
	case 2:
		if (!info.specific_resource[supply]
		    || player->assets[supply] < 2) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR maritime trade, not 2 resources\n");
			return;
		}
		break;
	}

	memset(check, 0, sizeof(check));
	check[receive] = 1;
	if (!resource_available(player, check, NULL))
		return;

	game->bank_deck[supply] += ratio;
	game->bank_deck[receive]--;
	player->assets[supply] -= ratio;
	player->assets[receive]++;

	player_broadcast(player, PB_RESPOND, FIRST_VERSION, LATEST_VERSION,
			 "maritime-trade %d supply %r receive %r\n",
			 ratio, supply, receive);
}

/* The current player has rejected the trade,
 * wait for the initiating player to make a new request.
 */
gboolean mode_domestic_quote_rejected(Player * player,
				      G_GNUC_UNUSED gint event)
{
	StateMachine *sm = player->sm;

	sm_state_name(sm, "mode_domestic_quote_rejected");
	return FALSE;
}

gboolean mode_domestic_quote(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gint quote_num;
	gint supply[NO_RESOURCE];
	gint receive[NO_RESOURCE];

	sm_state_name(sm, "mode_domestic_quote");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "domestic-quote finish")) {
		/* Player has rejected domestic trade - remove all
		 * quotes from that player
		 */
		for (;;) {
			QuoteInfo *quote;
			quote = quotelist_find_domestic(game->quotes,
							player->num, -1);
			if (quote == NULL)
				break;
			player_broadcast(player, PB_ALL, FIRST_VERSION,
					 LATEST_VERSION,
					 "domestic-quote delete %d\n",
					 quote->var.d.quote_num);
			quotelist_delete(game->quotes, quote);
		}
		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION,
				 "domestic-quote finish\n");

		sm_goto(sm, (StateFunc) mode_domestic_quote_rejected);
		return TRUE;
	}

	if (sm_recv(sm, "domestic-quote delete %d", &quote_num)) {
		/* Player wants to retract a quote
		 */
		QuoteInfo *quote;
		quote = quotelist_find_domestic(game->quotes,
						player->num, quote_num);
		if (quote == NULL) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR quote already deleted\n");
			return TRUE;
		}
		quotelist_delete(game->quotes, quote);
		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION,
				 "domestic-quote delete %d\n", quote_num);
		return TRUE;
	}

	if (sm_recv(sm, "domestic-quote quote %d supply %R receive %R",
		    &quote_num, supply, receive)) {
		/* Make sure that quoting party can satisfy the trade
		 */
		if (!cost_can_afford(supply, player->assets)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR quote not enough resources\n");
			return TRUE;
		}
		/* Make sure that the quote does not already exist
		 */
		if (quotelist_find_domestic
		    (game->quotes, player->num, quote_num) != NULL) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "INFO duplicate quote\n");
			return TRUE;
		}

		quotelist_add_domestic(game->quotes, player->num,
				       quote_num, supply, receive);
		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION,
				 "domestic-quote quote %d supply %R receive %R\n",
				 quote_num, supply, receive);
		return TRUE;
	}

	return FALSE;
}

/* Initiating player wants to end domestic trade
 */
void trade_finish_domestic(Player * player)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	GList *list;

	player_broadcast(player, PB_RESPOND, FIRST_VERSION, LATEST_VERSION,
			 "domestic-trade finish\n");
	sm_pop(sm);
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;
		if (scan != player
		    && !player_is_spectator(game, scan->num))
			sm_pop(scan->sm);
	}
	quotelist_free(&game->quotes);
}

void trade_accept_domestic(Player * player,
			   gint partner_num, gint quote_num,
			   gint * supply, gint * receive)
{
	Game *game = player->game;
	QuoteInfo *quote;
	Player *partner;

	/* Check for valid trade scenario */
	if ((!game->rolled_dice) ||
	    (((player->build_list != NULL) || (game->bought_develop)) &&
	     (game->params->strict_trade))) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR wrong-time\n");
		return;
	}

	/* Initiating player accepted a quote
	 */
	quote =
	    quotelist_find_domestic(game->quotes, partner_num, quote_num);
	if (quote == NULL) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR quote not found\n");
		return;
	}
	/* Make sure that both parties can satisfy the trade
	 */
	if (!cost_can_afford(quote->var.d.receive, player->assets)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR quote cannot afford\n");
		return;
	}
	partner = player_by_num(game, partner_num);
	if (!cost_can_afford(quote->var.d.supply, partner->assets)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR quote partner cannot afford\n");
		return;
	}

	/* Finally - we do the trade!
	 */
	cost_refund(quote->var.d.receive, partner->assets);
	cost_buy(quote->var.d.supply, partner->assets);
	cost_refund(quote->var.d.supply, player->assets);
	cost_buy(quote->var.d.receive, player->assets);

	player_broadcast(player, PB_RESPOND, FIRST_VERSION, LATEST_VERSION,
			 "domestic-trade accept player %d quote %d supply %R receive %R\n",
			 partner_num, quote_num, supply, receive);

	/* Remove the quote just processed.
	 * The client should remove the quote too.
	 */
	quotelist_delete(game->quotes, quote);
	/* Remove all other quotes from the partner that are no
	 * longer valid
	 */
	quote = quotelist_find_domestic(game->quotes, partner_num, -1);
	while (quote != NULL && quote->var.d.player_num == partner_num) {
		QuoteInfo *tmp = quote;

		quote = quotelist_next(quote);
		if (!cost_can_afford(tmp->var.d.supply, partner->assets)) {
			player_broadcast(partner, PB_ALL, FIRST_VERSION,
					 LATEST_VERSION,
					 "domestic-quote delete %d\n",
					 tmp->var.d.quote_num);
			quotelist_delete(game->quotes, tmp);
		}
	}
}

static void process_call_domestic(Player * player, gint * supply,
				  gint * receive)
{
	Game *game = player->game;
	GList *list;
	gint i;

	for (i = 0; i < NO_RESOURCE; i++) {
		game->quote_supply[i] = supply[i];
		game->quote_receive[i] = receive[i];
	}

	player_broadcast(player, PB_RESPOND, FIRST_VERSION, LATEST_VERSION,
			 "domestic-trade call supply %R receive %R\n",
			 supply, receive);
	/* make sure all the others are back in quote mode.  They may have
	 * gone to monitor mode (after rejecting), but they should be able
	 * to reply to the new call */
	for (list = player_first_real(game); list != NULL;
	     list = player_next_real(list)) {
		Player *scan = list->data;
		if (!player_is_spectator(game, scan->num)
		    && scan != player) {
			sm_goto(scan->sm, (StateFunc) mode_domestic_quote);
		}
	}
}

static void call_domestic(Player * player, gint * supply, gint * receive)
{
	Game *game = player->game;
	Player *partner;
	gint num_supply, num_receive;
	gint idx;
	QuoteInfo *quote;

	/* Check that the player actually has the specified resources
	 */
	num_supply = num_receive = 0;
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		if (supply[idx]) {
			if (player->assets[idx] == 0) {
				player_send(player, FIRST_VERSION,
					    LATEST_VERSION,
					    "ERR not enough resources for this quote\n");
				return;
			}
			num_supply++;
		}
		if (receive[idx] > 0)
			++num_receive;
	}
	if (num_supply == 0 && num_receive == 0) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR empty quote\n");
		return;
	}
	quote = quotelist_first(game->quotes);
	while (quote != NULL) {
		QuoteInfo *curr = quote;

		quote = quotelist_next(quote);
		if (!curr->is_domestic)
			continue;

		/* Is the current quote valid?
		 */
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			if (!receive[idx] && curr->var.d.supply[idx] != 0)
				break;
			if (!supply[idx] && curr->var.d.receive[idx] != 0)
				break;
		}
		if (idx < NO_RESOURCE) {
			partner =
			    player_by_num(game, curr->var.d.player_num);
			player_broadcast(partner, PB_ALL, FIRST_VERSION,
					 LATEST_VERSION,
					 "domestic-quote delete %d\n",
					 curr->var.d.quote_num);
			quotelist_delete(game->quotes, curr);
		}
	}
	process_call_domestic(player, supply, receive);
}

gboolean mode_domestic_initiate(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gint partner_num;
	gint quote_num;
	gint ratio;
	Resource supply_type;
	Resource receive_type;
	gint supply[NO_RESOURCE];
	gint receive[NO_RESOURCE];

	sm_state_name(sm, "mode_domestic_initiate");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "maritime-trade %d supply %r receive %r",
		    &ratio, &supply_type, &receive_type)) {
		trade_perform_maritime(player, ratio, supply_type,
				       receive_type);
		return TRUE;
	}

	if (sm_recv(sm, "domestic-trade finish")) {
		trade_finish_domestic(player);
		return TRUE;
	}

	if (sm_recv
	    (sm,
	     "domestic-trade accept player %d quote %d supply %R receive %R",
	     &partner_num, &quote_num, supply, receive)) {
		trade_accept_domestic(player, partner_num, quote_num,
				      supply, receive);
		return TRUE;
	}

	if (sm_recv(sm, "domestic-trade call supply %R receive %R",
		    supply, receive)) {
		if (!game->params->domestic_trade)
			return FALSE;
		call_domestic(player, supply, receive);
		return TRUE;
	}

	return FALSE;
}

void trade_begin_domestic(Player * player, gint * supply, gint * receive)
{
	Game *game = player->game;
	GList *list;

	sm_push(player->sm, (StateFunc) mode_domestic_initiate);
	quotelist_new(&game->quotes);

	/* push all others to quote mode.  process_call_domestic pops and
	 * repushes them all, so this is needed to keep the state stack
	 * from corrupting. */
	for (list = player_first_real(game); list != NULL;
	     list = player_next_real(list)) {
		Player *scan = list->data;
		if (!player_is_spectator(game, scan->num)
		    && scan != player)
			sm_push(scan->sm, (StateFunc) mode_domestic_quote);
	}

	process_call_domestic(player, supply, receive);
}
