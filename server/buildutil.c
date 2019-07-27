/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
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
#include "buildrec.h"
#include "cost.h"
#include "server.h"

void check_longest_road(Game * game)
{
	Map *map = game->params->map;
	guint road_length[MAX_PLAYERS];
	gint num_have_longest;
	guint longest_length;
	gboolean tie;
	guint i;

	map_longest_road(map, road_length, game->params->num_players);

	num_have_longest = -1;
	longest_length = 0;
	tie = FALSE;
	for (i = 0; i < game->params->num_players; i++) {
		if (road_length[i] >= 5) {
			if (road_length[i] > longest_length) {
				num_have_longest = (gint) i;
				longest_length = road_length[i];
				tie = FALSE;
			} else if (road_length[i] == longest_length) {
				tie = TRUE;
				if (game->longest_road != NULL
				    && (gint) i ==
				    game->longest_road->num) {
					/* Current owner in the tie */
					num_have_longest = (gint) i;
				}
			}
		}
	}

	if (num_have_longest == -1) {
		/* All roads are too short */
		if (game->longest_road != NULL) {
			/* Revoke the longest road */
			player_broadcast(player_none(game), PB_ALL,
					 FIRST_VERSION, LATEST_VERSION,
					 "longest-road\n");
			game->longest_road = NULL;
		}
	} else if (!tie) {
		/* One player has the longest road */
		if (game->longest_road == NULL
		    || game->longest_road->num != num_have_longest) {
			/* Reassign the longest road */
			game->longest_road =
			    player_by_num(game, num_have_longest);
			player_broadcast(game->longest_road, PB_ALL,
					 FIRST_VERSION, LATEST_VERSION,
					 "longest-road\n");
		}
	} else {
		/* Several players have the longest road */
		if (game->longest_road == NULL
		    || game->longest_road->num != num_have_longest) {
			/* If the current owner does not have the longest
			   road, nobody will have the extra points. */
			player_broadcast(player_none(game), PB_ALL,
					 FIRST_VERSION, LATEST_VERSION,
					 "longest-road\n");
			game->longest_road = NULL;
		}
	}
}

/* build something on a node */
void node_add(Player * player,
	      BuildType type, int x, int y, int pos, gboolean paid_for,
	      Points * points)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Node *node = map_node(map, x, y, pos);
	BuildRec *rec;

	/* administrate the built number of structures */
	if (type == BUILD_SETTLEMENT)
		player->num_settlements++;
	else if (type == BUILD_CITY) {
		if (node->type == BUILD_SETTLEMENT)
			player->num_settlements--;
		player->num_cities++;
	} else if (type == BUILD_CITY_WALL) {
		player->num_city_walls++;
	}

	/* fill the backup struct */
	rec = buildrec_new(type, x, y, pos);
	rec->prev_status = node->type;
	rec->longest_road =
	    game->longest_road ? game->longest_road->num : -1;
	rec->special_points_id = -1;

	/* compute the cost */
	if (paid_for) {
		if (type == BUILD_CITY)
			if (node->type == BUILD_SETTLEMENT)
				rec->cost = cost_upgrade_settlement();
			else
				rec->cost = cost_city();
		else if (type == BUILD_SETTLEMENT)
			rec->cost = cost_settlement();
		else if (type == BUILD_CITY_WALL)
			rec->cost = cost_city_wall();

		resource_spend(player, rec->cost);
	} else
		rec->cost = NULL;

	if (points != NULL) {
		rec->special_points_id = points->id;
	}

	/* put the struct in the undo list */
	player->build_list = g_list_append(player->build_list, rec);

	/* update the node information */
	node->owner = player->num;
	if (type == BUILD_CITY_WALL) {
		node->city_wall = TRUE;
		/* Older clients see an extension message */
		player_broadcast_extension(player, PB_RESPOND,
					   FIRST_VERSION, V0_10,
					   "built city wall\n");
		player_broadcast(player, PB_RESPOND, V0_11, LATEST_VERSION,
				 "built %B %d %d %d\n", type, x, y, pos);
	} else {
		node->type = type;
		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION, "built %B %d %d %d\n",
				 type, x, y, pos);
	}
	if (points != NULL) {
		player->special_points =
		    g_list_append(player->special_points, points);
		player_broadcast(player, PB_ALL, FIRST_VERSION,
				 LATEST_VERSION, "get-point %d %d %s\n",
				 points->id, points->points, points->name);
	}

	/* see if the longest road was cut */
	check_longest_road(game);
}

/* build something on an edge */
void edge_add(Player * player, BuildType type, int x, int y, int pos,
	      gboolean paid_for)
{
	Game *game = player->game;
	Map *map = game->params->map;
	Edge *edge = map_edge(map, x, y, pos);
	BuildRec *rec;

	/* fill the undo struct */
	rec = buildrec_new(type, x, y, pos);
	rec->longest_road =
	    game->longest_road ? game->longest_road->num : -1;

	/* take the money if needed */
	if (paid_for) {
		switch (type) {
		case BUILD_ROAD:
			rec->cost = cost_road();
			break;
		case BUILD_SHIP:
			rec->cost = cost_ship();
			break;
		case BUILD_BRIDGE:
			rec->cost = cost_bridge();
			break;
		case BUILD_MOVE_SHIP:
		case BUILD_SETTLEMENT:
		case BUILD_CITY:
		case BUILD_CITY_WALL:
		case BUILD_NONE:
			log_message(MSG_ERROR,
				    "In buildutils.c::edge_add() - Invalid build type.\n");
			break;
		}
		resource_spend(player, rec->cost);
	} else
		rec->cost = NULL;

	/* put the struct in the undo list */
	player->build_list = g_list_append(player->build_list, rec);

	/* update the pieces */
	switch (type) {
	case BUILD_ROAD:
		player->num_roads++;
		break;
	case BUILD_BRIDGE:
		player->num_bridges++;
		break;
	case BUILD_SHIP:
		player->num_ships++;
		break;
	case BUILD_MOVE_SHIP:
	case BUILD_SETTLEMENT:
	case BUILD_CITY:
	case BUILD_CITY_WALL:
	case BUILD_NONE:
		log_message(MSG_ERROR,
			    "In buildutils.c::edge_add() - Invalid build type.\n");
		break;
	}

	/* update the board */
	edge->owner = player->num;
	edge->type = type;
	player_broadcast(player, PB_RESPOND, FIRST_VERSION, LATEST_VERSION,
			 "built %B %d %d %d\n", type, x, y, pos);

	/* perhaps the longest road changed owner */
	check_longest_road(game);
}

static gint find_points_by_id(gconstpointer a, gconstpointer b)
{
	const Points *points = a;
	gint id = GPOINTER_TO_INT(b);

	return points->id == id ? 0 : points->id < id ? -1 : +1;
}

/* undo a build action */
gboolean perform_undo(Player * player)
{
	Game *game = player->game;
	Map *map = game->params->map;
	GList *list;
	BuildRec *rec;
	Hex *hex;
	int longest_road;

	/* If the player hasn't built anything, the undo fails */
	if (player->build_list == NULL)
		return FALSE;

	/* Fill some convenience variables */
	list = g_list_last(player->build_list);
	rec = list->data;
	hex = map_hex(map, rec->x, rec->y);

	/* Remove the entry from the list (doesn't remove the data itself) */
	player->build_list = g_list_remove_link(player->build_list, list);
	g_list_free_1(list);

	/* Do structure-specific things */
	switch (rec->type) {
	case BUILD_NONE:
		g_error("BUILD_NONE in perform_undo()");
		break;
	case BUILD_ROAD:
		player->num_roads--;

		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION, "remove %B %d %d %d\n",
				 BUILD_ROAD, rec->x, rec->y, rec->pos);
		hex->edges[rec->pos]->owner = -1;
		hex->edges[rec->pos]->type = BUILD_NONE;
		break;
	case BUILD_BRIDGE:
		player->num_bridges--;

		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION, "remove %B %d %d %d\n",
				 BUILD_BRIDGE, rec->x, rec->y, rec->pos);
		hex->edges[rec->pos]->owner = -1;
		hex->edges[rec->pos]->type = BUILD_NONE;
		break;
	case BUILD_SHIP:
		player->num_ships--;

		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION, "remove %B %d %d %d\n",
				 BUILD_SHIP, rec->x, rec->y, rec->pos);
		hex->edges[rec->pos]->owner = -1;
		hex->edges[rec->pos]->type = BUILD_NONE;
		break;
	case BUILD_CITY:
		player->num_cities--;
		player->num_settlements++;

		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION, "remove %B %d %d %d\n",
				 BUILD_CITY, rec->x, rec->y, rec->pos);
		hex->nodes[rec->pos]->type = BUILD_SETTLEMENT;
		if (rec->prev_status == BUILD_SETTLEMENT)
			break;
		/* Remove the settlement too */
		/* Fall through */
	case BUILD_SETTLEMENT:
		player->num_settlements--;

		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION, "remove %B %d %d %d\n",
				 BUILD_SETTLEMENT, rec->x, rec->y,
				 rec->pos);
		hex->nodes[rec->pos]->type = BUILD_NONE;
		hex->nodes[rec->pos]->owner = -1;
		break;
	case BUILD_CITY_WALL:
		player->num_city_walls--;
		/* Older clients see an extension message */
		player_broadcast_extension(player, PB_RESPOND,
					   FIRST_VERSION, V0_10,
					   "remove city wall\n");
		player_broadcast(player, PB_RESPOND, V0_11, LATEST_VERSION,
				 "remove %B %d %d %d\n", BUILD_CITY_WALL,
				 rec->x, rec->y, rec->pos);
		hex->nodes[rec->pos]->city_wall = FALSE;
		break;
	case BUILD_MOVE_SHIP:
		hex->edges[rec->pos]->owner = -1;
		hex->edges[rec->pos]->type = BUILD_NONE;
		hex = map_hex(map, rec->prev_x, rec->prev_y);
		hex->edges[rec->prev_pos]->owner = player->num;
		hex->edges[rec->prev_pos]->type = BUILD_SHIP;
		map->has_moved_ship = FALSE;
		player_broadcast(player, PB_RESPOND, FIRST_VERSION,
				 LATEST_VERSION,
				 "move-back %d %d %d %d %d %d\n",
				 rec->prev_x, rec->prev_y, rec->prev_pos,
				 rec->x, rec->y, rec->pos);
		break;
	}

	/* Give back the money, if any */
	if (rec->cost != NULL)
		resource_refund(player, rec->cost);

	/* If the longest road changed, change it back */
	longest_road = game->longest_road ? game->longest_road->num : -1;
	if (longest_road != rec->longest_road) {
		if (rec->longest_road >= 0) {
			game->longest_road =
			    player_by_num(game, rec->longest_road);
			player_broadcast(game->longest_road, PB_ALL,
					 FIRST_VERSION, LATEST_VERSION,
					 "longest-road\n");
		} else {
			game->longest_road = NULL;
			player_broadcast(player_none(game), PB_ALL,
					 FIRST_VERSION, LATEST_VERSION,
					 "longest-road\n");
		}
	}

	if (rec->special_points_id != -1) {
		GList *points;
		if (game->params->island_discovery_bonus != NULL) {
			if (!map_is_island_discovered
			    (map, map_node(map, rec->x, rec->y, rec->pos),
			     player->num)) {
				player->islands_discovered--;
			}
		}

		player_broadcast(player, PB_ALL, FIRST_VERSION,
				 LATEST_VERSION, "lose-point %d\n",
				 rec->special_points_id);
		points =
		    g_list_find_custom(player->special_points,
				       GINT_TO_POINTER
				       (rec->special_points_id),
				       find_points_by_id);
		if (points != NULL) {
			points_free(points->data);
			player->special_points =
			    g_list_remove(player->special_points,
					  points->data);
		}
	}
	/* free the memory */
	g_free(rec);

	return TRUE;
}
