/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2003,2006 Bas Wijnen <shevek@fmf.nl>
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
#include "callback.h"
#include "state.h"
#include "client.h"
#include "cost.h"

/* callbacks is a pointer to an array of function pointers.
 * It is filled in by the front end. */
struct callbacks callbacks;

/* current callback mode */
enum callback_mode callback_mode;

/* is chat currently colourful? */
gboolean color_chat_enabled;

void cb_connect(const gchar * server, const gchar * port,
		gboolean spectator)
{
	/* connect to a server */
	g_assert(callback_mode == MODE_INIT);
	requested_spectator = spectator;
	if (sm_connect(SM(), server, port)) {
		if (sm_is_connected(SM())) {
			sm_goto(SM(), mode_start);
		} else {
			sm_goto(SM(), mode_connecting);
		}
	} else {
		callbacks.offline();
	}
}

void cb_disconnect(void)
{
	sm_close(SM());
	callback_mode = MODE_INIT;
	callbacks.offline();
}

void cb_roll(void)
{
	/* roll dice */
	g_assert(callback_mode == MODE_TURN && !have_rolled_dice());
	sm_send(SM(), "roll\n");
	/* This should really be sm_push, but on return it should be
	 * sm_pop_noenter; sm_goto_noenter; sm_push, and since sm_pop_noenter
	 * doesn't exist, the combination is changed into
	 * sm_goto here and sm_goto_noenter; sm_push later. */
	sm_goto(SM(), mode_roll_response);
}

void cb_build_road(const Edge * edge)
{
	/* build road */
	g_assert(callback_mode == MODE_TURN
		 || callback_mode == MODE_ROAD_BUILD
		 || callback_mode == MODE_SETUP);
	sm_send(SM(), "build road %d %d %d\n", edge->x, edge->y,
		edge->pos);
	sm_push(SM(), mode_build_response);
}

void cb_build_ship(const Edge * edge)
{
	/* build ship */
	g_assert(callback_mode == MODE_TURN
		 || callback_mode == MODE_ROAD_BUILD
		 || callback_mode == MODE_SETUP);
	sm_send(SM(), "build ship %d %d %d\n", edge->x, edge->y,
		edge->pos);
	sm_push(SM(), mode_build_response);
}

void cb_build_bridge(const Edge * edge)
{
	/* build bridge */
	g_assert(callback_mode == MODE_TURN
		 || callback_mode == MODE_ROAD_BUILD
		 || callback_mode == MODE_SETUP);
	sm_send(SM(), "build bridge %d %d %d\n", edge->x, edge->y,
		edge->pos);
	sm_push(SM(), mode_build_response);
}

void cb_move_ship(const Edge * from, const Edge * to)
{
	/* move ship */
	g_assert(callback_mode == MODE_TURN);
	sm_send(SM(), "move %d %d %d %d %d %d\n",
		from->x, from->y, from->pos, to->x, to->y, to->pos);
	sm_push(SM(), mode_move_response);
}

void cb_build_settlement(const Node * node)
{
	/* build settlement */
	g_assert(callback_mode == MODE_TURN
		 || callback_mode == MODE_SETUP);
	sm_send(SM(), "build settlement %d %d %d\n",
		node->x, node->y, node->pos);
	sm_push(SM(), mode_build_response);
}

void cb_build_city(const Node * node)
{
	/* build city */
	g_assert(callback_mode == MODE_TURN);
	sm_send(SM(), "build city %d %d %d\n", node->x, node->y,
		node->pos);
	sm_push(SM(), mode_build_response);
}

void cb_build_city_wall(const Node * node)
{
	/* build city */
	g_assert(callback_mode == MODE_TURN);
	sm_send(SM(), "build city_wall %d %d %d\n", node->x, node->y,
		node->pos);
	sm_push(SM(), mode_build_response);
}

void cb_buy_develop(void)
{
	/* buy development card */
	g_assert(callback_mode == MODE_TURN && can_buy_develop());
	sm_send(SM(), "buy-develop\n");
	sm_push(SM(), mode_buy_develop_response);
}

void cb_play_develop(guint card)
{
	/* play development card */
	g_assert(callback_mode == MODE_TURN && can_play_develop(card));
	sm_send(SM(), "play-develop %u\n", card);
	sm_push(SM(), mode_play_develop_response);
}

void cb_undo(void)
{
	/* undo a move */
	g_assert(callback_mode == MODE_TURN
		 || callback_mode == MODE_ROAD_BUILD
		 || callback_mode == MODE_SETUP
		 || callback_mode == MODE_ROB);
	sm_send(SM(), "undo\n");
	sm_push(SM(), mode_undo_response);
}

void cb_maritime(gint ratio, Resource supply, Resource receive)
{
	/* trade with the bank */
	g_assert(callback_mode == MODE_TURN
		 || callback_mode == MODE_DOMESTIC);
	sm_send(SM(), "maritime-trade %d supply %r receive %r\n",
		ratio, supply, receive);
	sm_push(SM(), mode_trade_maritime_response);
}

void cb_domestic(const gint * supply, const gint * receive)
{
	/* call for quotes */
	g_assert(callback_mode == MODE_TURN
		 || callback_mode == MODE_DOMESTIC);
	sm_send(SM(), "domestic-trade call supply %R receive %R\n",
		supply, receive);
	if (callback_mode == MODE_DOMESTIC) {
		sm_push(SM(), mode_trade_call_again_response);
	} else {
		sm_push(SM(), mode_trade_call_response);
	}
}

void cb_end_turn(void)
{
	/* end turn or road building or setup */
	g_assert(callback_mode == MODE_TURN
		 || callback_mode == MODE_ROAD_BUILD
		 || callback_mode == MODE_SETUP);
	sm_send(SM(), "done\n");
	sm_push(SM(), mode_done_response);
}

void cb_place_robber(const Hex * hex)
{
	/* place robber */
	g_assert(callback_mode == MODE_ROBBER);
	sm_send(SM(), "move-robber %d %d\n", hex->x, hex->y);
	sm_push(SM(), mode_robber_move_response);
}

void cb_rob(gint victim_num)
{
	/* after placing the robber, rob someone */
	g_assert(callback_mode == MODE_ROB);
	sm_send(SM(), "rob %d\n", victim_num);
	sm_push(SM(), mode_robber_response);
}

void cb_choose_monopoly(gint resource)
{
	/* choose a monopoly resource */
	g_assert(callback_mode == MODE_MONOPOLY);
	sm_send(SM(), "monopoly %r\n", resource);
	sm_push(SM(), mode_monopoly_response);
}

void cb_choose_plenty(gint * resources)
{
	/* choose year of plenty resources */
	g_assert(callback_mode == MODE_PLENTY);
	sm_send(SM(), "plenty %R\n", resources);
	sm_push(SM(), mode_year_of_plenty_response);
}

void cb_trade(gint player, gint quote, const gint * supply,
	      const gint * receive)
{
	/* accept a domestic trade */
	g_assert(callback_mode == MODE_DOMESTIC);
	sm_send(SM(),
		"domestic-trade accept player %d quote %d supply %R receive %R\n",
		player, quote, supply, receive);
	sm_push(SM(), mode_trade_domestic_response);
}

void cb_end_trade(void)
{
	/* stop trading */
	g_assert(callback_mode == MODE_DOMESTIC);
	sm_send(SM(), "domestic-trade finish\n");
	sm_push(SM(), mode_domestic_finish_response);
}

void cb_quote(gint num, const gint * supply, const gint * receive)
{
	/* make a quote */
	g_assert(callback_mode == MODE_QUOTE);
	sm_send(SM(), "domestic-quote quote %d supply %R receive %R\n",
		num, supply, receive);
	sm_push(SM(), mode_quote_submit_response);
}

void cb_delete_quote(gint num)
{
	/* revoke a quote */
	g_assert(callback_mode == MODE_QUOTE);
	sm_send(SM(), "domestic-quote delete %d\n", num);
	sm_push(SM(), mode_quote_delete_response);
}

void cb_end_quote(void)
{
	/* stop trading */
	g_assert(callback_mode == MODE_QUOTE);
	sm_send(SM(), "domestic-quote finish\n");
	sm_push(SM(), mode_quote_finish_response);
}

void cb_chat(const gchar * text)
{
	/* chat a message */
	g_assert(callback_mode != MODE_INIT);
	sm_send(SM(), "chat %s\n", text);
}

void cb_name_change(const gchar * name)
{
	/* change your name */
	g_assert(callback_mode != MODE_INIT);
	sm_send(SM(), "name %s\n", name);
}

void cb_style_change(const gchar * style)
{
	/* change your style */
	g_assert(callback_mode != MODE_INIT);
	sm_send(SM(), "style %s\n", style);
}

void cb_discard(const gint * resources)
{
	/* discard resources */
	g_assert(callback_mode == MODE_DISCARD);
	callback_mode = MODE_DISCARD_WAIT;
	sm_send(SM(), "discard %R\n", resources);
}

void cb_choose_gold(const gint * resources)
{
	/* choose gold */
	g_assert(callback_mode == MODE_GOLD);
	callback_mode = MODE_GOLD_WAIT;
	sm_send(SM(), "chose-gold %R\n", resources);
}

gboolean have_ships(void)
{
	return game_params == NULL
	    || game_params->num_build_type[BUILD_SHIP] > 0;
}

gboolean have_bridges(void)
{
	return game_params == NULL
	    || game_params->num_build_type[BUILD_BRIDGE] > 0;
}

gboolean have_city_walls(void)
{
	return game_params == NULL
	    || game_params->num_build_type[BUILD_CITY_WALL] > 0;
}

const GameParams *get_game_params(void)
{
	return game_params;
}

gint game_resources(void)
{
	return game_params->resource_count;
}

guint game_victory_points(void)
{
	return game_params->victory_points;
}

gint stat_get_vp_value(StatisticType type)
{
	/* victory point values of all the statistic types */
	static gint stat_vp_values[] = {
		1,		/* settlement */
		2,		/* city */
		0,		/* city wall */
		2,		/* largest army */
		2,		/* longest road */
		1,		/* chapel */
		1,		/* pioneer university */
		1,		/* governor's house */
		1,		/* library */
		1,		/* market */
		0,		/* soldier */
		0,		/* resource card */
		0,		/* development card */
	};

	return stat_vp_values[type];
}

gboolean can_undo(void)
{
	return build_can_undo();
}

gboolean road_building_can_build_road(void)
{
	return build_count_edges() < 2
	    && stock_num_roads() > 0
	    && map_can_place_road(callbacks.get_map(), my_player_num());
}

gboolean road_building_can_build_ship(void)
{
	return build_count_edges() < 2
	    && stock_num_ships() > 0
	    && map_can_place_ship(callbacks.get_map(), my_player_num());
}

gboolean road_building_can_build_bridge(void)
{
	return build_count_edges() < 2
	    && stock_num_bridges() > 0
	    && map_can_place_bridge(callbacks.get_map(), my_player_num());
}

gboolean road_building_can_finish(void)
{
	return !road_building_can_build_road()
	    && !road_building_can_build_ship()
	    && !road_building_can_build_bridge()
	    && build_is_valid();
}

gboolean turn_can_build_road(void)
{
	return have_rolled_dice()
	    && stock_num_roads() > 0 && can_afford(cost_road())
	    && map_can_place_road(callbacks.get_map(), my_player_num());
}

gboolean turn_can_build_ship(void)
{
	return have_rolled_dice()
	    && stock_num_ships() > 0 && can_afford(cost_ship())
	    && map_can_place_ship(callbacks.get_map(), my_player_num());
}

gboolean turn_can_build_bridge(void)
{
	return have_rolled_dice()
	    && stock_num_bridges() > 0 && can_afford(cost_bridge())
	    && map_can_place_bridge(callbacks.get_map(), my_player_num());
}

gboolean turn_can_build_settlement(void)
{
	return have_rolled_dice()
	    && stock_num_settlements() > 0 && can_afford(cost_settlement())
	    && map_can_place_settlement(callbacks.get_map(),
					my_player_num());
}

gboolean turn_can_build_city(void)
{
	return have_rolled_dice()
	    && stock_num_cities() > 0
	    && can_afford(cost_upgrade_settlement())
	    && map_can_upgrade_settlement(callbacks.get_map(),
					  my_player_num());
}

gboolean turn_can_build_city_wall(void)
{
	return have_rolled_dice()
	    && stock_num_city_walls() > 0 && can_afford(cost_city_wall())
	    && map_can_place_city_wall(callbacks.get_map(),
				       my_player_num());
}

gboolean turn_can_trade(void)
{
	/* We are not allowed to trade before we have rolled the dice,
	 * or after we have done built a settlement / city, or after
	 * buying a development card.
	 */
	if (!have_rolled_dice())
		return FALSE;

	if (game_params->strict_trade
	    && (have_built() || have_bought_develop()))
		return FALSE;

	return can_trade_maritime()
	    || can_trade_domestic();
}

static gboolean really_try_move_ship(const Hex * hex, gpointer closure)
{
	guint idx;
	const Edge *from = closure;
	for (idx = 0; idx < G_N_ELEMENTS(hex->edges); ++idx) {
		const Edge *edge;
		edge = hex->edges[idx];
		if (edge->x != hex->x || edge->y != hex->y)
			continue;
		if (can_move_ship(from, edge))
			return TRUE;
	}
	return FALSE;
}

gboolean can_move_ship(const Edge * from, const Edge * to)
{
	gboolean retval;
	gint owner;
	Edge *ship_sailed_from_here;

	if (to == from)
		return FALSE;
	g_assert(from->type == BUILD_SHIP);
	owner = from->owner;
	if (!can_ship_be_moved(from, owner))
		return FALSE;
	ship_sailed_from_here = map_edge(callbacks.get_map(), from->x, from->y, from->pos);	/* Copy to non-const pointer */
	ship_sailed_from_here->owner = -1;
	ship_sailed_from_here->type = BUILD_NONE;
	retval = can_ship_be_built(to, owner);
	ship_sailed_from_here->owner = owner;
	ship_sailed_from_here->type = BUILD_SHIP;
	return retval;
}

static gboolean try_move_ship(const Hex * hex,
			      G_GNUC_UNUSED gpointer closure)
{
	guint idx;
	for (idx = 0; idx < G_N_ELEMENTS(hex->edges); ++idx) {
		Edge *edge;	/* Huh? Can non-const be taken from const Hex ? */
		edge = hex->edges[idx];
		if (edge->x != hex->x || edge->y != hex->y)
			continue;
		if (edge->owner != my_player_num()
		    || edge->type != BUILD_SHIP)
			continue;
		if (map_traverse_const
		    (callbacks.get_map(), really_try_move_ship, edge))
			return TRUE;
	}
	return FALSE;
}

gboolean turn_can_move_ship(void)
{
	if (!have_rolled_dice() || callbacks.get_map()->has_moved_ship)
		return FALSE;
	return map_traverse_const(callbacks.get_map(), try_move_ship,
				  NULL);
}

guint robber_count_victims(const Hex * hex, gint * victim_list)
{
	guint idx;
	guint node_idx;
	guint num_victims;

	/* If there is no-one to steal from, or the players have no
	 * resources, we do not go into steal_resource.
	 */
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); idx++)
		victim_list[idx] = -1;
	num_victims = 0;
	for (node_idx = 0; node_idx < G_N_ELEMENTS(hex->nodes); node_idx++) {
		Node *node = hex->nodes[node_idx];
		Player *owner;

		if (node->type == BUILD_NONE
		    || node->owner == my_player_num())
			/* Can't steal from myself
			 */
			continue;

		/* Check if the node owner has any resources
		 */
		owner = player_get(node->owner);
		if (owner->statistics[STAT_RESOURCES] > 0) {
			/* Has resources - we can steal
			 */
			for (idx = 0; idx < num_victims; idx++)
				if (victim_list[idx] == node->owner)
					break;
			if (idx == num_victims)
				victim_list[num_victims++] = node->owner;
		}
	}

	return num_victims;
}

guint pirate_count_victims(const Hex * hex, gint * victim_list)
{
	guint idx;
	guint edge_idx;
	guint num_victims;

	/* If there is no-one to steal from, or the players have no
	 * resources, we do not go into steal_resource.
	 */
	for (idx = 0; idx < G_N_ELEMENTS(hex->edges); idx++)
		victim_list[idx] = -1;
	num_victims = 0;
	for (edge_idx = 0; edge_idx < G_N_ELEMENTS(hex->edges); edge_idx++) {
		Edge *edge = hex->edges[edge_idx];
		Player *owner;

		if (edge->type != BUILD_SHIP
		    || edge->owner == my_player_num())
			/* Can't steal from myself
			 */
			continue;

		/* Check if the node owner has any resources
		 */
		owner = player_get(edge->owner);
		if (owner->statistics[STAT_RESOURCES] > 0) {
			/* Has resources - we can steal
			 */
			for (idx = 0; idx < num_victims; idx++)
				if (victim_list[idx] == edge->owner)
					break;
			if (idx == num_victims)
				victim_list[num_victims++] = edge->owner;
		}
	}

	return num_victims;
}
