/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
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
#include <string.h>

#include "game.h"
#include "map.h"
#include "client.h"
#include "cost.h"
#include "log.h"
#include "callback.h"

static Player players[MAX_PLAYERS];
static GList *spectators;

static gint turn_player = -1;	/* whose turn is it */
static gint my_player_id = -1;	/* what is my player number */
static gint num_total_players = 4;	/* total number of players in the game */

/* this function is called when the game starts, to clean up from the
 * previous game. */
void player_reset(void)
{
	guint i;
	guint idx;

	/* remove all spectators */
	while (spectators != NULL) {
		Spectator *spectator = spectators->data;
		g_free(spectator->name);
		g_free(spectator->style);
		spectators = g_list_remove(spectators, spectator);
	}
	/* free player's memory */
	for (i = 0; i < MAX_PLAYERS; ++i) {
		if (players[i].name != NULL) {
			g_free(players[i].name);
			players[i].name = NULL;
		}
		if (players[i].style != NULL) {
			g_free(players[i].style);
			players[i].style = NULL;
		}
		while (players[i].points != NULL) {
			Points *points = players[i].points->data;
			players[i].points =
			    g_list_remove(players[i].points, points);
			points_free(points);
			g_free(points);
		}
		for (idx = 0; idx < G_N_ELEMENTS(players[i].statistics);
		     ++idx)
			players[i].statistics[idx] = 0;
	}
}

Player *player_get(gint num)
{
	return &players[num];
}

gboolean player_is_spectator(gint num)
{
	return num < 0 || num >= num_total_players;
}

Spectator *spectator_get(gint num)
{
	GList *list;
	for (list = spectators; list != NULL; list = g_list_next(list)) {
		Spectator *spectator = list->data;
		if (spectator->num == num)
			break;
	}
	if (list)
		return list->data;
	return NULL;
}

/** Find a spectator with the given name.
 *  @param name The name to find
 *  @return -1 if the name is not found
 */
gint find_spectator_by_name(const gchar * name)
{
	GList *list;
	for (list = spectators; list != NULL; list = g_list_next(list)) {
		Spectator *spectator = list->data;
		if (!strcmp(spectator->name, name))
			return spectator->num;
	}
	return -1;
}

const gchar *player_name(gint player_num, gboolean word_caps)
{
	static gchar buff[256];
	if (player_num >= num_total_players) {
		/* this is about a spectator */
		Spectator *spectator = spectator_get(player_num);
		if (spectator != NULL)
			return spectator->name;
		else {
			if (word_caps)
				sprintf(buff, _("Spectator %d"),
					player_num);
			else
				sprintf(buff, _("spectator %d"),
					player_num);
			return buff;
		}
	} else if (player_num >= 0) {
		Player *player = player_get(player_num);
		return player->name;
	}
	if (word_caps)
		sprintf(buff, _("Player %d"), player_num);
	else
		sprintf(buff, _("player %d"), player_num);
	return buff;
}

gint player_get_score(gint player_num)
{
	Player *player = player_get(player_num);
	GList *list;
	guint i;
	gint score;

	for (i = 0, score = 0; i < G_N_ELEMENTS(player->statistics); i++) {
		score += stat_get_vp_value(i) * player->statistics[i];
	}

	list = player->points;
	while (list) {
		Points *points = list->data;
		score += points->points;
		list = g_list_next(list);
	}
	return score;
}

gint my_player_num(void)
{
	return my_player_id;
}

gint num_players(void)
{
	return num_total_players;
}

void player_set_my_num(gint player_num)
{
	my_player_id = player_num;
}

void player_modify_statistic(gint player_num, StatisticType type, gint num)
{
	Player *player = player_get(player_num);
	player->statistics[type] += num;
	callbacks.new_statistics(player_num, type, num);
}

void player_modify_points(gint player_num, Points * points, gboolean added)
{
	Player *player = player_get(player_num);
	/* if !added -> is already removed */
	if (added)
		player->points = g_list_append(player->points, points);
	callbacks.new_points(player_num, points, added);
}

void player_change_name(gint player_num, const gchar * name)
{
	Player *player;
	gchar *old_name;

	if (player_num < 0)
		return;
	if (player_num >= num_total_players) {
		/* this is about a spectator */
		Spectator *spectator = spectator_get(player_num);
		if (spectator == NULL) {
			/* there is a new spectator */
			spectator = g_malloc0(sizeof(*spectator));
			spectators = g_list_prepend(spectators, spectator);
			spectator->num = player_num;
			spectator->name = NULL;
			spectator->style = g_strdup(default_player_style);
			old_name = NULL;
		} else {
			old_name = spectator->name;
		}
		if (old_name == NULL)
			log_message(MSG_INFO, _("New spectator: %s.\n"),
				    name);
		else
			log_message(MSG_INFO, _("%s is now %s.\n"),
				    old_name, name);
		spectator->name = g_strdup(name);
		if (old_name != NULL)
			g_free(old_name);
		callbacks.spectator_name(player_num, name);
		return;
	}

	player = player_get(player_num);
	old_name = player->name;
	player->name = g_strdup(name);
	if (old_name == NULL)
		log_message(MSG_INFO, _("Player %d is now %s.\n"),
			    player_num, name);
	else if (strcmp(old_name, name))
		log_message(MSG_INFO, _("%s is now %s.\n"), old_name,
			    name);
	if (old_name != NULL)
		g_free(old_name);
	callbacks.player_name(player_num, name);
}

void player_change_style(gint player_num, const gchar * style)
{
	player_set_style(player_num, style);
	callbacks.player_style(player_num, style);
}

void player_has_quit(gint player_num)
{
	Spectator *spectator;

	if (player_num < 0)
		return;

	/* usually callbacks are called after the event has been handled.
	 * Here it is called before, so the frontend can access the
	 * information about the quitting player/spectator */

	if (player_num >= num_total_players) {
		/* a spectator has quit */
		callbacks.spectator_quit(player_num);
		spectator = spectator_get(player_num);
		g_free(spectator->name);
		g_free(spectator->style);
		spectators = g_list_remove(spectators, spectator);
		return;
	}
	callbacks.player_quit(player_num);
	log_message(MSG_INFO, _("%s has quit.\n"), player_name(player_num,
							       TRUE));
}

void player_largest_army(gint player_num)
{
	gint idx;
	if (player_num < 0)
		log_message(MSG_LARGESTARMY,
			    _("There is no largest army.\n"));
	else
		log_message(MSG_LARGESTARMY,
			    _("%s has the largest army.\n"),
			    player_name(player_num, TRUE));
	for (idx = 0; idx < num_total_players; idx++) {
		Player *player = player_get(idx);
		if (player->statistics[STAT_LARGEST_ARMY] != 0
		    && idx != player_num)
			player_modify_statistic(idx, STAT_LARGEST_ARMY,
						-1);
		if (player->statistics[STAT_LARGEST_ARMY] == 0
		    && idx == player_num)
			player_modify_statistic(idx, STAT_LARGEST_ARMY, 1);
	}
}


void player_longest_road(gint player_num)
{
	gint idx;

	if (player_num < 0)
		log_message(MSG_LONGESTROAD,
			    _("There is no longest road.\n"));
	else
		log_message(MSG_LONGESTROAD,
			    _("%s has the longest road.\n"),
			    player_name(player_num, TRUE));

	for (idx = 0; idx < num_total_players; idx++) {
		Player *player = player_get(idx);

		if (player->statistics[STAT_LONGEST_ROAD] != 0
		    && idx != player_num)
			player_modify_statistic(idx, STAT_LONGEST_ROAD,
						-1);
		if (player->statistics[STAT_LONGEST_ROAD] == 0
		    && idx == player_num)
			player_modify_statistic(idx, STAT_LONGEST_ROAD, 1);
	}
}

void player_set_current(gint player_num)
{
	turn_player = player_num;
	if (player_num != my_player_num()) {
		gchar *buffer;
		buffer = g_strdup_printf(_("Waiting for %s."),
					 player_name(player_num, FALSE));
		callbacks.instructions(buffer);
		g_free(buffer);
		return;
	}
	build_new_turn();
}

void player_set_total_num(gint num)
{
	num_total_players = num;
}

void player_stole_from(gint player_num, gint victim_num, Resource resource)
{
	player_modify_statistic(player_num, STAT_RESOURCES, 1);
	player_modify_statistic(victim_num, STAT_RESOURCES, -1);

	if (resource == NO_RESOURCE) {
		/* CHECK THIS: Since anonymous players (NULL) no longer exist,
		 *  player_name doesn't use its static buffer anymore, and
		 * two calls can be safely combined. If not: ai/player.c should also be fixed */
		/* FIXME: in the client, anonymous players can exist.
		 * I prefer changing that instead of this function */
		log_message(MSG_STEAL,
			    /* We are not in on the action
			       someone stole a resource from someone else */
			    _("%s stole a resource from %s.\n"),
			    player_name(player_num, TRUE),
			    player_name(victim_num, FALSE));
	} else {
		gchar *buf;

		buf = resource_cards(1, resource);
		if (player_num == my_player_num()) {
			/* We stole a card :-) */
			log_message(MSG_STEAL,
				    /* $1=resource, $2=player name */
				    _("You stole %s from %s.\n"), buf,
				    player_name(victim_num, FALSE));
			resource_modify(resource, 1);
		} else {
			/* Someone stole our card :-( */
			log_message(MSG_STEAL,
				    /* $1=player name, $2=resource */
				    _("%s stole %s from you.\n"),
				    player_name(player_num, TRUE), buf);
			resource_modify(resource, -1);
		}
		g_free(buf);
	}
	callbacks.player_robbed(player_num, victim_num, resource);
}

void player_domestic_trade(gint player_num, gint partner_num,
			   gint * supply, gint * receive)
{
	gchar *supply_desc;
	gchar *receive_desc;
	gint diff;
	gint idx;

	diff = resource_count(receive) - resource_count(supply);
	player_modify_statistic(player_num, STAT_RESOURCES, -diff);
	player_modify_statistic(partner_num, STAT_RESOURCES, diff);
	if (player_num == my_player_num()) {
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			resource_modify(idx, supply[idx]);
			resource_modify(idx, -receive[idx]);
		}
	} else if (partner_num == my_player_num()) {
		for (idx = 0; idx < NO_RESOURCE; idx++) {
			resource_modify(idx, -supply[idx]);
			resource_modify(idx, receive[idx]);
		}
	}

	if (!resource_count(supply)) {
		if (!resource_count(receive)) {
			log_message(MSG_TRADE, _("%s gave %s nothing!?\n"),
				    player_name(player_num, TRUE),
				    player_name(partner_num, FALSE));
		} else {
			receive_desc = resource_format_num(receive);
			log_message(MSG_TRADE,
				    /* $1=giving player, $2=receiving player, $3=resources */
				    _("%s gave %s %s for free.\n"),
				    player_name(player_num, TRUE),
				    player_name(partner_num, FALSE),
				    receive_desc);
			g_free(receive_desc);
		}
	} else if (!resource_count(receive)) {
		supply_desc = resource_format_num(supply);
		log_message(MSG_TRADE,
			    /* $1=giving player, $2=receiving player, $3=resources */
			    _("%s gave %s %s for free.\n"),
			    player_name(partner_num, TRUE),
			    player_name(player_num, FALSE), supply_desc);
		g_free(supply_desc);
	} else {
		supply_desc = resource_format_num(supply);
		receive_desc = resource_format_num(receive);
		log_message(MSG_TRADE,
			    _("%s gave %s %s in exchange for %s.\n"),
			    player_name(player_num, TRUE),
			    player_name(partner_num, FALSE), receive_desc,
			    supply_desc);
		g_free(supply_desc);
		g_free(receive_desc);
	}
}

void player_maritime_trade(gint player_num,
			   gint ratio, Resource supply, Resource receive)
{
	gchar *buf_give;
	gchar *buf_receive;
	gint resources[NO_RESOURCE];
	gint idx;

	for (idx = 0; idx < NO_RESOURCE; ++idx)
		resources[idx] = 0;
	resources[supply] = ratio;
	resources[receive] = -1;
	modify_bank(resources);

	player_modify_statistic(player_num, STAT_RESOURCES, 1 - ratio);
	if (player_num == my_player_num()) {
		resource_modify(supply, -ratio);
		resource_modify(receive, 1);
	}

	buf_give = resource_cards(ratio, supply);
	buf_receive = resource_cards(1, receive);
	log_message(MSG_TRADE, _("%s exchanged %s for %s.\n"),
		    player_name(player_num, TRUE), buf_give, buf_receive);
	g_free(buf_give);
	g_free(buf_receive);
}

void player_build_add(gint player_num,
		      BuildType type, gint x, gint y, gint pos,
		      gboolean log_changes)
{
	Edge *edge;
	Node *node;

	switch (type) {
	case BUILD_ROAD:
		edge = map_edge(callbacks.get_map(), x, y, pos);
		edge->owner = player_num;
		edge->type = BUILD_ROAD;
		callbacks.draw_edge(edge);
		if (log_changes) {
			log_message(MSG_BUILD, _("%s built a road.\n"),
				    player_name(player_num, TRUE));
		}
		if (player_num == my_player_num())
			stock_use_road();
		break;
	case BUILD_SHIP:
		edge = map_edge(callbacks.get_map(), x, y, pos);
		edge->owner = player_num;
		edge->type = BUILD_SHIP;
		callbacks.draw_edge(edge);
		if (log_changes) {
			log_message(MSG_BUILD, _("%s built a ship.\n"),
				    player_name(player_num, TRUE));
		}
		if (player_num == my_player_num())
			stock_use_ship();
		break;
	case BUILD_SETTLEMENT:
		node = map_node(callbacks.get_map(), x, y, pos);
		node->type = BUILD_SETTLEMENT;
		node->owner = player_num;
		callbacks.draw_node(node);
		if (log_changes) {
			log_message(MSG_BUILD,
				    _("%s built a settlement.\n"),
				    player_name(player_num, TRUE));
		}
		player_modify_statistic(player_num, STAT_SETTLEMENTS, 1);
		if (player_num == my_player_num())
			stock_use_settlement();
		break;
	case BUILD_CITY:
		node = map_node(callbacks.get_map(), x, y, pos);
		if (node->type == BUILD_SETTLEMENT) {
			player_modify_statistic(player_num,
						STAT_SETTLEMENTS, -1);
			if (player_num == my_player_num())
				stock_replace_settlement();
		}
		node->type = BUILD_CITY;
		node->owner = player_num;
		callbacks.draw_node(node);
		if (log_changes) {
			log_message(MSG_BUILD, _("%s built a city.\n"),
				    player_name(player_num, TRUE));
		}
		player_modify_statistic(player_num, STAT_CITIES, 1);
		if (player_num == my_player_num())
			stock_use_city();
		break;
	case BUILD_CITY_WALL:
		node = map_node(callbacks.get_map(), x, y, pos);
		node->city_wall = TRUE;
		node->owner = player_num;
		callbacks.draw_node(node);
		if (log_changes) {
			log_message(MSG_BUILD,
				    _("%s built a city wall.\n"),
				    player_name(player_num, TRUE));
		}
		player_modify_statistic(player_num, STAT_CITY_WALLS, 1);
		if (player_num == my_player_num())
			stock_use_city_wall();
		break;

	case BUILD_NONE:
		log_message(MSG_ERROR,
			    /* Error message */
			    _(""
			      "player_build_add called with BUILD_NONE for user %s\n"),
			    player_name(player_num, TRUE));
		break;
	case BUILD_BRIDGE:
		edge = map_edge(callbacks.get_map(), x, y, pos);
		edge->owner = player_num;
		edge->type = BUILD_BRIDGE;
		callbacks.draw_edge(edge);
		if (log_changes) {
			log_message(MSG_BUILD, _("%s built a bridge.\n"),
				    player_name(player_num, TRUE));
		}
		if (player_num == my_player_num())
			stock_use_bridge();
		break;
	case BUILD_MOVE_SHIP:
		/* This clause here to remove a compiler warning.
		   It should not be possible to reach this code. */
		g_error("Bug: unreachable code reached");
		break;
	}
}

void player_build_remove(gint player_num,
			 BuildType type, gint x, gint y, gint pos)
{
	Edge *edge;
	Node *node;

	switch (type) {
	case BUILD_ROAD:
		edge = map_edge(callbacks.get_map(), x, y, pos);
		edge->owner = -1;
		callbacks.draw_edge(edge);
		edge->type = BUILD_NONE;
		log_message(MSG_BUILD, _("%s removed a road.\n"),
			    player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_replace_road();
		break;
	case BUILD_SHIP:
		edge = map_edge(callbacks.get_map(), x, y, pos);
		edge->owner = -1;
		callbacks.draw_edge(edge);
		edge->type = BUILD_NONE;
		log_message(MSG_BUILD, _("%s removed a ship.\n"),
			    player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_replace_ship();
		break;
	case BUILD_SETTLEMENT:
		node = map_node(callbacks.get_map(), x, y, pos);
		node->type = BUILD_NONE;
		node->owner = -1;
		callbacks.draw_node(node);
		log_message(MSG_BUILD, _("%s removed a settlement.\n"),
			    player_name(player_num, TRUE));
		player_modify_statistic(player_num, STAT_SETTLEMENTS, -1);
		if (player_num == my_player_num())
			stock_replace_settlement();
		break;
	case BUILD_CITY:
		node = map_node(callbacks.get_map(), x, y, pos);
		node->type = BUILD_SETTLEMENT;
		node->owner = player_num;
		callbacks.draw_node(node);
		log_message(MSG_BUILD, _("%s removed a city.\n"),
			    player_name(player_num, TRUE));
		player_modify_statistic(player_num, STAT_SETTLEMENTS, 1);
		player_modify_statistic(player_num, STAT_CITIES, -1);
		if (player_num == my_player_num()) {
			stock_use_settlement();
			stock_replace_city();
		}
		break;
	case BUILD_CITY_WALL:
		node = map_node(callbacks.get_map(), x, y, pos);
		node->city_wall = FALSE;
		node->owner = player_num;
		callbacks.draw_node(node);
		log_message(MSG_BUILD, _("%s removed a city wall.\n"),
			    player_name(player_num, TRUE));
		player_modify_statistic(player_num, STAT_CITY_WALLS, -1);
		if (player_num == my_player_num())
			stock_replace_city_wall();
		break;

	case BUILD_NONE:
		log_message(MSG_ERROR,
			    /* Error message */
			    _(""
			      "player_build_remove called with BUILD_NONE for user %s\n"),
			    player_name(player_num, TRUE));
		break;
	case BUILD_BRIDGE:
		edge = map_edge(callbacks.get_map(), x, y, pos);
		edge->owner = -1;
		callbacks.draw_edge(edge);
		edge->type = BUILD_NONE;
		log_message(MSG_BUILD, _("%s removed a bridge.\n"),
			    player_name(player_num, TRUE));
		if (player_num == my_player_num())
			stock_replace_bridge();
		break;
	case BUILD_MOVE_SHIP:
		/* This clause here to remove a compiler warning.
		   It should not be possible to reach this case. */
		g_error("Bug: unreachable code reached");
		break;
	}
}

void player_build_move(gint player_num, gint sx, gint sy, gint spos,
		       gint dx, gint dy, gint dpos, gint isundo)
{
	Edge *from = map_edge(callbacks.get_map(), sx, sy, spos),
	    *to = map_edge(callbacks.get_map(), dx, dy, dpos);
	if (isundo) {
		Edge *tmp = from;
		from = to;
		to = tmp;
	}
	from->owner = -1;
	callbacks.draw_edge(from);
	from->type = BUILD_NONE;
	to->owner = player_num;
	to->type = BUILD_SHIP;
	callbacks.draw_edge(to);
	if (isundo)
		log_message(MSG_BUILD,
			    _("%s has canceled a ship's movement.\n"),
			    player_name(player_num, TRUE));
	else
		log_message(MSG_BUILD, _("%s moved a ship.\n"),
			    player_name(player_num, TRUE));
}

void player_resource_action(gint player_num, const gchar * action,
			    const gint * resource_list, gint mult)
{
	resource_log_list(player_num, action, resource_list);
	resource_apply_list(player_num, resource_list, mult);
}

/* get a new point */
void player_get_point(gint player_num, gint id, const gchar * str,
		      gint num)
{
	Player *player = player_get(player_num);
	/* create the point */
	Points *point = points_new(id, str, num);
	player_modify_points(player_num, point, TRUE);
	/* tell the user that someone got something */
	log_message(MSG_INFO, _("%s received %s.\n"), player->name,
		    _(str));
}

/* lose a point: no one gets it */
void player_lose_point(gint player_num, gint id)
{
	Player *player = player_get(player_num);
	Points *point;
	GList *list;
	/* look up the point in the list */
	for (list = player->points; list != NULL; list = g_list_next(list)) {
		point = list->data;
		if (point->id == id)
			break;
	}
	/* communication error: the point doesn't exist */
	if (list == NULL) {
		log_message(MSG_ERROR,
			    _("server asks to lose invalid point.\n"));
		return;
	}
	player->points = g_list_remove(player->points, point);
	player_modify_points(player_num, point, FALSE);
	/* tell the user the point is lost */
	log_message(MSG_INFO, _("%s lost %s.\n"), player->name,
		    _(point->name));
	/* free the memory */
	points_free(point);
	g_free(point);
}

/* take a point from an other player */
void player_take_point(gint player_num, gint id, gint old_owner)
{
	Player *player = player_get(player_num);
	Player *victim = player_get(old_owner);
	Points *point;
	GList *list;
	/* look up the point in the list */
	for (list = victim->points; list != NULL; list = g_list_next(list)) {
		point = list->data;
		if (point->id == id)
			break;
	}
	/* communication error: the point doesn't exist */
	if (list == NULL) {
		log_message(MSG_ERROR,
			    _("server asks to move invalid point.\n"));
		return;
	}
	/* move the point in memory */
	victim->points = g_list_remove(victim->points, point);
	player->points = g_list_append(player->points, point);
	/* tell the user someone (1) lost something (2) to someone else (3) */
	log_message(MSG_INFO, _("%s lost %s to %s.\n"), victim->name,
		    _(point->name), player->name);
}

void player_set_style(gint player_num, const gchar * style)
{
	if (player_num >= num_total_players) {
		Spectator *spectator = spectator_get(player_num);
		if (spectator->style)
			g_free(spectator->style);
		spectator->style = g_strdup(style);
	} else if (player_num >= 0) {
		Player *player = player_get(player_num);
		if (player->style)
			g_free(player->style);
		player->style = g_strdup(style);
	}
}

const gchar *player_get_style(gint player_num)
{
	const gchar *style = NULL;
	if (player_num >= num_total_players) {
		Spectator *spectator = spectator_get(player_num);
		style = spectator->style;
	} else if (player_num >= 0) {
		Player *player = player_get(player_num);
		style = player->style;
	}
	return style == NULL ? default_player_style : style;
}

gint current_player(void)
{
	return turn_player;
}

const gchar *my_player_name(void)
{
	return player_name(my_player_num(), TRUE);
}

gboolean my_player_spectator(void)
{
	return player_is_spectator(my_player_num());
}

const gchar *my_player_style(void)
{
	return player_get_style(my_player_num());
}

gint find_player_by_name(const gchar * name)
{
	gint i;
	GList *list;

	for (i = 0; i < num_total_players; i++) {
		Player *player = player_get(i);
		if (player->name && !strcmp(player->name, name))
			return i;
	}

	for (list = spectators; list != NULL; list = g_list_next(list)) {
		Spectator *spectator = list->data;
		if (!strcmp(spectator->name, name))
			return spectator->num;
	}
	return -1;
}
