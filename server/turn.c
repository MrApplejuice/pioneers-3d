/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2006 Roland Clobus <rclobus@bigfoot.com>
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
#include <stdlib.h>
#include "buildrec.h"
#include "cost.h"
#include "server.h"
#include "admin.h"
#include "random.h"

static void build_add(Player * player, BuildType type, gint x, gint y,
		      gint pos)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Points *special_points;

	if (!game->rolled_dice) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR roll-dice\n");
		return;
	}

	/* Add settlement/city/road
	 */
	if (type == BUILD_ROAD) {
		/* Building a road, make sure it is next to a
		 * settlement/city/road
		 */
		if (!map_road_vacant(map, x, y, pos)
		    || !map_road_connect_ok(map, player->num, x, y, pos)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return;
		}
		/* Make sure the player can afford the road
		 */
		if (!cost_can_afford(cost_road(), player->assets)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-expensive\n");
			return;
		}
		/* Make sure that there are some roads left to use!
		 */
		if (player->num_roads ==
		    game->params->num_build_type[BUILD_ROAD]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many road\n");
			return;
		}
		edge_add(player, BUILD_ROAD, x, y, pos, TRUE);
		return;
	}

	if (type == BUILD_BRIDGE) {
		/* Building a bridge, make sure it is next to a
		 * settlement/city/road
		 */
		if (!map_road_vacant(map, x, y, pos)
		    || !map_bridge_connect_ok(map, player->num, x, y, pos)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return;
		}
		/* Make sure the player can afford the bridge
		 */
		if (!cost_can_afford(cost_bridge(), player->assets)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-expensive\n");
			return;
		}
		/* Make sure that there are some roads left to use!
		 */
		if (player->num_bridges ==
		    game->params->num_build_type[BUILD_BRIDGE]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many bridge\n");
			return;
		}
		edge_add(player, BUILD_BRIDGE, x, y, pos, TRUE);
		return;
	}

	if (type == BUILD_SHIP) {
		/* Building a ship, make sure it is next to a
		 * settlement/city/ship
		 */
		if (!map_ship_vacant(map, x, y, pos)
		    || !map_ship_connect_ok(map, player->num, x, y, pos)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return;
		}
		/* Make sure the player can afford the ship
		 */
		if (!cost_can_afford(cost_ship(), player->assets)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-expensive\n");
			return;
		}
		/* Make sure that there are some roads left to use!
		 */
		if (player->num_ships ==
		    game->params->num_build_type[BUILD_SHIP]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many ship\n");
			return;
		}
		edge_add(player, BUILD_SHIP, x, y, pos, TRUE);
		return;
	}

	if (type == BUILD_CITY_WALL) {
		if (!can_city_wall_be_built(map_node(map, x, y, pos),
					    player->num)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return;
		}
		/* Make sure that there are some city walls left to use!
		 */
		if (player->num_city_walls ==
		    game->params->num_build_type[BUILD_CITY_WALL]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many city wall\n");
			return;
		}
		if (!cost_can_afford(cost_city_wall(), player->assets)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-expensive\n");
			return;
		}
		node_add(player, type, x, y, pos, TRUE, NULL);
		return;
	}

	/* Build the settlement/city
	 */
	if (!map_building_vacant(map, type, x, y, pos)
	    || !map_building_spacing_ok(map, player->num, type, x, y, pos)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-pos\n");
		return;
	}

	if (type == BUILD_CITY
	    && can_settlement_be_upgraded(map_node(map, x, y, pos),
					  player->num)) {
		/* Make sure that there are some cities left to use!
		 */
		if (player->num_cities ==
		    game->params->num_build_type[BUILD_CITY]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many city\n");
			return;
		}
		if (!cost_can_afford(cost_upgrade_settlement(),
				     player->assets)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-expensive\n");
			return;
		}
	} else {
		/* New building: make sure it connects to a road
		 */
		if (!map_building_connect_ok(map, player->num, x, y, pos)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return;
		}
		/* Make sure that there are some settlements left to use!
		 * Also when building a city, there must be an intermediate
		 * settlement.
		 */
		if (player->num_settlements ==
		    game->params->num_build_type[BUILD_SETTLEMENT]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many settlement\n");
			return;
		}
		/* Make sure the player can afford the building
		 */
		if (type == BUILD_SETTLEMENT) {
			if (!cost_can_afford
			    (cost_settlement(), player->assets)) {
				player_send(player, FIRST_VERSION,
					    LATEST_VERSION,
					    "ERR too-expensive\n");
				return;
			}
		} else {
			/* Make sure that there are some cities left to use!
			 */
			if (player->num_cities ==
			    game->params->num_build_type[BUILD_CITY]) {
				player_send(player, FIRST_VERSION,
					    LATEST_VERSION,
					    "ERR too-many city\n");
				return;
			}
			if (!cost_can_afford(cost_city(), player->assets)) {
				player_send(player, FIRST_VERSION,
					    LATEST_VERSION,
					    "ERR too-expensive\n");
				return;
			}
		}
	}

	special_points = NULL;
	if (game->params->island_discovery_bonus != NULL) {
		if (!map_is_island_discovered
		    (map, map_node(map, x, y, pos), player->num)) {
			gboolean first_island;
			gint points;

			first_island = (player->islands_discovered == 0);
			/* Use the last entry in island_discovery_bonus,
			 * or the current island
			 */
			points =
			    g_array_index(game->params->
					  island_discovery_bonus, gint,
					  MIN(game->params->
					      island_discovery_bonus->len -
					      1,
					      player->islands_discovered));

			if (points != 0)
				special_points =
				    points_new
				    (player->special_points_next_id++,
				     first_island ?
				     N_("Island Discovery Bonus")
				     : N_("Additional Island Bonus"),
				     points);
			player->islands_discovered++;
		}
	}
	node_add(player, type, x, y, pos, TRUE, special_points);

}

static void build_remove(Player * player)
{
	/* Remove the settlement/road we just built
	 */
	if (!perform_undo(player))
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-undo\n");
}

static void build_move(Player * player, gint sx, gint sy, gint spos,
		       gint dx, gint dy, gint dpos)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Edge *from = map_edge(map, sx, sy, spos),
	    *to = map_edge(map, dx, dy, dpos);
	BuildRec *rec;

	/* Allow only one move per turn */
	if (map->has_moved_ship) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR already-moved\n");
		return;
	}

	/* Check if the ship is allowed to move away */
	if (from->owner != player->num || from->type != BUILD_SHIP
	    || to->owner >= 0
	    || !can_ship_be_moved(map_edge(map, sx, sy, spos),
				  player->num)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-pos\n");
		return;
	}

	if (map->pirate_hex != NULL) {
		guint idx;
		/* check that the pirate is not on the from hexes */
		for (idx = 0; idx < G_N_ELEMENTS(from->hexes); ++idx) {
			if (map->pirate_hex == from->hexes[idx]) {
				player_send(player, FIRST_VERSION,
					    LATEST_VERSION,
					    "ERR has-pirate\n");
				return;
			}
		}
		/* checking of destination for pirate is done in
		 * can_ship_be_built */
	}

	/* Move it away */
	from->owner = -1;
	from->type = BUILD_NONE;

	/* Check if it is allowed to move to the other place */
	if ((sx == dx && sy == dy && spos == dpos)
	    || !can_ship_be_built(to, player->num)) {
		from->owner = player->num;
		from->type = BUILD_SHIP;
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-pos\n");
		return;
	}

	/* everything is fine, tell everybode the ship has moved */
	player_broadcast(player, PB_RESPOND, FIRST_VERSION, LATEST_VERSION,
			 "move %d %d %d %d %d %d\n", sx, sy, spos, dx, dy,
			 dpos);

	/* put the move in the undo information */
	rec = buildrec_new(BUILD_MOVE_SHIP, dx, dy, dpos);
	rec->cost = NULL;
	rec->prev_x = sx;
	rec->prev_y = sy;
	rec->prev_pos = spos;
	rec->longest_road =
	    game->longest_road ? game->longest_road->num : -1;
	player->build_list = g_list_append(player->build_list, rec);
	map->has_moved_ship = TRUE;

	/* check the longest road while the ship is moving */
	check_longest_road(game);

	/* administrate the arrival of the ship */
	to->owner = player->num;
	to->type = BUILD_SHIP;

	/* check the longest road again */
	check_longest_road(game);
}

typedef struct {
	Game *game;
	int roll;
} GameRoll;

static gboolean distribute_resources(const Hex * hex, gpointer closure)
{
	guint idx;
	GameRoll *data = closure;

	if (hex->roll != data->roll || hex->robber)
		/* return false so the traverse function continues */
		return FALSE;

	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); idx++) {
		const Node *node = hex->nodes[idx];
		Player *player;
		gint num;

		if (node->type == BUILD_NONE)
			continue;
		player = player_by_num(data->game, node->owner);
		if (player != NULL) {
			num = (node->type == BUILD_CITY) ? 2 : 1;
			if (hex->terrain == GOLD_TERRAIN)
				player->gold += num;
			else
				player->assets[hex->terrain] += num;
		} else {
			/* This should be fixed at some point. */
			log_message(MSG_ERROR,
				    _(""
				      "Tried to assign resources to NULL player.\n"));
		}
	}

	/* return false so the traverse function continues */
	return FALSE;
}

gboolean check_victory(Player * player)
{
	Game *game = player->game;
	GList *list;
	gint points;		/* can be negative, due to island bonuses */

	if (player->num != game->curr_player)
		/* Only the player that has the turn can win */
		return FALSE;

	points = player->num_settlements
	    + player->num_cities * 2 + player->develop_points;

	if (game->longest_road == player)
		points += 2;
	if (game->largest_army == player)
		points += 2;

	list = player->special_points;
	while (list) {
		Points *point = list->data;
		points += point->points;
		list = g_list_next(list);
	}

	if (points >= (gint) game->params->victory_points) {
		player_broadcast(player, PB_ALL, FIRST_VERSION,
				 LATEST_VERSION, "won with %d\n", points);
		game->is_game_over = TRUE;
		/* Set all state machines to idle, to make sure nothing
		 * happens. */
		for (list = player_first_real(game); list != NULL;
		     list = player_next_real(list)) {
			Player *scan = list->data;
			sm_pop_all_and_goto(scan->sm,
					    (StateFunc) mode_idle);
		}
		meta_unregister();

		game_is_over(game);
		return TRUE;
	}
	return FALSE;
}

/** Rolls the dice.
 * @param player The player who rolled the dice.
 */
static void roll_dice(Player * player)
{
	Game *game = player->game;
	const Map *map = game->params->map;
	GameRoll data;
	gint roll;

	if (game->rolled_dice) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR already-rolled\n");
		return;
	}

	roll = admin_get_dice_roll();
	if (roll == 0) {
		/* roll the dice until we like it */
		while (TRUE) {
			if (game->params->use_dice_deck) {
				/* dice deck */
				gint i;
				gint card;
				if (game->num_dice_cards <=
				    game->params->num_removed_dice_cards) {
					/* shuffle deck */
					for (i = 0; i < 36; i++) {
						game->dice_cards[i] =
						    game->
						    params->num_dice_decks;
					}
					game->num_dice_cards =
					    game->params->num_dice_decks *
					    36;

					player_broadcast(player, PB_ALL,
							 V15,
							 LATEST_VERSION,
							 "shuffled-dice-deck\n");
				}
				card = random_guint(game->num_dice_cards);

				i = -1;
				while (card >= 0) {
					i++;
					card -= game->dice_cards[i];
				}
				game->dice_cards[i]--;
				game->num_dice_cards--;
				game->die1 = i % 6 + 1;
				game->die2 = i / 6 + 1;
			} else {
				/* two dice */
				game->die1 = random_guint(6) + 1;
				game->die2 = random_guint(6) + 1;
			}
			roll = game->die1 + game->die2;
			/* sevens_rule == 1: reroll first two turns */
			if (game->params->sevens_rule == 1)
				if (roll == 7 && game->curr_turn <= 2)
					continue;
			/* sevens_rule == 2: reroll all sevens */
			if (game->params->sevens_rule == 2)
				if (roll == 7)
					continue;
			/* sevens_rule == 0: don't reroll anything */
			break;
		}
	} else {
		/* The administrator can override the dice */
		game->die1 = roll > 6 ? 6 : 1;
		game->die2 = roll - game->die1;
		player_broadcast(player, PB_SILENT, FIRST_VERSION,
				 LATEST_VERSION, "NOTE %s\n",
				 /* Cheat mode has been activated */
				 N_(""
				    "The dice roll has been determined by the administrator."));
	}
	game->rolled_dice = TRUE;
	g_assert(roll == game->die1 + game->die2);

	/* let people know what we rolled */
	player_broadcast(player, PB_RESPOND, FIRST_VERSION,
			 LATEST_VERSION, "rolled %d %d\n",
			 game->die1, game->die2);

	if (roll == 7) {
		/* Find all players with more than 7 cards -
		 * they must discard half (rounded down)
		 */
		discard_resources(game);
		/* there are no resources to distribute on a 7 */
		return;
	}
	resource_start(game);
	data.game = game;
	data.roll = roll;
	map_traverse_const(map, distribute_resources, &data);
	/* distribute resources and gold (includes resource_end) */
	distribute_first(list_from_player(player));
	return;

}

/* Handle all actions that a player may perform in a turn
 */
gboolean mode_turn(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	BuildType build_type;
	DevelType devel_type;
	gint x, y, pos;
	guint idx;
	gint ratio;
	Resource supply_type, receive_type;
	gint supply[NO_RESOURCE], receive[NO_RESOURCE];
	gint sx, sy, spos, dx, dy, dpos;

	sm_state_name(sm, "mode_turn");
	if (event == SM_INIT) {
		if (!game->params->check_victory_at_end_of_turn)
			check_victory(player);
		return TRUE;
	}
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "roll")) {
		roll_dice(player);
		return TRUE;
	}
	/* try to end a turn */
	if (sm_recv(sm, "done")) {
		if (!game->rolled_dice) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR roll-dice\n");
			return TRUE;
		}
		/* Ok, finish turn */
		player_send(player, FIRST_VERSION, LATEST_VERSION, "OK\n");
		if (!check_victory(player)) {
			/* game isn't over, so pop the state machine back to idle */
			sm_pop(sm);
			turn_next_player(game);
		}
		return TRUE;
	}
	if (sm_recv(sm, "buy-develop")) {
		develop_buy(player);
		return TRUE;
	}
	if (sm_recv(sm, "play-develop %u", &idx, &devel_type)) {
		develop_play(player, idx);
		if (!game->params->check_victory_at_end_of_turn)
			check_victory(player);
		return TRUE;
	}
	if (sm_recv(sm, "maritime-trade %d supply %r receive %r",
		    &ratio, &supply_type, &receive_type)) {
		trade_perform_maritime(player, ratio, supply_type,
				       receive_type);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-trade call supply %R receive %R",
		    supply, receive)) {
		if (!game->params->domestic_trade)
			return FALSE;
		trade_begin_domestic(player, supply, receive);
		return TRUE;
	}
	if (sm_recv(sm, "build %B %d %d %d", &build_type, &x, &y, &pos)) {
		build_add(player, build_type, x, y, pos);
		if (!game->params->check_victory_at_end_of_turn)
			check_victory(player);
		return TRUE;
	}
	if (sm_recv
	    (sm, "move %d %d %d %d %d %d", &sx, &sy, &spos, &dx, &dy,
	     &dpos)) {
		build_move(player, sx, sy, spos, dx, dy, dpos);
		if (!game->params->check_victory_at_end_of_turn)
			check_victory(player);
		return TRUE;
	}
	if (sm_recv(sm, "undo")) {
		build_remove(player);
		return TRUE;
	}
	return FALSE;
}

/* Player should be idle - I will tell them when to do something
 */
gboolean mode_idle(Player * player, G_GNUC_UNUSED gint event)
{
	StateMachine *sm = player->sm;
	sm_state_name(sm, "mode_idle");
	return FALSE;
}

void turn_next_player(Game * game)
{
	Player *player = NULL;
	GList *list = NULL;

	/* the first time this is called there is no curr_player yet */
	if (game->curr_player >= 0) {
		player = player_by_num(game, game->curr_player);
		game->curr_player = -1;
		g_assert(player != NULL);
		list = list_from_player(player);
	}

	do {
		/* next player */
		if (list)
			list = player_next_real(list);
		/* See if it's the first player's turn again */
		if (list == NULL) {
			list = player_first_real(game);
			game->curr_turn++;
		}
		/* sanity check */
		g_assert(list != NULL && list->data != NULL);
		player = list->data;
		/* disconnected players don't take turns */
	} while (player->disconnected);

	/* reset variables */
	game->curr_player = player->num;
	game->rolled_dice = FALSE;
	game->bought_develop = FALSE;
	game->num_playable_cards = deck_count(player->devel);
	player->build_list = buildrec_free(player->build_list);
	game->params->map->has_moved_ship = FALSE;

	/* tell everyone what's happening */
	player_broadcast(player, PB_RESPOND, FIRST_VERSION, LATEST_VERSION,
			 "turn %d\n", game->curr_turn);
	if (game->is_manipulated) {
		/* Intentionally not translated */
		admin_broadcast(game, "The administrator has "
				"manipulated this game.");
	}

	/* put the player in the right state */
	sm_push(player->sm, (StateFunc) mode_turn);
}
