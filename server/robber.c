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
#include "random.h"

static Hex *previous_robber_hex;

static void move_pirate(Player * player, Hex * hex, gboolean is_undo)
{
	Map *map = hex->map;

	previous_robber_hex = map->pirate_hex;
	map->pirate_hex = hex;
	/* 0.10 didn't know about undo for movement, so move happens
	 * only after stealing has been done.  */
	if (is_undo) {
		player_broadcast(player, PB_ALL, V0_11, LATEST_VERSION,
				 "unmoved-pirate %d %d\n", hex->x, hex->y);
	} else {
		player_broadcast(player, PB_ALL, V0_11, LATEST_VERSION,
				 "moved-pirate %d %d\n", hex->x, hex->y);
	}
}

static void move_robber(Player * player, Hex * hex, gboolean is_undo)
{
	Map *map = hex->map;

	previous_robber_hex = map->robber_hex;
	if (map->robber_hex)
		map->robber_hex->robber = FALSE;
	map->robber_hex = hex;
	map->robber_hex->robber = TRUE;
	/* 0.10 didn't know about undo for movement, so move happens
	 * only after stealing has been done.  */
	if (is_undo) {
		player_broadcast(player, PB_ALL, V0_11, LATEST_VERSION,
				 "unmoved-robber %d %d\n", hex->x, hex->y);
	} else {
		player_broadcast(player, PB_ALL, V0_11, LATEST_VERSION,
				 "moved-robber %d %d\n", hex->x, hex->y);
	}
}

static void steal_card_from(Player * player, Player * victim)
{
	Game *game = player->game;
	guint idx;
	gint num;
	gint steal;
	GList *list;

	/* Work out how many cards the victim has
	 */
	num = 0;
	for (idx = 0; idx < G_N_ELEMENTS(victim->assets); idx++)
		num += victim->assets[idx];
	if (num == 0) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR no-resources\n");
		return;
	}

	/* Work out which card to steal from the victim
	 */
	steal = random_guint(num);
	for (idx = 0; idx < G_N_ELEMENTS(victim->assets); idx++) {
		steal -= victim->assets[idx];
		if (steal < 0)
			break;
	}
	/* Now inform the various parties of the theft.  All
	 * interested parties find out which card was stolen, the
	 * others just hear about the theft.
	 */
	for (list = game->player_list; list != NULL;
	     list = g_list_next(list)) {
		Player *scan = list->data;

		if (scan->num >= 0 && !scan->disconnected) {
			if (scan == player || scan == victim) {
				player_send(scan, FIRST_VERSION,
					    LATEST_VERSION,
					    "player %d stole %r from %d\n",
					    player->num, idx, victim->num);
			} else
				player_send(scan, FIRST_VERSION,
					    LATEST_VERSION,
					    "player %d stole from %d\n",
					    player->num, victim->num);
		}
	}
	/* Alter the assets of the respective players
	 */
	player->assets[idx]++;
	victim->assets[idx]--;
}

static void done_robbing_pre_steal(Player * player)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Hex *hex = map_robber_hex(map);
	if (hex) {
		player_broadcast(player, PB_RESPOND, FIRST_VERSION, V0_10,
				 "moved-robber %d %d\n", hex->x, hex->y);
	}
}

static void done_robbing_post_steal(Player * player)
{
	sm_pop(player->sm);
	player_send(player, V0_11, LATEST_VERSION, "robber-done\n");
}

static void do_select_robbed(Player * player, Hex * hex, gint victim_num)
{
	Game *game = player->game;
	Player *owner;
	Resource resource;
	guint idx;

	/* Check if the victim has any resources
	 */
	owner = player_by_num(game, victim_num);
	if (!owner) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-player: not found\n");
		return;
	}
	for (resource = 0; resource < NO_RESOURCE; resource++)
		if (owner->assets[resource] != 0)
			break;
	if (resource == NO_RESOURCE) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-player: no resources\n");
		return;
	}
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); idx++) {
		Node *node = hex->nodes[idx];

		if (node->type == BUILD_NONE || node->owner != victim_num)
			continue;

		/* Victim has resources and has a building there: steal.  */
		done_robbing_pre_steal(player);
		steal_card_from(player, owner);
		done_robbing_post_steal(player);
		return;
	}

	player_send(player, FIRST_VERSION, LATEST_VERSION,
		    "ERR bad-player: no buildings\n");
}

/* Wait for the player to select a building to rob
 */
gboolean mode_select_robbed(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	gint victim_num;
	Hex *hex;

	sm_state_name(sm, "mode_select_robbed");

	hex = map_robber_hex(map);

	if (event == SM_ENTER) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "rob %d %d\n", hex->x, hex->y);
		return TRUE;
	}

	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "undo")) {
		robber_undo(player);
		return TRUE;
	}

	if (!sm_recv(sm, "rob %d", &victim_num))
		return FALSE;

	do_select_robbed(player, hex, victim_num);
	return TRUE;
}


static void do_select_pirated(Player * player, Hex * hex, gint victim_num)
{
	Game *game = player->game;
	Player *owner;
	Resource resource;
	guint idx;

	/* Check if the victim has any resources
	 */
	owner = player_by_num(game, victim_num);
	if (!owner) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-player: not found\n");
		return;
	}
	for (resource = 0; resource < NO_RESOURCE; resource++)
		if (owner->assets[resource] != 0)
			break;
	if (resource == NO_RESOURCE) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-player: no resources\n");
		return;
	}
	for (idx = 0; idx < G_N_ELEMENTS(hex->edges); ++idx) {
		Edge *edge = hex->edges[idx];

		if (edge->type != BUILD_SHIP || edge->owner != victim_num)
			continue;

		/* Victim has resources and has a ship there: steal.  */
		done_robbing_pre_steal(player);
		steal_card_from(player, owner);
		done_robbing_post_steal(player);
		return;
	}

	player_send(player, FIRST_VERSION, LATEST_VERSION,
		    "ERR bad-player: no ships\n");
}

/* Wait for the player to select a ship to rob
 */
gboolean mode_select_pirated(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	gint victim_num;
	Hex *hex;

	sm_state_name(sm, "mode_select_pirated");

	hex = map_pirate_hex(map);

	if (event == SM_ENTER) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "rob %d %d\n", hex->x, hex->y);
		return TRUE;
	}

	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "undo")) {
		robber_undo(player);
		return TRUE;
	}

	if (!sm_recv(sm, "rob %d", &victim_num))
		return FALSE;

	do_select_pirated(player, hex, victim_num);
	return TRUE;
}

/* Wait for the player to place the robber
 */
gboolean mode_place_robber(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	Map *map = game->params->map;
	gint x, y;
	Hex *hex;
	guint idx;
	gint one_victim;
	gint victim_num = -1;
	gboolean old_style;

	sm_state_name(sm, "mode_place_robber");

	if (event != SM_RECV)
		return FALSE;

	if (sm_recv(sm, "move-robber %d %d %d", &x, &y, &victim_num))
		old_style = TRUE;
	else if (sm_recv(sm, "move-robber %d %d", &x, &y))
		old_style = FALSE;
	else
		return FALSE;

	hex = map_hex(map, x, y);
	if (hex == NULL || !can_robber_or_pirate_be_moved(hex)) {
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "ERR bad-pos\n");
		return TRUE;
	}

	/* check if the pirate was moved.
	 */
	if (hex->terrain == SEA_TERRAIN) {
		move_pirate(player, hex, FALSE);

		/* If there is no-one to steal from, or the players have no
		 * resources, we cannot steal resources.
		 */
		one_victim = -1;
		for (idx = 0; idx < G_N_ELEMENTS(hex->edges); ++idx) {
			Edge *edge = hex->edges[idx];
			Player *owner;
			Resource resource;

			if (edge->type != BUILD_SHIP
			    || edge->owner == player->num)
				/* Can't steal from myself
				 */
				continue;

			/* Check if the node owner has any resources
			 */
			owner = player_by_num(game, edge->owner);
			for (resource = 0; resource < NO_RESOURCE;
			     resource++)
				if (owner->assets[resource] != 0)
					break;
			if (resource == NO_RESOURCE)
				continue;

			/* Has resources - we can steal
			 */
			if (one_victim == owner->num)
				/* We already knew about this player.
				 */
				continue;
			if (one_victim >= 0)
				/* This is the second victim, which means
				 * there is choice.  That's all we need to
				 * know.
				 */
				break;
			one_victim = owner->num;
		}
		if (idx != G_N_ELEMENTS(hex->edges)) {
			/* There is choice for stealing.  Wait for the
			 * user to choose.  */
			if (old_style) {
				/* The user already chose.  */
				do_select_pirated(player, hex, victim_num);
			} else
				sm_goto(sm,
					(StateFunc) mode_select_pirated);
			return TRUE;
		}
		if (one_victim < 0) {
			/* No one to steal from - resume turn
			 */
			done_robbing_pre_steal(player);
			done_robbing_post_steal(player);
			return TRUE;
		}
		/* Only one victim: automatically steal.  */
		done_robbing_pre_steal(player);
		steal_card_from(player, player_by_num(game, one_victim));
		done_robbing_post_steal(player);
		return TRUE;
	}

	/* It wasn't the pirate; it was the robber. */

	move_robber(player, hex, FALSE);

	/* If there is no-one to steal from, or the players have no
	 * resources, we cannot steal resources.
	 */
	one_victim = -1;
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); idx++) {
		Node *node = hex->nodes[idx];
		Player *owner;
		Resource resource;

		if (node->type == BUILD_NONE || node->owner == player->num)
			/* Can't steal from myself
			 */
			continue;

		/* Check if the node owner has any resources
		 */
		owner = player_by_num(game, node->owner);
		for (resource = 0; resource < NO_RESOURCE; resource++)
			if (owner->assets[resource] != 0)
				break;
		if (resource == NO_RESOURCE)
			continue;

		/* Has resources - we can steal
		 */
		if (one_victim == owner->num)
			/* We already knew about this player.
			 */
			continue;
		if (one_victim >= 0)
			/* This is the second victim, which means
			 * there is choice.  That's all we need to
			 * know.
			 */
			break;
		one_victim = owner->num;
	}
	if (idx != G_N_ELEMENTS(hex->nodes)) {
		/* There is choice for stealing.  Wait for the user to choose.
		 */
		if (old_style) {
			/* The user already chose.  */
			do_select_robbed(player, hex, victim_num);
		} else
			sm_goto(sm, (StateFunc) mode_select_robbed);
		return TRUE;
	}
	if (one_victim < 0) {
		/* No one to steal from - resume turn
		 */
		done_robbing_pre_steal(player);
		done_robbing_post_steal(player);
		return TRUE;
	}
	/* Only one victim: automatically steal.  */
	done_robbing_pre_steal(player);
	steal_card_from(player, player_by_num(game, one_victim));
	done_robbing_post_steal(player);
	return TRUE;
}

void robber_place(Player * player)
{
	StateMachine *sm = player->sm;
	player_broadcast(player, PB_OTHERS, FIRST_VERSION, LATEST_VERSION,
			 "is-robber\n");
	player_send(player, FIRST_VERSION, LATEST_VERSION,
		    "you-are-robber\n");
	sm_push(sm, (StateFunc) mode_place_robber);
}

void robber_undo(Player * player)
{
	if (previous_robber_hex->terrain == SEA_TERRAIN)
		move_pirate(player, previous_robber_hex, TRUE);
	else
		move_robber(player, previous_robber_hex, TRUE);
	sm_goto(player->sm, (StateFunc) mode_place_robber);
	player_send(player, V0_11, LATEST_VERSION, "undo-robber\n");
}
