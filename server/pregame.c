/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003-2005 Bas Wijnen <shevek@fmf.nl>
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
#include <string.h>
#include "buildrec.h"
#include "server.h"
#include "version.h"

static void build_add(Player * player, BuildType type, gint x, gint y,
		      gint pos)
{
	Game *game = player->game;
	Map *map = game->params->map;
	gint num;
	gint num_allowed;

	num_allowed = game->double_setup ? 2 : 1;

	/* Add settlement/road
	 */
	num = buildrec_count_type(player->build_list, type);
	if (num == num_allowed) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR too-many\n");
		return;
	}

	if (type == BUILD_ROAD) {
		/* Make sure that there are some roads left to use */
		if (player->num_roads ==
		    game->params->num_build_type[BUILD_ROAD]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many road\n");
			return;
		}

		/* Building a road, make sure it is next to a
		 * settlement/road
		 */
		if (!buildrec_can_setup_road
		    (player->build_list, map_edge(map, x, y, pos),
		     game->double_setup)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return;
		}
		edge_add(player, BUILD_ROAD, x, y, pos, FALSE);
		return;
	}

	if (type == BUILD_BRIDGE) {
		/* Make sure that there are some bridges left to use */
		if (player->num_bridges ==
		    game->params->num_build_type[BUILD_BRIDGE]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many bridge\n");
			return;
		}

		/* Building a bridge, make sure it is next to a
		 * settlement/road
		 */
		if (!buildrec_can_setup_bridge
		    (player->build_list, map_edge(map, x, y, pos),
		     game->double_setup)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return;
		}
		edge_add(player, BUILD_BRIDGE, x, y, pos, FALSE);
		return;
	}

	if (type == BUILD_SHIP) {
		/* Make sure that there are some ships left to use */
		if (player->num_ships ==
		    game->params->num_build_type[BUILD_SHIP]) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR too-many ship\n");
			return;
		}

		/* Building a ship, make sure it is next to a
		 * settlement/ship
		 */
		if (!buildrec_can_setup_ship
		    (player->build_list, map_edge(map, x, y, pos),
		     game->double_setup)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR bad-pos\n");
			return;
		}
		edge_add(player, BUILD_SHIP, x, y, pos, FALSE);
		return;
	}

	if (type != BUILD_SETTLEMENT) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR expected-road-or-settlement\n");
		return;
	}
	/* Build the settlement
	 */
	if (!buildrec_can_setup_settlement
	    (player->build_list, map_node(map, x, y, pos),
	     game->double_setup)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-pos\n");
		return;
	}
	node_add(player, BUILD_SETTLEMENT, x, y, pos, FALSE, NULL);
}

static void build_remove(Player * player)
{
	/* Remove the settlement/road we just built
	 */
	if (!perform_undo(player))
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-pos\n");
}

static void start_setup_player(Player * player)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;

	player->build_list = buildrec_free(player->build_list);

	if (game->double_setup)
		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION, "setup-double\n");
	else
		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION, "setup %d\n",
				 game->reverse_setup);

	sm_goto(sm, (StateFunc) mode_setup);
}

static void allocate_resources(Player * player, BuildRec * rec)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Node *node;
	guint idx;

	node = map_node(map, rec->x, rec->y, rec->pos);

	resource_start(game);
	for (idx = 0; idx < G_N_ELEMENTS(node->hexes); idx++) {
		Hex *hex = node->hexes[idx];
		if (hex && hex->roll > 0) {
			if (hex->terrain == GOLD_TERRAIN)
				++player->gold;
			else
				++player->assets[hex->terrain];
		}
	}
	/* give out the gold */
	distribute_first(list_from_player(player));
	return;
}

/* Player tried to finish setup mode */
static void try_setup_done(Player * player)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	gint num_allowed;

	num_allowed = game->double_setup ? 2 : 1;

	/* Make sure we have built the right number of
	 * settlements/roads
	 */
	if (buildrec_count_edges(player->build_list) != num_allowed
	    || buildrec_count_type(player->build_list,
				   BUILD_SETTLEMENT) != num_allowed) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR expected-build-or-remove\n");
		return;
	}
	/* We have the right number, now make sure that all roads are
	 * connected to buildings and vice-versa
	 */
	if (!buildrec_is_valid(player->build_list, map, player->num)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR unconnected\n");
		return;
	}
	/* Player has finished setup phase - give resources for second
	 * settlement
	 */
	player_send(player, FIRST_VERSION, LATEST_VERSION, "OK\n");

	if (game->double_setup)
		allocate_resources(player,
				   buildrec_get(player->build_list,
						BUILD_SETTLEMENT, 1));
	else if (game->reverse_setup)
		allocate_resources(player,
				   buildrec_get(player->build_list,
						BUILD_SETTLEMENT, 0));
	else {
		sm_goto(sm, (StateFunc) mode_idle);
		next_setup_player(game);
	}
}

/* find next player to do setup. */
void next_setup_player(Game * game)
{
	if (game->reverse_setup) {
		/* Going back for second setup phase
		 */
		GList *prev = NULL, *list;
		for (list = player_first_real(game); list != NULL;
		     list = player_next_real(list)) {
			if (list == game->setup_player)
				break;
			prev = list;
		}
		game->setup_player = prev;
		game->double_setup = FALSE;
		if (game->setup_player != NULL) {
			start_setup_player(game->setup_player->data);
		} else {
			/* Start the game!!!
			 */
			turn_next_player(game);
		}
	} else {
		/* First setup phase
		 */
		game->setup_player = player_next_real(game->setup_player);
		/* Last player gets double setup
		 */
		game->double_setup
		    = player_next_real(game->setup_player) == NULL;
		/* Prepare to go backwards next time
		 */
		game->reverse_setup = game->double_setup;
		start_setup_player(game->setup_player->data);
	}
}

/* Player must place exactly one settlement and one road which connect
 * to each other.  If last player, then perform a double setup.
 */
gboolean mode_setup(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	BuildType type;
	gint x, y, pos;

	sm_state_name(sm, "mode_setup");
	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "done")) {
		try_setup_done(player);
		return TRUE;
	}
	if (sm_recv(sm, "build %B %d %d %d", &type, &x, &y, &pos)) {
		build_add(player, type, x, y, pos);
		return TRUE;
	}
	if (sm_recv(sm, "undo")) {
		build_remove(player);
		return TRUE;
	}

	return FALSE;
}

static void try_start_game(Game * game)
{
	GList *list;
	guint num;
	int numturn;

	num = 0;
	numturn = 0;
	for (list = player_first_real(game);
	     list != NULL; list = player_next_real(list)) {
		Player *player = list->data;

		if (sm_current(player->sm) == (StateFunc) mode_idle)
			num++;

		if (sm_current(player->sm) == (StateFunc) mode_turn ||
		    sm_current(player->sm)
		    == (StateFunc) mode_discard_resources ||
		    sm_current(player->sm)
		    == (StateFunc) mode_place_robber ||
		    sm_current(player->sm)
		    == (StateFunc) mode_road_building ||
		    sm_current(player->sm)
		    == (StateFunc) mode_monopoly || sm_current(player->sm)
		    == (StateFunc) mode_plenty_resources) {
			/* looks like this player got disconnected and
			   now it's his turn. */
			num++;
			numturn++;
		}
	}
	if (num != game->params->num_players)
		return;

	if (params_game_is_unstartable(game->params)) {
		/* this game cannot be started, don't enter the setup phase */
		return;
	}

	if (numturn > 0) {
		/* someone got disconnected. Now everyone's back. Let's
		   continue the game... */

		return;
	}

	/* All players have connected, and are ready to begin
	 */
	if (game->tournament_timer != 0) {
		g_source_remove(game->tournament_timer);
		game->tournament_timer = 0;
	}
	meta_start_game();
	game->setup_player = player_first_real(game);
	while (((Player *) game->setup_player->data)->num < 0)
		game->setup_player = game->setup_player->next;
	game->double_setup = game->reverse_setup = FALSE;

	start_setup_player(game->setup_player->data);
}

/* Send the player list to the client
 */
static void send_player_list(Player * player)
{
	Game *game = player->game;
	GList *list;

	player_send_uncached(player, FIRST_VERSION, LATEST_VERSION,
			     "players follow\n");
	for (list = game->player_list; list != NULL;
	     list = g_list_next(list)) {
		Player *scan = list->data;
		if (player == scan || scan->num < 0)
			continue;
		player_send_uncached(player, FIRST_VERSION, LATEST_VERSION,
				     "player %d is %s\n", scan->num,
				     scan->name);
		player_send_uncached(player, V0_11, LATEST_VERSION,
				     "player %d style %s\n", scan->num,
				     scan->style);
		if (scan->disconnected)
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION,
					     "player %d has quit\n",
					     scan->num);
	}
	player_send_uncached(player, FIRST_VERSION, LATEST_VERSION, ".\n");
}

/* Send the game parameters to the player (uncached)
 */
static void send_game_line(gpointer player, const gchar * str)
{
	player_send_uncached((Player *) player, FIRST_VERSION,
			     LATEST_VERSION, "%s\n", str);
}

gboolean send_gameinfo_uncached(const Hex * hex, void *data)
{
	guint i;
	Player *player = data;

	for (i = 0; i < G_N_ELEMENTS(hex->nodes); i++) {
		if (!hex->nodes[i] || hex->nodes[i]->x != hex->x
		    || hex->nodes[i]->y != hex->y)
			continue;
		if (hex->nodes[i]->owner >= 0) {
			switch (hex->nodes[i]->type) {
			case BUILD_SETTLEMENT:
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "S%d,%d,%d,%d\n",
						     hex->x, hex->y, i,
						     hex->nodes[i]->owner);
				break;
			case BUILD_CITY:
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "C%d,%d,%d,%d\n",
						     hex->x, hex->y, i,
						     hex->nodes[i]->owner);
				break;
			default:
				;
			}
			if (hex->nodes[i]->city_wall) {
				/* Older clients see an extension message */
				player_send_uncached(player, FIRST_VERSION,
						     V0_10,
						     "extension city wall\n");
				player_send_uncached(player, V0_11,
						     LATEST_VERSION,
						     "W%d,%d,%d,%d\n",
						     hex->x, hex->y, i,
						     hex->nodes[i]->owner);
			}
		}
	}

	for (i = 0; i < G_N_ELEMENTS(hex->edges); i++) {
		if (!hex->edges[i] || hex->edges[i]->x != hex->x
		    || hex->edges[i]->y != hex->y)
			continue;
		if (hex->edges[i]->owner >= 0) {
			switch (hex->edges[i]->type) {
			case BUILD_ROAD:
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "R%d,%d,%d,%d\n",
						     hex->x, hex->y, i,
						     hex->edges[i]->owner);
				break;
			case BUILD_SHIP:
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "SH%d,%d,%d,%d\n",
						     hex->x, hex->y, i,
						     hex->edges[i]->owner);
				break;
			case BUILD_BRIDGE:
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "B%d,%d,%d,%d\n",
						     hex->x, hex->y, i,
						     hex->edges[i]->owner);
				break;
			default:
				;
			}
		}
	}

	if (hex->robber) {
		player_send_uncached(player, FIRST_VERSION, LATEST_VERSION,
				     "RO%d,%d\n", hex->x, hex->y);
	}

	if (hex == hex->map->pirate_hex) {
		player_send_uncached(player, FIRST_VERSION, LATEST_VERSION,
				     "P%d,%d\n", hex->x, hex->y);
	}

	return FALSE;
}

/* Player setup phase
 */
gboolean mode_pre_game(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	StateFunc state;
	const gchar *prevstate;
	guint i;
	GList *next;
	gint longestroadpnum = -1;
	gint largestarmypnum = -1;
	static gboolean recover_from_plenty = FALSE;
	guint stack_offset;
	gchar *player_style;

	if (game->longest_road) {
		longestroadpnum = game->longest_road->num;
	}
	if (game->largest_army) {
		largestarmypnum = game->largest_army->num;
	}

	sm_state_name(sm, "mode_pre_game");
	switch (event) {
	case SM_ENTER:
		player_send_uncached(player, FIRST_VERSION, LATEST_VERSION,
				     "player %d of %d, welcome to pioneers server %s\n",
				     player->num,
				     game->params->num_players,
				     FULL_VERSION);
		/* Tell the player that he exists.  This is not done in
		 * player_set_name, because at that point the client doesn't
		 * know how many players are in the game, and therefore if
		 * he is a player or a spectator. */
		/* Tell the other players about this player */
		player_broadcast(player, PB_OTHERS, FIRST_VERSION,
				 LATEST_VERSION, "is %s\n", player->name);
		/* Tell this player his own name */
		player_send_uncached(player, FIRST_VERSION, LATEST_VERSION,
				     "player %d is %s\n", player->num,
				     player->name);
		break;

	case SM_RECV:
		if (sm_recv(sm, "style %S", &player_style)) {
			if (player->style)
				g_free(player->style);
			player->style = player_style;
			player_broadcast(player, PB_OTHERS, V0_11,
					 LATEST_VERSION, "style %s\n",
					 player_style);
			player_send_uncached(player, V0_11, LATEST_VERSION,
					     "player %d style %s\n",
					     player->num, player_style);
			return TRUE;
		}
		if (sm_recv(sm, "players")) {
			send_player_list(player);
			return TRUE;
		}
		if (sm_recv(sm, "game")) {
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION, "game\n");
			params_write_lines(game->params, player->version,
					   FALSE, send_game_line, player);
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION, "end\n");
			return TRUE;
		}
		if (sm_recv(sm, "gameinfo")) {
			GList *list;

			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION, "gameinfo\n");
			map_traverse_const(map, send_gameinfo_uncached,
					   player);
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION, ".\n");

			/* Notify old clients about new features */
			if (game->params->num_build_type[BUILD_CITY_WALL] >
			    0) {
				player_send_uncached(player, FIRST_VERSION,
						     V0_10,
						     "extension city wall\n");
			}

			/* now, send state info */
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION,
					     "turn num %d\n",
					     game->curr_turn);
			if (game->curr_player >= 0) {
				Player *playerturn
				    =
				    player_by_num(game, game->curr_player);
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "player turn: %d\n",
						     playerturn->num);
			}
			if (game->rolled_dice) {
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "dice rolled: %d %d\n",
						     game->die1,
						     game->die2);
			} else if (game->die1 + game->die2 > 1) {
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "dice value: %d %d\n",
						     game->die1,
						     game->die2);
			}
			if (game->bought_develop) {
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "bought develop\n");
			}
			if (player->disconnected) {
				player_send_uncached(player, FIRST_VERSION,
						     V0_12,
						     "player disconnected\n");
			}
			stack_offset = 1;
			state = sm_stack_inspect(sm, stack_offset);
			while ((state == (StateFunc) mode_choose_gold) ||
			       (state == (StateFunc)
				mode_wait_for_gold_choosing_players)) {
				++stack_offset;
				state = sm_stack_inspect(sm, stack_offset);
			}

			if (state == (StateFunc) mode_idle)
				prevstate = "IDLE";
			else if (state == (StateFunc) mode_turn)
				prevstate = "TURN";
			else if (state ==
				 (StateFunc) mode_discard_resources)
				prevstate = "DISCARD";
			else if (state == (StateFunc)
				 mode_wait_for_other_discarding_players)
				prevstate = "DISCARD";
			else if (state == (StateFunc) mode_place_robber)
				prevstate = "YOUAREROBBER";
			else if (state == (StateFunc) mode_road_building)
				prevstate = "ROADBUILDING";
			else if (state == (StateFunc) mode_monopoly)
				prevstate = "MONOPOLY";
			else if (state ==
				 (StateFunc) mode_plenty_resources) {
				recover_from_plenty = TRUE;
				prevstate = "PLENTY";
			} else if (state == (StateFunc) mode_setup) {
				if (game->double_setup)
					prevstate = "SETUPDOUBLE";
				else if (game->reverse_setup)
					prevstate = "RSETUP";
				else
					prevstate = "SETUP";
				/* If player is selecting gold, the state
				 * should be IDLE instead */
				if (stack_offset != 1)
					prevstate = "IDLE";
			} else
				prevstate = "PREGAME";

			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION, "state %s\n",
					     prevstate);

			/* Send the bank, so the client can count remaining
			 * resources
			 */
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION, "bank %R\n",
					     game->bank_deck);

			/* Send the number of development cards played, so the
			 * client knows how many are left.
			 */
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION,
					     "development-bought %d\n",
					     game->develop_next);

			/* Send player info about what he has:
			   resources, dev cards, roads, # roads,
			   # bridges, # ships, # settles, # cities,
			   # soldiers, road len, dev points,
			   who has longest road/army,
			   spectators will receive an empty list */
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION,
					     "playerinfo: resources: %R\n",
					     player->assets);
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION,
					     "playerinfo: numdevcards: %d\n",
					     deck_count(player->devel));
			for (i = 0; i < deck_count(player->devel); i++) {
				player_send_uncached(player, FIRST_VERSION,
						     V14,
						     "playerinfo: devcard: %d %d\n",
						     (gint)
						     deck_get_guint
						     (player->devel, i),
						     i <
						     game->num_playable_cards
						     ? -1 : game->
						     curr_turn);
				player_send_uncached(player, V15,
						     LATEST_VERSION,
						     "playerinfo: devcard: %d %d\n",
						     (gint)
						     deck_get_guint
						     (player->devel, i),
						     i <
						     game->num_playable_cards
						     ? 0 : 1);
			}
			player_send_uncached(player, V14,
					     LATEST_VERSION,
					     "playerinfo: %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
					     player->num_roads,
					     player->num_bridges,
					     player->num_ships,
					     player->num_settlements,
					     player->num_cities,
					     player->num_soldiers,
					     player->chapel_played,
					     player->univ_played,
					     player->gov_played,
					     player->libr_played,
					     player->market_played,
					     (player->num ==
					      longestroadpnum),
					     (player->num ==
					      largestarmypnum));
			player_send_uncached(player, FIRST_VERSION,
					     V0_12,
					     "playerinfo: %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
					     player->num_roads,
					     player->num_bridges,
					     player->num_ships,
					     player->num_settlements,
					     player->num_cities,
					     player->num_soldiers,
					     0,
					     player->chapel_played,
					     player->univ_played,
					     player->gov_played,
					     player->libr_played,
					     player->market_played,
					     (player->num ==
					      longestroadpnum),
					     (player->num ==
					      largestarmypnum));

			/* Send special points */
			list = player->special_points;
			while (list) {
				Points *points = list->data;
				player_send_uncached(player,
						     FIRST_VERSION,
						     LATEST_VERSION,
						     "get-point %d %d %d %s\n",
						     player->num,
						     points->id,
						     points->points,
						     points->name);
				list = g_list_next(list);
			}

			/* Send info about other players */
			for (next = player_first_real(game); next != NULL;
			     next = player_next_real(next)) {
				Player *p = (Player *) next->data;
				gint numassets = 0;
				if (p->num == player->num)
					continue;
				for (i = 0; i < NO_RESOURCE; i++) {
					numassets += p->assets[i];
				}
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "otherplayerinfo: %d %d %d %d %d %d %d %d %d %d %d\n",
						     p->num, numassets,
						     deck_count(p->devel),
						     p->num_soldiers,
						     p->chapel_played,
						     p->univ_played,
						     p->gov_played,
						     p->libr_played,
						     p->market_played,
						     (p->num ==
						      longestroadpnum),
						     (p->num ==
						      largestarmypnum));

				/* Send special points */
				list = p->special_points;
				while (list) {
					Points *points = list->data;
					player_send_uncached(player,
							     FIRST_VERSION,
							     LATEST_VERSION,
							     "get-point %d %d %d %s\n",
							     p->num,
							     points->id,
							     points->
							     points,
							     points->name);
					list = g_list_next(list);
				}
			}

			/* Send build info for the current player
			   - what builds the player was in the process
			   of building when he disconnected */
			for (next = player->build_list;
			     next != NULL; next = g_list_next(next)) {
				BuildRec *build = (BuildRec *) next->data;
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "buildinfo: %B %d %d %d\n",
						     build->type, build->x,
						     build->y, build->pos);
			}

			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION, "end\n");
			return TRUE;
		}
		if (sm_recv(sm, "start")) {
			player_send_uncached(player, FIRST_VERSION,
					     LATEST_VERSION, "OK\n");

			/* Some player was in the setup phase */
			if (game->setup_player != NULL
			    && (Player *) game->setup_player->data !=
			    player) {
				gint num =
				    ((Player *) (game->setup_player->
						 data))->num;
				if (game->double_setup)
					player_send_uncached(player,
							     FIRST_VERSION,
							     LATEST_VERSION,
							     "player %d setup-double\n",
							     num);
				else
					player_send_uncached(player,
							     FIRST_VERSION,
							     LATEST_VERSION,
							     "player %d setup %d\n",
							     num,
							     game->
							     reverse_setup);
			}

			if (recover_from_plenty) {
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "plenty %R\n",
						     game->bank_deck);
				recover_from_plenty = FALSE;
			}

			/* send discard and gold info for all players */
			for (next = player_first_real(game); next != NULL;
			     next = player_next_real(next)) {
				Player *p = (Player *) next->data;
				if (p->discard_num > 0) {
					player_send_uncached(player,
							     FIRST_VERSION,
							     LATEST_VERSION,
							     "player %d must-discard %d\n",
							     p->num,
							     p->
							     discard_num);
				}
				if (p->gold > 0) {
					player_send_uncached(player,
							     FIRST_VERSION,
							     LATEST_VERSION,
							     "player %d prepare-gold %d\n",
							     p->num,
							     p->gold);

				}
			}

			/* The current player was choosing gold */
			state = sm_stack_inspect(sm, 1);
			if (state == (StateFunc) mode_choose_gold) {
				gint limited_bank[NO_RESOURCE];
				gold_limited_bank(game, player->gold,
						  limited_bank);
				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "choose-gold %d %R\n",
						     player->gold,
						     limited_bank);
			}

			/* Trade was in progress */
			if (game->curr_player != -1 &&
			    (StateFunc) mode_domestic_initiate ==
			    sm_stack_inspect(player_by_num
					     (game, game->curr_player)->sm,
					     0)) {
				QuoteInfo *quote =
				    quotelist_first(game->quotes);

				player_send_uncached(player, FIRST_VERSION,
						     LATEST_VERSION,
						     "player %d domestic-trade call supply %R receive %R\n",
						     game->curr_player,
						     game->quote_supply,
						     game->quote_receive);
				while (quote) {
					if (quote->is_domestic) {
						player_send_uncached
						    (player, FIRST_VERSION,
						     LATEST_VERSION,
						     "player %d domestic-quote quote %d supply %R receive %R\n",
						     quote->var.d.
						     player_num,
						     quote->var.d.
						     quote_num,
						     quote->var.d.supply,
						     quote->var.d.receive);
					}
					quote = quotelist_next(quote);
				}
				/* The player already rejected all quotes,
				 * send reject again */
				if (state == (StateFunc)
				    mode_domestic_quote_rejected) {
					player_send_uncached(player,
							     FIRST_VERSION,
							     LATEST_VERSION,
							     "player %d domestic-quote finish\n",
							     player->num);
				}
			}
			sm_set_use_cache(sm, FALSE);

			if (player->disconnected) {
				player->disconnected = FALSE;
				driver->player_change(game);
				if (!sm_is_connected(sm))
					/* This happens when the connection is
					 * dropped when the cache is sent */
					sm_goto(sm, (StateFunc)
						mode_spectator);
				else {
					if (sm_stack_inspect(sm, 1) !=
					    NULL) {
						sm_pop(sm);
					} else {
						/* During the previous
						 * connection attempt, the
						 * connection was lost too
						 * soon */
						sm_goto(sm, (StateFunc)
							mode_idle);
					}
				}
			} else {
				if (!player_is_spectator
				    (game, player->num))
					sm_goto(sm, (StateFunc) mode_idle);
				else
					sm_goto(sm, (StateFunc)
						mode_spectator);
			}
			try_start_game(game);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}
