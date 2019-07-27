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
#include <math.h>
#include <ctype.h>
#include <string.h>

#include <glib.h>

#include "game.h"
#include "map.h"

/* Local function prototypes */
gboolean node_has_edge_owned_by(const Node * node, gint owner,
				BuildType type);
gboolean is_road_valid(const Edge * edge, gint owner);
gboolean is_ship_valid(const Edge * edge, gint owner);
gboolean is_bridge_valid(const Edge * edge, gint owner);


/* This file is broken into a number of sections:
 *
 * Simple Checks:
 *
 * Most map queries require a number of checks to be performed.  The
 * results of the checks are combined to provide a more complex
 * answer.
 *
 * Cursor Checks:
 *
 * When interacting with the user, the GUI needs to establish whether
 * or not to draw a cursor over a specific edge, node or hex.  Cursor
 * check functions are designed to be passed as the check_func
 * parameter to the gui_map_set_cursor() function.
 *
 * Queries:
 *
 * Provides an answer to a specific question about the entire map.
 */

/* Return whether or not an edge is adjacent to a node
 */
gboolean is_edge_adjacent_to_node(const Edge * edge, const Node * node)
{
	g_return_val_if_fail(edge != NULL, FALSE);
	g_return_val_if_fail(node != NULL, FALSE);
	return edge->nodes[0] == node || edge->nodes[1] == node;
}

/* Return whether or not an edge is on land or coast
 */
gboolean is_edge_on_land(const Edge * edge)
{
	guint idx;

	g_return_val_if_fail(edge != NULL, FALSE);

	for (idx = 0; idx < G_N_ELEMENTS(edge->hexes); idx++) {
		Hex *hex = edge->hexes[idx];
		if (hex != NULL && hex->terrain != SEA_TERRAIN)
			return TRUE;
	}

	return FALSE;
}

/* Return whether or not an edge is on sea or coast (used only for ships)
 */
gboolean is_edge_on_sea(const Edge * edge)
{
	guint idx;

	g_return_val_if_fail(edge != NULL, FALSE);

	/* If the pirate is currently next to the edge, then specific sea
	 * actions should not be possible (building ships is the only
	 * specific sea action). */
	for (idx = 0; idx < G_N_ELEMENTS(edge->hexes); idx++) {
		Hex *hex = edge->hexes[idx];
		if (hex && edge->map->pirate_hex == hex)
			return FALSE;
	}
	/* The pirate is not next to the edge, return true if there is sea */
	for (idx = 0; idx < G_N_ELEMENTS(edge->hexes); idx++) {
		Hex *hex = edge->hexes[idx];
		if (hex != NULL && hex->terrain == SEA_TERRAIN)
			return TRUE;
	}

	/* There is no sea */
	return FALSE;
}

/* Return whether or not a node is on land
 */
gboolean is_node_on_land(const Node * node)
{
	guint idx;

	g_return_val_if_fail(node != NULL, FALSE);

	for (idx = 0; idx < G_N_ELEMENTS(node->hexes); idx++) {
		Hex *hex = node->hexes[idx];
		if (hex != NULL && hex->terrain != SEA_TERRAIN)
			return TRUE;
	}

	return FALSE;
}

/* Check if a node has a adjacent road/ship/bridge owned by the
 * specified player
 */
gboolean node_has_edge_owned_by(const Node * node, gint owner,
				BuildType type)
{
	guint idx;

	g_return_val_if_fail(node != NULL, FALSE);

	for (idx = 0; idx < G_N_ELEMENTS(node->edges); idx++)
		if (node->edges[idx] != NULL
		    && node->edges[idx]->owner == owner
		    && node->edges[idx]->type == type)
			return TRUE;

	return FALSE;
}

/* Check if a node has a adjacent road owned by the specified player
 */
gboolean node_has_road_owned_by(const Node * node, gint owner)
{
	g_return_val_if_fail(node != NULL, FALSE);
	return node_has_edge_owned_by(node, owner, BUILD_ROAD);
}

/* Check if a node has a adjacent ship owned by the specified player
 */
gboolean node_has_ship_owned_by(const Node * node, gint owner)
{
	g_return_val_if_fail(node != NULL, FALSE);
	return node_has_edge_owned_by(node, owner, BUILD_SHIP);
}

/* Check if a node has a adjacent bridge owned by the specified player
 */
gboolean node_has_bridge_owned_by(const Node * node, gint owner)
{
	g_return_val_if_fail(node != NULL, FALSE);
	return node_has_edge_owned_by(node, owner, BUILD_BRIDGE);
}

/* Check node proximity to other buildings.  A building can be
 * constructed on a node if none of the adjacent nodes have buildings
 * on them.  There is an exception when bridges are being used - two
 * buildings may be on adjacent nodes if separated by water.
 */
gboolean is_node_spacing_ok(const Node * node)
{
	guint idx;

	g_return_val_if_fail(node != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(node->edges); idx++) {
		Edge *edge = node->edges[idx];
		guint idx2;

		if (edge == NULL)
			continue;
		if (node->map->have_bridges && !is_edge_on_land(edge))
			continue;
		else
			for (idx2 = 0; idx2 < G_N_ELEMENTS(edge->nodes);
			     ++idx2) {
				Node *scan = edge->nodes[idx2];
				if (scan == node)
					continue;
				if (scan->type != BUILD_NONE)
					return FALSE;
			}
	}

	return TRUE;
}

/* Check if the specified node is next to the hex with the robber
 */
gboolean is_node_next_to_robber(const Node * node)
{
	guint idx;

	g_return_val_if_fail(node != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(node->hexes); idx++)
		if (node->hexes[idx]->robber)
			return TRUE;

	return FALSE;
}

/* Check if a road has been positioned properly
 */
gboolean is_road_valid(const Edge * edge, gint owner)
{
	guint idx;

	g_return_val_if_fail(edge != NULL, FALSE);

	/* Can only build road if edge is adjacent to a land hex
	 */
	if (!is_edge_on_land(edge))
		return FALSE;

	/* Road can be build adjacent to building we own, or a road we
	 * own that is not separated by a building owned by someone else
	 */
	for (idx = 0; idx < G_N_ELEMENTS(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];

		if (node->owner == owner)
			return TRUE;
		if (node->owner >= 0)
			continue;

		if (node_has_road_owned_by(node, owner)
		    || node_has_bridge_owned_by(node, owner))
			return TRUE;
	}

	return FALSE;
}

/* Check if a ship has been positioned properly
 */
gboolean is_ship_valid(const Edge * edge, gint owner)
{
	guint idx;

	g_return_val_if_fail(edge != NULL, FALSE);

	/* Can only build ship if edge is adjacent to a sea hex
	 */
	if (!is_edge_on_sea(edge))
		return FALSE;

	/* Ship can be build adjacent to building we own, or a ship we
	 * own that is not separated by a building owned by someone else
	 */
	for (idx = 0; idx < G_N_ELEMENTS(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];

		if (node->owner == owner)
			return TRUE;
		if (node->owner >= 0)
			continue;

		if (node_has_ship_owned_by(node, owner))
			return TRUE;
	}

	return FALSE;
}

/* Check if a bridge has been positioned properly
 */
gboolean is_bridge_valid(const Edge * edge, gint owner)
{
	guint idx;

	g_return_val_if_fail(edge != NULL, FALSE);

	/* Can only build bridge if edge is not on land
	 */
	if (is_edge_on_land(edge))
		return FALSE;

	/* Bridge can be build adjacent to building we own, or a road we
	 * own that is not separated by a building owned by someone else
	 */
	for (idx = 0; idx < G_N_ELEMENTS(edge->nodes); idx++) {
		Node *node = edge->nodes[idx];

		if (node->owner == owner)
			return TRUE;
		if (node->owner >= 0)
			continue;

		if (node_has_road_owned_by(node, owner)
		    || node_has_bridge_owned_by(node, owner))
			return TRUE;
	}

	return FALSE;
}

/* Returns TRUE if one of the nodes can be used for setup,
 * or if it already has been used.
 * A full check (to see if the owner of the road matches the owner
 * of the settlement) is performed in another function
 */
static gboolean can_adjacent_settlement_be_built(const Edge * edge)
{
	g_return_val_if_fail(edge != NULL, FALSE);
	return can_settlement_be_setup(edge->nodes[0]) ||
	    can_settlement_be_setup(edge->nodes[1]) ||
	    edge->nodes[0]->owner >= 0 || edge->nodes[1]->owner >= 0;
}

/* Edge cursor check function.
 *
 * Determine whether or not a road can be built in this edge by the
 * specified player during the setup phase.  Perform the following checks:
 *
 * 1 - Edge must not currently have a road on it.
 * 2 - Edge must be adjacent to a land hex.
 * 3 - At least one node must available for a settlement
 *
 * The checks are not as strict as for normal play.  This allows the
 * player to try a few different configurations without layout
 * restrictions.  The server will enfore correct placement at the end
 * of the setup phase.
 */
gboolean can_road_be_setup(const Edge * edge)
{
	g_return_val_if_fail(edge != NULL, FALSE);
	return edge->owner < 0 && is_edge_on_land(edge)
	    && can_adjacent_settlement_be_built(edge);
}

/* Edge cursor check function.
 *
 * Determine whether or not a ship can be built in this edge by the
 * specified player during the setup phase.  Perform the following checks:
 *
 * 1 - Edge must not currently have a ship on it.
 * 2 - Edge must be adjacent to a sea hex.
 * 3 - At least one node must available for a settlement
 *
 * The checks are not as strict as for normal play.  This allows the
 * player to try a few different configurations without layout
 * restrictions.  The server will enfore correct placement at the end
 * of the setup phase.
 */
gboolean can_ship_be_setup(const Edge * edge)
{
	g_return_val_if_fail(edge != NULL, FALSE);
	return edge->owner < 0 && is_edge_on_sea(edge)
	    && can_adjacent_settlement_be_built(edge);
}

/* Edge cursor check function.
 *
 * Determine whether or not a bridge can be built in this edge by the
 * specified player during the setup phase.  Perform the following checks:
 *
 * 1 - Edge must not currently have a road on it.
 * 2 - Edge must not be adjacent to a land hex.
 * 3 - At least one node must available for a settlement
 *
 * The checks are not as strict as for normal play.  This allows the
 * player to try a few different configurations without layout
 * restrictions.  The server will enfore correct placement at the end
 * of the setup phase.
 */
gboolean can_bridge_be_setup(const Edge * edge)
{
	g_return_val_if_fail(edge != NULL, FALSE);
	return edge->owner < 0 && !is_edge_on_land(edge)
	    && can_adjacent_settlement_be_built(edge);
}

/* Edge cursor check function.
 *
 * Determine whether or not a road can be built on this edge by the
 * specified player.  Perform the following checks:
 *
 * 1 - Edge must not currently have a road on it.
 * 2 - Edge must be adjacent to a land hex.
 * 3 - Edge must be adjacent to a building that is owned by the
 *     specified player, or must be adjacent to another road segment
 *     owned by the specifed player, but not separated by a building
 *     owned by a different player.
 */
gboolean can_road_be_built(const Edge * edge, gint owner)
{
	g_return_val_if_fail(edge != NULL, FALSE);
	return edge->owner < 0 && is_road_valid(edge, owner);
}

/* Edge cursor check function.
 *
 * Determine whether or not a ship can be built on this edge by the
 * specified player.  Perform the following checks:
 *
 * 1 - Edge must not currently have a road or ship on it.
 * 2 - Edge must be adjacent to a sea hex.
 * 3 - Edge must be adjacent to a building that is owned by the
 *     specified player, or must be adjacent to another ship segment
 *     owned by the specifed player, but not separated by a building
 *     owned by a different player.
 */
gboolean can_ship_be_built(const Edge * edge, gint owner)
{
	g_return_val_if_fail(edge != NULL, FALSE);
	return edge->owner < 0 && is_ship_valid(edge, owner);
}

/* Helper function for can_ship_be_moved */
static gboolean can_ship_be_moved_node(const Node * node, gint owner,
				       const Edge * not)
{
	guint idx;

	g_return_val_if_fail(node != NULL, FALSE);
	g_return_val_if_fail(not != NULL, FALSE);

	/* if a building of a different player is on it, it is
	 * unconnected */
	if (node->type != BUILD_NONE && node->owner != owner)
		return TRUE;
	/* if there is a building of the player, it is connected */
	if (node->type != BUILD_NONE)
		return FALSE;
	/* no buildings: check all edges for ships */
	for (idx = 0; idx < G_N_ELEMENTS(node->edges); idx++) {
		Edge *edge = node->edges[idx];
		/* If this is a ship of the player, it is connected */
		if (edge && edge->owner == owner && edge != not
		    && edge->type == BUILD_SHIP)
			return FALSE;
	}
	return TRUE;
}

/* Edge cursor check function.
 *
 * Determine whether or not a ship can be moved from this edge by the
 * specified player.  Perform the following checks:
 *
 * 1 - Edge must currently have a ship on it.
 * 2 - On one side, there must be neither a building, nor a ship of the
 *     specified player.  A ship is allowed, if there is building of a
 *     different player in between.
 */
gboolean can_ship_be_moved(const Edge * edge, gint owner)
{
	guint idx;
	g_return_val_if_fail(edge != NULL, FALSE);
	/* edge must be a ship of the correct user */
	if (edge->owner != owner || edge->type != BUILD_SHIP)
		return FALSE;
	/* if the pirate is next to the edge, it is not allowed to move */
	if (!is_edge_on_sea(edge))
		return FALSE;
	/* check all nodes, until one is found that is not connected */
	for (idx = 0; idx < G_N_ELEMENTS(edge->nodes); idx++)
		if (can_ship_be_moved_node(edge->nodes[idx], owner, edge))
			return TRUE;
	return FALSE;
}

/* Edge cursor check function.
 *
 * Determine whether or not a bridge can be built on this edge by the
 * specified player.  Perform the following checks:
 *
 * 1 - Edge must not currently have a road on it.
 * 2 - Edge must not be adjacent to a land hex.
 * 3 - Edge must be adjacent to a building that is owned by the
 *     specified player, or must be adjacent to another road/bridge
 *     segment owned by the specifed player, but not separated by a
 *     building owned by a different player.
 */
gboolean can_bridge_be_built(const Edge * edge, gint owner)
{
	g_return_val_if_fail(edge != NULL, FALSE);
	return edge->owner < 0 && is_bridge_valid(edge, owner);
}

/* Node cursor check function.
 *
 * Determine whether or not a settlement can be built on this node by
 * the specified player during the setup phase.  Perform the following
 * checks:
 *
 * 1 - Node must not be in the no-setup list.
 * 2 - Node must be vacant.
 * 3 - Node must be adjacent to a land hex.
 * 4 - Node must not be within one node of another building.
 *
 * The checks are not as strict as for normal play.  This allows the
 * player to try a few different configurations without layout
 * restrictions.  The server will enfore correct placement at the end
 * of the setup phase.
 */
gboolean can_settlement_be_setup(const Node * node)
{
	g_return_val_if_fail(node != NULL, FALSE);
	return !node->no_setup && node->owner < 0 && is_node_on_land(node)
	    && is_node_spacing_ok(node);
}

/* Node cursor check function.
 *
 * Determine whether or not a settlement can be built on this node by the
 * specified player.  Perform the following checks:
 *
 * 1 - Node must be vacant.
 * 2 - Node must be adjacent to a road owned by the specified player
 * 3 - Node must be adjacent to a land hex.
 * 4 - Node must not be within one node of another building.
 */
gboolean can_settlement_be_built(const Node * node, gint owner)
{
	g_return_val_if_fail(node != NULL, FALSE);
	return node->owner < 0 && (node_has_road_owned_by(node, owner)
				   || node_has_ship_owned_by(node, owner)
				   || node_has_bridge_owned_by(node,
							       owner))
	    && is_node_on_land(node)
	    && is_node_spacing_ok(node);
}

/* Node cursor check function.
 *
 * Determine whether or not a settlement can be upgraded to a city by
 * the specified player.
 */
gboolean can_settlement_be_upgraded(const Node * node, gint owner)
{
	g_return_val_if_fail(node != NULL, FALSE);
	return node->owner == owner && node->type == BUILD_SETTLEMENT;
}

/* Node cursor check function.
 *
 * Determine whether or not a city can be built on this node by the
 * specified player.  Perform the following checks:
 *
 * 1 - Node must either be vacant, or have settlement owned by the
 *     specified player on it.
 * 2 - If vacant, node must be adjacent to a road owned by the
 *     specified player
 * 3 - If vacant, node must be adjacent to a land hex.
 * 4 - If vacent, node must not be within one node of another
 *     building.
 */
gboolean can_city_be_built(const Node * node, gint owner)
{
	g_return_val_if_fail(node != NULL, FALSE);
	if (can_settlement_be_upgraded(node, owner))
		return TRUE;

	return node->owner < 0 && (node_has_road_owned_by(node, owner)
				   || node_has_ship_owned_by(node, owner)
				   || node_has_bridge_owned_by(node,
							       owner))
	    && is_node_on_land(node)
	    && is_node_spacing_ok(node);
}

/* Node cursor check function.
 *
 * Determine whether or not a city wall can be built by
 * the specified player.
 */
gboolean can_city_wall_be_built(const Node * node, gint owner)
{
	g_return_val_if_fail(node != NULL, FALSE);
	return node->owner == owner && node->type == BUILD_CITY &&
	    !node->city_wall;
}

/* Hex cursor check function.
 *
 * Determine whether or not the robber be moved to the specified hex.
 *
 * Can only move the robber to hex which produces resources (roll >
 * 0).  We cannot move the robber to the same hex it is already on.
 * Also check if pirate can be moved.
 */
gboolean can_robber_or_pirate_be_moved(const Hex * hex)
{
	g_return_val_if_fail(hex != NULL, FALSE);
	if (hex->terrain == SEA_TERRAIN)
		return (hex->map->has_pirate)
		    && (hex != hex->map->pirate_hex);
	else
		return (hex->roll > 0) && (!hex->robber);
}

/* Iterator function for map_can_place_road() query
 */
static gboolean can_place_road_check(const Hex * hex, void *closure)
{
	guint idx;
	gint *owner = closure;

	g_return_val_if_fail(hex != NULL, FALSE);
	g_return_val_if_fail(owner != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(hex->edges); idx++)
		if (can_road_be_built(hex->edges[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Iterator function for map_can_place_ship() query
 */
static gboolean can_place_ship_check(const Hex * hex, void *closure)
{
	guint idx;
	gint *owner = closure;

	g_return_val_if_fail(hex != NULL, FALSE);
	g_return_val_if_fail(owner != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(hex->edges); idx++)
		if (can_ship_be_built(hex->edges[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Iterator function for map_can_place_bridge() query
 */
static gboolean can_place_bridge_check(const Hex * hex, void *closure)
{
	guint idx;
	gint *owner = closure;

	g_return_val_if_fail(hex != NULL, FALSE);
	g_return_val_if_fail(owner != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(hex->edges); idx++)
		if (can_bridge_be_built(hex->edges[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Query.
 *
 * Determine if there are any edges on the map where a player can
 * place a road.
 */
gboolean map_can_place_road(const Map * map, gint owner)
{
	g_return_val_if_fail(map != NULL, FALSE);
	return map_traverse_const(map, can_place_road_check, &owner);
}

/* Query.
 *
 * Determine if there are any edges on the map where a player can
 * place a ship.
 */
gboolean map_can_place_ship(const Map * map, gint owner)
{
	g_return_val_if_fail(map != NULL, FALSE);
	return map_traverse_const(map, can_place_ship_check, &owner);
}

/* Query.
 *
 * Determine if there are any edges on the map where a player can
 * place a bridge.
 */
gboolean map_can_place_bridge(const Map * map, gint owner)
{
	g_return_val_if_fail(map != NULL, FALSE);
	return map_traverse_const(map, can_place_bridge_check, &owner);
}

/* Iterator function for map_can_place_settlement() query
 */
static gboolean can_place_settlement_check(const Hex * hex, void *closure)
{
	guint idx;
	gint *owner = (gint *) closure;

	g_return_val_if_fail(hex != NULL, FALSE);
	g_return_val_if_fail(owner != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); idx++)
		if (can_settlement_be_built(hex->nodes[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Query.
 *
 * Determine if there are any nodes on the map where a player can
 * place a settlement
 */
gboolean map_can_place_settlement(const Map * map, gint owner)
{
	g_return_val_if_fail(map != NULL, FALSE);
	return map_traverse_const(map, can_place_settlement_check, &owner);
}

/* Iterator function for map_can_upgrade_settlement() query
 */
static gboolean can_upgrade_settlement_check(const Hex * hex,
					     void *closure)
{
	guint idx;
	gint *owner = closure;

	g_return_val_if_fail(hex != NULL, FALSE);
	g_return_val_if_fail(owner != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); idx++)
		if (can_settlement_be_upgraded(hex->nodes[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Query.
 *
 * Determine if there are any nodes on the map where a player can
 * upgrade a settlement
 */
gboolean map_can_upgrade_settlement(const Map * map, gint owner)
{
	g_return_val_if_fail(map != NULL, FALSE);
	return map_traverse_const(map, can_upgrade_settlement_check,
				  &owner);
}

/* Iterator function for map_can_place_city_wall() query
 */
static gboolean can_place_city_wall_check(const Hex * hex, void *closure)
{
	guint idx;
	gint *owner = closure;

	g_return_val_if_fail(hex != NULL, FALSE);
	g_return_val_if_fail(owner != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); idx++)
		if (can_city_wall_be_built(hex->nodes[idx], *owner))
			return TRUE;
	return FALSE;
}

/* Query.
 *
 * Determine if there are any nodes on the map where a player can
 * place a settlement
 */
gboolean map_can_place_city_wall(const Map * map, gint owner)
{
	g_return_val_if_fail(map != NULL, FALSE);
	return map_traverse_const(map, can_place_city_wall_check, &owner);
}

/* Ignoring road connectivity, decide whether or not a settlement/city
 * can be placed at the specified location.
 */
gboolean map_building_spacing_ok(Map * map, gint owner,
				 BuildType type, gint x, gint y, gint pos)
{
	Node *node;
	g_return_val_if_fail(map != NULL, FALSE);
	node = map_node(map, x, y, pos);
	if (node == NULL)
		return FALSE;

	if (node->type == BUILD_NONE)
		/* Node is vacant.  Make sure that all adjacent nodes
		 * are also vacant
		 */
		return is_node_spacing_ok(node);

	/* Node is not vacant, make sure I am the current owner, and I
	 * am trying to upgrade a settlement to a city.
	 */
	return node->owner == owner
	    && node->type == BUILD_SETTLEMENT && type == BUILD_CITY;
}

/* Ignoring building spacing, check if the building connects to a road.
 */
gboolean map_building_connect_ok(const Map * map, gint owner, gint x,
				 gint y, gint pos)
{
	const Node *node;
	g_return_val_if_fail(map != NULL, FALSE);
	node = map_node_const(map, x, y, pos);
	if (node == NULL)
		return FALSE;

	return node_has_road_owned_by(node, owner)
	    || node_has_ship_owned_by(node, owner)
	    || node_has_bridge_owned_by(node, owner);
}

gboolean map_building_vacant(Map * map, BuildType type,
			     gint x, gint y, gint pos)
{
	Node *node;
	g_return_val_if_fail(map != NULL, FALSE);
	node = map_node(map, x, y, pos);
	if (node == NULL)
		return FALSE;

	switch (type) {
	case BUILD_NONE:
	case BUILD_SETTLEMENT:
		return node->type == BUILD_NONE;
	case BUILD_CITY:
		return node->type == BUILD_NONE
		    || node->type == BUILD_SETTLEMENT;
	case BUILD_ROAD:
	case BUILD_SHIP:
	case BUILD_MOVE_SHIP:
	case BUILD_BRIDGE:
		g_error("map_building_vacant() called with edge");
		return FALSE;
	case BUILD_CITY_WALL:
		g_error("map_building_vacant() called with city wall");
		return FALSE;
	}
	return FALSE;
}

gboolean map_road_vacant(Map * map, gint x, gint y, gint pos)
{
	Edge *edge;
	g_return_val_if_fail(map != NULL, FALSE);
	edge = map_edge(map, x, y, pos);

	return edge != NULL && edge->owner < 0;
}

gboolean map_ship_vacant(Map * map, gint x, gint y, gint pos)
{
	Edge *edge;
	g_return_val_if_fail(map != NULL, FALSE);
	edge = map_edge(map, x, y, pos);

	return edge != NULL && edge->owner < 0;
}

gboolean map_bridge_vacant(Map * map, gint x, gint y, gint pos)
{
	Edge *edge;
	g_return_val_if_fail(map != NULL, FALSE);
	edge = map_edge(map, x, y, pos);

	return edge != NULL && edge->owner < 0;
}

/* Ignoring whether or not a road already exists at this point, check
 * that it has the right connectivity.
 */
gboolean map_road_connect_ok(const Map * map, gint owner, gint x, gint y,
			     gint pos)
{
	const Edge *edge;
	g_return_val_if_fail(map != NULL, FALSE);
	edge = map_edge_const(map, x, y, pos);
	if (edge == NULL)
		return FALSE;

	return is_road_valid(edge, owner);
}

/* Ignoring whether or not a ship already exists at this point, check
 * that it has the right connectivity.
 */
gboolean map_ship_connect_ok(const Map * map, gint owner, gint x, gint y,
			     gint pos)
{
	const Edge *edge;
	g_return_val_if_fail(map != NULL, FALSE);
	edge = map_edge_const(map, x, y, pos);
	if (edge == NULL)
		return FALSE;

	return is_ship_valid(edge, owner);
}

/* Ignoring whether or not a bridge already exists at this point, check
 * that it has the right connectivity.
 */
gboolean map_bridge_connect_ok(const Map * map, gint owner, gint x, gint y,
			       gint pos)
{
	const Edge *edge;
	g_return_val_if_fail(map != NULL, FALSE);
	edge = map_edge_const(map, x, y, pos);
	if (edge == NULL)
		return FALSE;

	return is_bridge_valid(edge, owner);
}

static BuildType bridge_as_road(BuildType type)
{
	if (type == BUILD_BRIDGE)
		return BUILD_ROAD;
	else
		return type;
}

/* calculate the longest road */
static gint find_longest_road_recursive(Edge * edge)
{
	gint len = 0;
	guint nodeidx;
	guint edgeidx;

	g_return_val_if_fail(edge != NULL, 0);

	edge->visited = TRUE;
	/* check all nodes to see which one make the longer road. */
	for (nodeidx = 0; nodeidx < G_N_ELEMENTS(edge->nodes); nodeidx++) {
		Node *node = edge->nodes[nodeidx];
		/* don't go back to where we came from */
		if (node->visited)
			continue;
		/* don't continue counting if someone else's building is on
		 * the node. */
		if (node->type != BUILD_NONE && node->owner != edge->owner)
			continue;
		/* don't let other go back here */
		node->visited = TRUE;
		/* try all edges */
		for (edgeidx = 0; edgeidx < G_N_ELEMENTS(node->edges);
		     edgeidx++) {
			Edge *here = node->edges[edgeidx];
			if (here && !here->visited
			    && here->owner == edge->owner) {
				/* don't allow ships to extend roads, except
				 * if there is a construction in between */
				/* bridges are treated as roads */
				if (node->type != BUILD_NONE ||
				    bridge_as_road(here->type) ==
				    bridge_as_road(edge->type)) {
					gint thislen =
					    find_longest_road_recursive
					    (here);
					/* take the maximum of all paths */
					if (thislen > len)
						len = thislen;
				}
			}
		}
		/* Allow other roads to use this node again. */
		node->visited = FALSE;
	}
	edge->visited = FALSE;
	return len + 1;
}

static gboolean find_longest_road(Hex * hex, gpointer closure)
{
	guint idx;
	gint *lengths = closure;
	g_return_val_if_fail(hex != NULL, FALSE);
	g_return_val_if_fail(lengths != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(hex->edges); idx++) {
		Edge *edge = hex->edges[idx];
		gint len;
		/* skip unowned edges, and edges that will be handled by
		 * other hexes */
		if (edge->owner < 0 || edge->x != hex->x
		    || edge->y != hex->y)
			continue;
		len = find_longest_road_recursive(edge);
		if (len > lengths[edge->owner])
			lengths[edge->owner] = len;
	}
	return FALSE;
}

/* Zero the visited attribute for all edges and nodes.
 */
static gboolean zero_visited(Hex * hex, G_GNUC_UNUSED gpointer closure)
{
	guint idx;

	g_return_val_if_fail(hex != NULL, FALSE);
	for (idx = 0; idx < G_N_ELEMENTS(hex->edges); idx++) {
		Edge *edge = hex->edges[idx];
		edge->visited = FALSE;
	}
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); idx++) {
		Node *node = hex->nodes[idx];
		node->visited = FALSE;
	}

	return FALSE;
}

/* Finding the longest road:
 * 1 - set the visited attribute of all edges and nodes to FALSE
 * 2 - for every edge, find the longest road using this one as a tail
 */
void map_longest_road(Map * map, guint * lengths, guint num_players)
{
	g_return_if_fail(map != NULL);
	g_return_if_fail(lengths != NULL);
	g_return_if_fail(num_players > 0);

	map_traverse(map, zero_visited, NULL);
	memset(lengths, 0, num_players * sizeof(*lengths));
	map_traverse(map, find_longest_road, lengths);
}

static gboolean map_island_recursive(Map * map, Node * node, gint owner)
{
	guint idx;
	gboolean discovered;

	g_return_val_if_fail(map != NULL, FALSE);

	if (node == NULL)
		return FALSE;
	if (node->owner == owner)
		return TRUE;	/* Already discovered */
	if (node->visited)
		return FALSE;	/* Not discovered */
	node->visited = TRUE;

	discovered = FALSE;
	for (idx = 0; idx < G_N_ELEMENTS(node->edges) && !discovered;
	     idx++) {
		gint num_sea;
		guint idx2;
		Edge *edge = node->edges[idx];
		if (edge == NULL)
			continue;
		if (edge->visited)
			continue;
		edge->visited = TRUE;

		/* If the edge points into the sea, or along the border,
		 * don't follow it */
		num_sea = 0;
		for (idx2 = 0; idx2 < G_N_ELEMENTS(edge->hexes); idx2++) {
			const Hex *hex = edge->hexes[idx2];
			if (hex == NULL) {
				num_sea++;
				continue;
			}
			if (hex->terrain == SEA_TERRAIN)
				num_sea++;
		}
		if (num_sea == G_N_ELEMENTS(edge->hexes))
			continue;

		/* Follow the other node */
		for (idx2 = 0;
		     idx2 < G_N_ELEMENTS(edge->nodes) && !discovered;
		     ++idx2) {
			Node *node2 = edge->nodes[idx2];
			if (node == node2)
				continue;
			discovered |=
			    map_island_recursive(map, node2, owner);
		}
	}
	return discovered;
}

/* Has anything be built by this player on this island */
gboolean map_is_island_discovered(Map * map, Node * node, gint owner)
{
	g_return_val_if_fail(map != NULL, FALSE);
	g_return_val_if_fail(node != NULL, FALSE);
	map_traverse(map, zero_visited, NULL);
	return map_island_recursive(map, node, owner);
}

/* Determine the maritime trading capabilities for the specified player
 */
static gboolean find_maritime(const Hex * hex, gpointer closure)
{
	MaritimeInfo *info = closure;

	g_return_val_if_fail(hex != NULL, FALSE);
	g_return_val_if_fail(info != NULL, FALSE);

	if (hex->terrain != SEA_TERRAIN || hex->resource == NO_RESOURCE)
		return FALSE;

	if (hex->nodes[hex->facing]->owner != info->owner
	    && hex->nodes[(hex->facing + 5) % 6]->owner != info->owner)
		return FALSE;

	if (hex->resource == ANY_RESOURCE)
		info->any_resource = TRUE;
	else
		info->specific_resource[hex->resource] = TRUE;

	return FALSE;
}

/* Determine the maritime trading capacity of the specified player
 */
void map_maritime_info(const Map * map, MaritimeInfo * info, gint owner)
{
	g_return_if_fail(map != NULL);
	g_return_if_fail(info != NULL);
	memset(info, 0, sizeof(*info));
	info->owner = owner;
	map_traverse_const(map, find_maritime, info);
}

typedef struct {
	gboolean visited[MAP_SIZE][MAP_SIZE];
	guint count;
	guint recursion_level;
} IslandCount;

static gboolean count_islands(const Hex * hex, gpointer info)
{
	IslandCount *count = info;
	HexDirection direction;

	if (hex == NULL)
		return FALSE;

	g_return_val_if_fail(hex->map != NULL, FALSE);

	if (count->visited[hex->y][hex->x])
		return FALSE;

	if (hex->terrain == SEA_TERRAIN)
		return FALSE;

	count->visited[hex->y][hex->x] = TRUE;
	count->recursion_level++;
	for (direction = 0; direction < 6; direction++) {
		count_islands(hex_in_direction(hex, direction), count);
	}
	count->recursion_level--;
	if (count->recursion_level == 0)
		count->count++;
	return FALSE;
}

guint map_count_islands(const Map * map)
{
	IslandCount island_count;

	g_return_val_if_fail(map != NULL, 0u);

	memset(island_count.visited, 0, sizeof(island_count.visited));
	island_count.count = 0;
	island_count.recursion_level = 0;

	map_traverse_const(map, count_islands, &island_count);

	return island_count.count;
}
