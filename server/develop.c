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
#include "buildrec.h"
#include "cost.h"
#include "server.h"
#include "random.h"

void develop_shuffle(Game * game)
{
	GameParams *params;
	gint idx;
	guint shuffle_idx;
	gint shuffle_counts[NUM_DEVEL_TYPES];

	params = game->params;
	memcpy(shuffle_counts, params->num_develop_type,
	       sizeof(shuffle_counts));
	game->num_develop = 0;
	for (idx = 0; idx < NUM_DEVEL_TYPES; idx++)
		game->num_develop += shuffle_counts[idx];
	if (game->develop_deck != NULL)
		g_free(game->develop_deck);
	game->develop_deck =
	    g_malloc0((gsize) game->num_develop *
		      sizeof(*game->develop_deck));

	for (idx = 0; idx < game->num_develop; idx++) {
		gint card_idx;

		card_idx = random_guint(game->num_develop - idx);
		for (shuffle_idx = 0;
		     shuffle_idx < G_N_ELEMENTS(shuffle_counts);
		     shuffle_idx++) {
			card_idx -= shuffle_counts[shuffle_idx];
			if (card_idx < 0) {
				shuffle_counts[shuffle_idx]--;
				game->develop_deck[idx] = shuffle_idx;
				break;
			}
		}
	}

	/* Check that the deck was shuffled correctly
	 */
	memcpy(shuffle_counts, params->num_develop_type,
	       sizeof(shuffle_counts));
	for (idx = 0; idx < game->num_develop; idx++)
		shuffle_counts[game->develop_deck[idx]]--;
	for (shuffle_idx = 0; shuffle_idx < G_N_ELEMENTS(shuffle_counts);
	     shuffle_idx++)
		if (shuffle_counts[shuffle_idx] != 0) {
			log_message(MSG_ERROR, "Bad shuffle\n");
			break;
		}
	game->develop_next = 0;
}

void develop_buy(Player * player)
{
	Game *game = player->game;
	DevelType card;

	if (!game->rolled_dice) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR roll-dice\n");
		return;
	}
	if (!cost_can_afford(cost_development(), player->assets)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR too-expensive\n");
		return;
	}
	if (game->develop_next >= game->num_develop) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR no-cards\n");
		return;
	}

	/* Clear the build list to prevent undo after buying
	 * development card
	 */
	player->build_list = buildrec_free(player->build_list);
	resource_spend(player, cost_development());
	player_broadcast(player, PB_OTHERS, FIRST_VERSION, LATEST_VERSION,
			 "bought-develop\n");
	game->bought_develop = TRUE;

	card = game->develop_deck[game->develop_next++];
	deck_add_guint(player->devel, card);
	player_send(player, FIRST_VERSION, LATEST_VERSION,
		    "bought-develop %d\n", card);
}

gboolean mode_road_building(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	BuildType type;
	gint x, y, pos;
	GList *rb_build_rec;

	sm_state_name(sm, "mode_road_building");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "done")) {
		/* Make sure we have built the right number of roads
		 */
		gint num_built;

		num_built = buildrec_count_edges(player->build_list);
		if (num_built < 2
		    &&
		    ((player->num_roads <
		      game->params->num_build_type[BUILD_ROAD]
		      && map_can_place_road(map, player->num))
		     || (player->num_ships <
			 game->params->num_build_type[BUILD_SHIP]
			 && map_can_place_ship(map, player->num))
		     || (player->num_bridges <
			 game->params->num_build_type[BUILD_BRIDGE]
			 && map_can_place_bridge(map, player->num)))) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR expected-build\n");
			return TRUE;
		}
		/* We have the right number, now make sure that all
		 * roads are connected to buildings
		 */
		if (!buildrec_is_valid
		    (player->build_list, map, player->num)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR unconnected\n");
			return TRUE;
		}

		/* Player has finished road building
		 */

		/* Remove the roads built from the player's build_list.
		 * If we don't, trading will fail when it shouldn't.
		 */
		if (num_built >= 2) {
			num_built = 2;
		}
		for (; num_built >= 0; num_built--) {
			rb_build_rec = g_list_last(player->build_list);
			player->build_list =
			    g_list_remove_link(player->build_list,
					       rb_build_rec);
			g_list_free_1(rb_build_rec);
		}

		/* Send ack to client, check for victory, and quit.
		 */
		player_send(player, FIRST_VERSION, LATEST_VERSION, "OK\n");
		sm_pop(sm);
		check_victory(player);
		return TRUE;
	}

	if (sm_recv(sm, "build %B %d %d %d", &type, &x, &y, &pos)) {
		if (buildrec_count_type(player->build_list, type) == 2) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many\n");
			return TRUE;
		}

		/* Building a road / ship / bridge, make sure it is
		 * correctly placed
		 */
		if (!map_road_vacant(map, x, y, pos)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return TRUE;
		}
		switch (type) {
		case BUILD_ROAD:
			if (map_road_connect_ok
			    (map, player->num, x, y, pos))
				break;
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return TRUE;
		case BUILD_SHIP:
			if (map_ship_connect_ok
			    (map, player->num, x, y, pos))
				break;
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return TRUE;
		case BUILD_BRIDGE:
			if (map_bridge_connect_ok
			    (map, player->num, x, y, pos))
				break;
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return TRUE;
		default:
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR expected-road\n");
			return TRUE;
		}

		edge_add(player, type, x, y, pos, FALSE);
		return TRUE;
	}

	if (sm_recv(sm, "undo")) {
		if (!perform_undo(player))
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
		return TRUE;
	}

	return FALSE;
}

gboolean mode_plenty_resources(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	int idx;
	int num;
	int num_in_bank;
	int plenty[NO_RESOURCE];

	sm_state_name(sm, "mode_plenty_resources");
	if (event != SM_RECV)
		return FALSE;

	if (!sm_recv(sm, "plenty %R", plenty))
		return FALSE;

	num = 0;
	for (idx = 0; idx < NO_RESOURCE; idx++)
		num += plenty[idx];

	if (!resource_available(player, plenty, &num_in_bank)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR plenty-no-resources\n");
		return TRUE;
	}
	if ((num_in_bank < 2 && num != num_in_bank)
	    || (num_in_bank >= 2 && num != 2)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR wrong-plenty\n");
		return TRUE;
	}

	/* Give the resources to the player
	 */
	resource_start(game);
	cost_refund(plenty, player->assets);
	resource_end(game, "plenty", 1);
	player_send(player, FIRST_VERSION, LATEST_VERSION, "OK\n");
	sm_pop(sm);
	return TRUE;
}

/* monopoly <resource-type>
 */
gboolean mode_monopoly(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	GList *list;
	Resource type;

	sm_state_name(sm, "mode_monopoly");
	if (event != SM_RECV)
		return FALSE;

	if (!sm_recv(sm, "monopoly %r", &type))
		return FALSE;

	/* Now inform the various parties of the monopoly.
	 */
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *scan = list->data;

		if (scan == player)
			continue;

		player_broadcast(player, PB_ALL, FIRST_VERSION,
				 LATEST_VERSION,
				 "monopoly %d %r from %d\n",
				 scan->assets[type], type, scan->num);

		/* Alter the assets of the respective players
		 */
		player->assets[type] += scan->assets[type];
		scan->assets[type] = 0;
	}
	player_send(player, FIRST_VERSION, LATEST_VERSION, "OK\n");

	sm_pop(sm);
	return TRUE;
}

static void check_largest_army(Game * game)
{
	GList *list;
	Player *new_largest;

	new_largest = NULL;
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *player = list->data;

		/* Only 3 or more soldiers can earn largest army
		 */
		if (player->num_soldiers < 3)
			continue;
		if (new_largest == NULL)
			new_largest = player;
		else if (player->num_soldiers > new_largest->num_soldiers)
			/* Only get the largest if exceed the current
			 * largest
			 */
			new_largest = player;
	}
	if (new_largest == NULL)
		return;

	/* Now change the largest army owner if necessary
	 */
	if (game->largest_army == NULL) {
		game->largest_army = new_largest;
		player_broadcast(game->largest_army, PB_ALL, FIRST_VERSION,
				 LATEST_VERSION, "largest-army\n");
		return;
	}
	/* Did largest army owner change?
	 */
	if (new_largest != game->largest_army
	    && new_largest->num_soldiers >
	    game->largest_army->num_soldiers) {
		game->largest_army = new_largest;
		player_broadcast(game->largest_army, PB_ALL, FIRST_VERSION,
				 LATEST_VERSION, "largest-army\n");
	}
}

void develop_play(Player * player, guint idx)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	DevelType card;

	if (idx >= deck_count(player->devel)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR no-card\n");
		return;
	}

	card = deck_get_guint(player->devel, idx);
	if (!deck_card_play(player->devel, game->num_playable_cards, idx)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR wrong-time\n");
		return;
	}

	if (!is_victory_card(card))
		game->num_playable_cards = 0;

	/* Cannot undo after playing development card
	 */
	player->build_list = buildrec_free(player->build_list);

	player_broadcast(player, PB_RESPOND, FIRST_VERSION, LATEST_VERSION,
			 "play-develop %d %D\n", idx, card);

	switch (card) {
	case DEVEL_ROAD_BUILDING:
		/* Place 2 new roads as if you had just built them.
		 */
		sm_push(sm, (StateFunc) mode_road_building);
		break;
	case DEVEL_MONOPOLY:
		/* When you play this card, announce one type of
		 * resource.  All other players must give you all
		 * their resource cards of that type.
		 */
		sm_push(sm, (StateFunc) mode_monopoly);
		break;
	case DEVEL_YEAR_OF_PLENTY:
		/* Take any 2 resource cards from the bank and add
		 * them to your hand. They can be two different
		 * resources or two of the same resource. They may
		 * immediately be used to build.
		 */
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "plenty %R\n", game->bank_deck);
		sm_push(sm, (StateFunc) mode_plenty_resources);
		break;
	case DEVEL_CHAPEL:
	case DEVEL_UNIVERSITY:
	case DEVEL_GOVERNORS_HOUSE:
	case DEVEL_LIBRARY:
	case DEVEL_MARKET:

		switch (card) {
		case DEVEL_CHAPEL:
			++player->chapel_played;
			break;
		case DEVEL_UNIVERSITY:
			++player->univ_played;
			break;
		case DEVEL_GOVERNORS_HOUSE:
			++player->gov_played;
			break;
		case DEVEL_LIBRARY:
			++player->libr_played;
			break;
		case DEVEL_MARKET:
			++player->market_played;
			break;
		default:
			;
		}

		/* One victory point
		 */
		player->develop_points++;
		break;

	case DEVEL_SOLDIER:
		/* Move the robber. Steal one resource card from the
		 * owner of an adjacent settlement or city.
		 */
		player->num_soldiers++;
		check_largest_army(game);
		robber_place(player);
		break;
	}
}
