/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2011 Micah Bunting <Amnykon@gmail.com>
 * Copyright (C) 2011 Roland Clobus <rclobus@rclobus.nl>
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
#include <ctype.h>
#include <string.h>
#include <glib.h>

#include "game.h"
#include "random.h"
#include "map.h"

/* The numbering of the hexes, nodes and edges:
 *
 *                   /\       /\
 *               /1\/0 \     / 0\/1\
 *              /   \1 0\   /0 1/   \
 *             /2   1\ 1/\ /\1 /2   1\
 *            /       \/1 0 0\/       \
 *           /2       0\1 2 2/2       0\
 *          |           |---|           |
 *          |           | 0 |           |
 *          |3         0|1 0|3         0|
 *          |           | 1 |           |
 *          |           |---|           |
 *           \3       5/1 0 0\3       5/
 *           /\       /\1 2 2/\       /
 *       /1\/0 \4   5/ 0\/1\/0 \4   5/
 *      /   \1 0\   /0 1/   \1 0\   /
 *     /2   1\ 1/\4/\1 /2   1\ 1/\4/
 *    /       \/1 0 0\/       \/
 *   /2       0\1 2 2/2       0\
 *  |           |---|           |
 *  |           | 0 |           |
 *  |3         0|1 0|3         0|
 *  |           | 1 |           |
 *  |           |---|           |
 *   \3       5/1 0 0\3       5/
 *    \       /\1 2 2/\       /
 *     \4   5/ 0\/ \/0 \4   5/
 *      \   /1 0/   \1 0\   /
 *       \4/\1 /     \ 1/\4/
 *           \/       \/
 */

/* The accessor functions:

 *              /\               *
 *             /  \              *
 *            /    \             *
 *           /      \            *
 *          /        \           *
 *         |          |          *
 *         |          |          *
 *         |    cc_   |          *
 *         |    hex   |          *
 *         |          |          *
 *          \         /          *
 *          /\       /\          *
 *         /  \     /  \         *
 *      /\/ cc_\   /op_ \/\      *      /\      /\/\      /\
 *     /  \edge/\4/\edge/  \     *     /  \    /    \    /  \
 *    /   1\ 5/2 1 0\3 /    \    *    /    \  /  cc_ \  /    \
 *   /      \/ node  \/      \   *   /      \/  node  \/      \
 *  /        \3  4  5/        \  *  /        \    4   /        \
 * |         0|-----|2         | * |         0|------|          |
 * |   hex    |  1  |    cw_   | * |   hex    |   1  |    op_   |
 * |         0| cw_ |    hex   | * |         0|3edge0|3   hex   |
 * |          | edge|          | * |          |   4  |          |
 * |          |-----|          | * |         5|------|          |
 *  \        /       \        /  *  \        /   1    \        /
 *   \      /         \      /   *   \      /\   cw_  /\      /
 *    \    /           \    /    *    \    /  \ node /  \    /
 *     \  /             \  /     *     \  /    \    /    \  /
 *      \/               \/      *      \/      \/\/      \/
 *
 */

/* Function of shrink_left and shrink_right:
 *
 *  / \ / \       / \ / \         / \           / \
 * | A | B |     | A | B |       | B |         | B |
 *  \ / \ / \     \ / \ /       / \ / \       / \ /
 *   | C | D |     | C |       | C | D |     | C |
 *    \ / \ /       \ /         \ / \ /       \ /
 * 2 2 F F         2 2 F T     2 2 T F       2 2 T T
 *
 * The numbers below the map are:
 *  x_size, y_size, shrink_left, shrink_right
 */
static Hex *move_hex(Hex * hex, HexDirection direction);

static Node *get_node(Hex * hex, int dir)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	return hex->nodes[dir];
}

static void set_node(Hex * hex, int dir, Node * node)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	hex->nodes[dir] = node;
}

static Hex *get_cc_hex(Hex * hex, int dir)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	return hex_in_direction(hex, (dir + 1) % 6);
}

static Node *get_cc_hex_node(Hex * hex, int dir)
{
	g_assert(get_cc_hex(hex, dir) != NULL);
	return get_cc_hex(hex, dir)->nodes[(dir + 4) % 6];
}

static Hex *get_cw_hex(Hex * hex, int dir)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	return hex_in_direction(hex, dir);
}

static Node *get_cw_hex_node(Hex * hex, int dir)
{
	g_assert(get_cw_hex(hex, dir) != NULL);
	return get_cw_hex(hex, dir)->nodes[(dir + 2) % 6];
}

static void set_node_hex(Hex * hex, int dir, Hex * new_hex)
{
	g_assert(get_node(hex, dir) != NULL);
	get_node(hex, dir)->hexes[(dir + 3) / 2 % 3] = new_hex;
}

static void set_node_cc_edge(Hex * hex, int dir, Edge * edge)
{
	g_assert(get_node(hex, dir) != NULL);
	get_node(hex, dir)->edges[(dir + 2) / 2 % 3] = edge;
}

static void set_node_cw_edge(Hex * hex, int dir, Edge * edge)
{
	g_assert(get_node(hex, dir) != NULL);
	get_node(hex, dir)->edges[(dir + 4) / 2 % 3] = edge;
}

static Edge *get_cc_edge(Hex * hex, int dir)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	return hex->edges[(dir + 1) % 6];
}

static Edge *get_cw_edge(Hex * hex, int dir)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	return hex->edges[dir];
}

static Edge *get_edge(Hex * hex, int dir)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	return hex->edges[dir];
}

static void set_edge(Hex * hex, int dir, Edge * edge)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	hex->edges[dir] = edge;
}

static Hex *get_op_hex(Hex * hex, int dir)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	return hex_in_direction(hex, dir);
}

static Edge *get_op_hex_edge(Hex * hex, int dir)
{
	g_assert(get_op_hex(hex, dir) != NULL);
	return get_op_hex(hex, dir)->edges[(dir + 3) % 6];
}

static Node *get_cc_node(Hex * hex, int dir)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	return hex->nodes[dir];
}

static void set_cc_node_edge(Hex * hex, int dir, Edge * edge)
{
	g_assert(get_cc_node(hex, dir) != NULL);
	get_cc_node(hex, dir)->edges[(dir + 4) / 2 % 3] = edge;
}

static Node *get_cw_node(Hex * hex, int dir)
{
	g_assert(hex != NULL && dir < 6 && dir >= 0);
	return hex->nodes[(dir + 5) % 6];
}

static void set_cw_node_edge(Hex * hex, int dir, Edge * edge)
{
	g_assert(get_cw_node(hex, dir) != NULL);
	get_cw_node(hex, dir)->edges[(dir + 1) / 2 % 3] = edge;
}

static void set_edge_cc_node(Hex * hex, int dir, Node * node)
{
	g_assert(get_edge(hex, dir) != NULL);
	get_edge(hex, dir)->nodes[(dir + 1) / 3 % 2] = node;
}

static void set_edge_cw_node(Hex * hex, int dir, Node * node)
{
	g_assert(get_edge(hex, dir) != NULL);
	get_edge(hex, dir)->nodes[(dir + 4) / 3 % 2] = node;
}

static void set_edge_hex(Hex * hex, int dir, Hex * new_hex)
{
	g_assert(get_edge(hex, dir) != NULL);
	get_edge(hex, dir)->hexes[(dir + 3) / 3 % 2] = new_hex;
}

Resource terrain_to_resource(Terrain terrain)
{
	switch (terrain) {
	case HILL_TERRAIN:
		return BRICK_RESOURCE;
	case FIELD_TERRAIN:
		return GRAIN_RESOURCE;
	case MOUNTAIN_TERRAIN:
		return ORE_RESOURCE;
	case PASTURE_TERRAIN:
		return WOOL_RESOURCE;
	case FOREST_TERRAIN:
		return LUMBER_RESOURCE;
	case DESERT_TERRAIN:
		return NO_RESOURCE;
	case SEA_TERRAIN:
		return NO_RESOURCE;
	case GOLD_TERRAIN:
		return GOLD_RESOURCE;
	case LAST_TERRAIN:
		return NO_RESOURCE;
	}
	g_assert_not_reached();
}

Terrain resource_to_terrain(Resource resource)
{
	switch (resource) {
	case BRICK_RESOURCE:
		return HILL_TERRAIN;
	case GRAIN_RESOURCE:
		return FIELD_TERRAIN;
	case ORE_RESOURCE:
		return MOUNTAIN_TERRAIN;
	case WOOL_RESOURCE:
		return PASTURE_TERRAIN;
	case LUMBER_RESOURCE:
		return FOREST_TERRAIN;
	case NO_RESOURCE:
		return SEA_TERRAIN;
	case ANY_RESOURCE:
		return SEA_TERRAIN;
	case GOLD_RESOURCE:
		return GOLD_TERRAIN;
	}
	g_assert_not_reached();
}

Hex *map_hex(Map * map, gint x, gint y)
{
	if (x < 0 || x >= map->x_size || y < 0 || y >= map->y_size)
		return NULL;

	return map->grid[y][x];
}

const Hex *map_hex_const(const Map * map, gint x, gint y)
{
	if (x < 0 || x >= map->x_size || y < 0 || y >= map->y_size)
		return NULL;

	return map->grid[y][x];
}

/** Returns the hex in the given direction, or NULL
 */
Hex *hex_in_direction(const Hex * hex, HexDirection direction)
{
	gint x = hex->x;
	gint y = hex->y;

	map_move_in_direction(direction, &x, &y);

	return map_hex(hex->map, x, y);
}

/** Move the hex coordinate in the given direction.
 * @param direction Move in this direction
 * @retval x x-coordinate of the hex to move
 * @retval y y-coordinate of the hex to move
*/
void map_move_in_direction(HexDirection direction, gint * x, gint * y)
{
	switch (direction) {
	case HEX_DIR_E:
		(*x)++;
		break;
	case HEX_DIR_NE:
		if (*y % 2 == 1)
			(*x)++;
		(*y)--;
		break;
	case HEX_DIR_NW:
		if (*y % 2 == 0)
			(*x)--;
		(*y)--;
		break;
	case HEX_DIR_W:
		(*x)--;
		break;
	case HEX_DIR_SW:
		if (*y % 2 == 0)
			(*x)--;
		(*y)++;
		break;
	case HEX_DIR_SE:
		if (*y % 2 == 1)
			(*x)++;
		(*y)++;
		break;
	}
}

Node *map_node(Map * map, gint x, gint y, gint pos)
{
	Hex *hex;

	if (x < 0 || x >= map->x_size
	    || y < 0 || y >= map->y_size || pos < 0 || pos >= 6)
		return NULL;

	hex = map->grid[y][x];
	if (hex == NULL)
		return NULL;
	return hex->nodes[pos];
}

const Node *map_node_const(const Map * map, gint x, gint y, gint pos)
{
	const Hex *hex;

	if (x < 0 || x >= map->x_size
	    || y < 0 || y >= map->y_size || pos < 0 || pos >= 6)
		return NULL;

	hex = map->grid[y][x];
	if (hex == NULL)
		return NULL;
	return hex->nodes[pos];
}

Edge *map_edge(Map * map, gint x, gint y, gint pos)
{
	Hex *hex;

	if (x < 0 || x >= map->x_size
	    || y < 0 || y >= map->y_size || pos < 0 || pos >= 6)
		return NULL;

	hex = map->grid[y][x];
	if (hex == NULL)
		return NULL;
	return hex->edges[pos];
}

const Edge *map_edge_const(const Map * map, gint x, gint y, gint pos)
{
	const Hex *hex;

	if (x < 0 || x >= map->x_size
	    || y < 0 || y >= map->y_size || pos < 0 || pos >= 6)
		return NULL;

	hex = map->grid[y][x];
	if (hex == NULL)
		return NULL;
	return hex->edges[pos];
}

/** Traverse the map and perform processing at a each node.
 *
 * If the callback function returns TRUE, stop traversal immediately
 * and return TRUE to caller,
 */
gboolean map_traverse(Map * map, HexFunc func, gpointer closure)
{
	gint x;

	for (x = 0; x < map->x_size; x++) {
		gint y;

		for (y = 0; y < map->y_size; y++) {
			Hex *hex;

			hex = map->grid[y][x];
			if (hex != NULL && func(hex, closure))
				return TRUE;
		}
	}

	return FALSE;
}

/** Traverse the map and perform processing at a each node.
 * The map is unmodified.
 *
 * If the callback function returns TRUE, stop traversal immediately
 * and return TRUE to caller,
 */
gboolean map_traverse_const(const Map * map, ConstHexFunc func,
			    gpointer closure)
{
	gint x;

	for (x = 0; x < map->x_size; x++) {
		gint y;

		for (y = 0; y < map->y_size; y++) {
			const Hex *hex;

			hex = map->grid[y][x];
			if (hex != NULL && func(hex, closure))
				return TRUE;
		}
	}

	return FALSE;
}

/* To expand the grid to a network, we build a chain of nodes and
 * edges around the current node.  Before allocating a new node or
 * edge, we must check if the node or edge has already been created by
 * processing an adjacent hex.
 *
 * Each node has three adjacent hexes, so we must check two other
 * hexes to see if the node has already been created.  Once we have
 * found or created the node for a specific position, we must attach
 * this hex to a specific position on that node.
 *
 * Each edge has only two adjacent hexes, so we check the other hex to
 * see if the edge exists before creating it.
 */

/* Build ring of nodes and edges around the current hex
 */
static gboolean build_network(Hex * hex, G_GNUC_UNUSED gpointer closure)
{
	gint idx;

	for (idx = 0; idx < 6; idx++) {
		Node *node = NULL;
		Edge *edge = NULL;

		if (get_cc_hex(hex, idx) != NULL)
			node = get_cc_hex_node(hex, idx);
		if (node == NULL && get_cw_hex(hex, idx) != NULL)
			node = get_cw_hex_node(hex, idx);
		if (node == NULL) {
			node = g_malloc0(sizeof(*node));
			node->map = hex->map;
			node->owner = -1;
			node->x = hex->x;
			node->y = hex->y;
			node->pos = idx;
		}
		set_node(hex, idx, node);
		set_node_hex(hex, idx, hex);

		if (get_op_hex(hex, idx) != NULL)
			edge = get_op_hex_edge(hex, idx);
		if (edge == NULL) {
			edge = g_malloc0(sizeof(*edge));
			edge->map = hex->map;
			edge->owner = -1;
			edge->x = hex->x;
			edge->y = hex->y;
			edge->pos = idx;
		}
		set_edge(hex, idx, edge);
		set_edge_hex(hex, idx, hex);
	}

	return FALSE;
}

/* Connect all of the adjacent nodes and edges to each other.
 *
 * A node connects to three edges, but we only bother connecting the
 * edges that are adjacent to this hex.  Once the entire grid of hexes
 * has been processed, all nodes (which require them) will have three
 * edges.
 */

/* Connect the the ring of nodes and edges to each other
 */
static gboolean connect_network(Hex * hex, G_GNUC_UNUSED gpointer closure)
{
	gint idx;
	for (idx = 0; idx < 6; idx++) {
		/* Connect current edge to adjacent nodes */
		set_edge_cc_node(hex, idx, get_cc_node(hex, idx));
		set_edge_cw_node(hex, idx, get_cw_node(hex, idx));
		/* Connect current node to adjacent edges */
		set_node_cc_edge(hex, idx, get_cc_edge(hex, idx));
		set_node_cw_edge(hex, idx, get_cw_edge(hex, idx));
	}

	return FALSE;
}

/* Layout the dice chits on the map according to the order specified.
 * When laying out the chits, we do not place one on the desert hex.
 * The maps only specify the layout sequence. When loading the map,
 * the program when performs the layout, skipping the desert hex.
 *
 * By making the program perform the layout, we have the ability to
 * shuffle the terrain hexes and then lay the chits out accounting for
 * the new position of the desert.
 * Returns TRUE if the chits could be distributed without errors
 */
static gboolean layout_chits(Map * map)
{
	Hex **hexes;
	guint num_chits;
	gint x, y;
	guint idx;
	guint chit_idx;
	guint num_deserts;

	g_return_val_if_fail(map != NULL, FALSE);
	g_return_val_if_fail(map->chits != NULL, FALSE);
	g_return_val_if_fail(map->chits->len > 0, FALSE);

	/* Count the number of hexes that have chits on them
	 */
	num_chits = 0;
	num_deserts = 0;
	for (x = 0; x < map->x_size; x++)
		for (y = 0; y < map->y_size; y++) {
			Hex *hex = map->grid[y][x];
			if (hex != NULL
			    && hex->chit_pos >= (gint) num_chits)
				num_chits = (guint) (hex->chit_pos + 1);
			if (hex != NULL && hex->terrain == DESERT_TERRAIN)
				num_deserts++;
		}

	/* Traverse the map and build an array of hexes in chit layout
	 * sequence.
	 */
	hexes = g_malloc0(num_chits * sizeof(*hexes));
	for (x = 0; x < map->x_size; x++)
		for (y = 0; y < map->y_size; y++) {
			Hex *hex = map->grid[y][x];
			if (hex == NULL || hex->chit_pos < 0)
				continue;
			if (hexes[hex->chit_pos] != NULL) {
				g_warning("Sequence number %d used again",
					  hex->chit_pos);
				g_free(hexes);
				return FALSE;
			}
			hexes[hex->chit_pos] = hex;
		}

	/* Check the number of chits */
	if (num_chits < map->chits->len + num_deserts) {
		g_warning("More chits (%u + %u) than available tiles (%u)",
			  map->chits->len, num_deserts, num_chits);
		g_free(hexes);
		return FALSE;
	}
	/* If less chits are defined than tiles that need chits,
	 * the sequence is used again
	 */

	/* Now layout the chits in the sequence specified, skipping
	 * the desert hex.
	 */
	chit_idx = 0;
	for (idx = 0; idx < num_chits; idx++) {
		Hex *hex = hexes[idx];
		if (hex == NULL)
			continue;

		if (hex->terrain == DESERT_TERRAIN) {
			/* Robber always starts in the desert
			 */
			hex->roll = 0;
			if (map->robber_hex == NULL) {
				hex->robber = TRUE;
				map->robber_hex = hex;
			}
		} else {
			hex->robber = FALSE;
			hex->roll =
			    g_array_index(map->chits, gint, chit_idx);
			chit_idx++;
			if (chit_idx == map->chits->len)
				chit_idx = 0;
		}
	}
	g_free(hexes);
	return TRUE;
}

/* Randomise a map.  We do this by shuffling all of the land hexes,
 * and randomly reassigning port types.  This is the procedure
 * described in the board game rules.
 */
void map_shuffle_terrain(Map * map)
{
	gint terrain_count[LAST_TERRAIN];
	gint port_count[ANY_RESOURCE + 1];
	gint x, y;
	gint num_terrain;
	gint num_port;

	/* Remove robber, because the desert will probably move.
	 * It will be restored by layout_chits.
	 */
	if (map->robber_hex) {
		map->robber_hex->robber = FALSE;
		map->robber_hex = NULL;
	}

	/* Count number of each terrain type
	 */
	memset(terrain_count, 0, sizeof(terrain_count));
	memset(port_count, 0, sizeof(port_count));
	num_terrain = num_port = 0;
	for (x = 0; x < map->x_size; x++) {
		for (y = 0; y < map->y_size; y++) {
			Hex *hex = map->grid[y][x];
			if (hex == NULL || hex->shuffle == FALSE)
				continue;
			if (hex->terrain == SEA_TERRAIN) {
				if (hex->resource == NO_RESOURCE)
					continue;
				port_count[hex->resource]++;
				num_port++;
			} else {
				terrain_count[hex->terrain]++;
				num_terrain++;
			}
		}
	}

	/* Shuffle the terrain / port types
	 */
	for (x = 0; x < map->x_size; x++) {
		for (y = 0; y < map->y_size; y++) {
			Hex *hex = map->grid[y][x];
			gint num;
			guint idx;

			if (hex == NULL || hex->shuffle == FALSE)
				continue;
			if (hex->terrain == SEA_TERRAIN) {
				if (hex->resource == NO_RESOURCE)
					continue;
				num = random_guint(num_port);
				for (idx = 0;
				     idx < G_N_ELEMENTS(port_count);
				     idx++) {
					num -= port_count[idx];
					if (num < 0)
						break;
				}
				port_count[idx]--;
				num_port--;
				hex->resource = idx;
			} else {
				num = random_guint(num_terrain);
				for (idx = 0;
				     idx < G_N_ELEMENTS(terrain_count);
				     idx++) {
					num -= terrain_count[idx];
					if (num < 0)
						break;
				}
				terrain_count[idx]--;
				num_terrain--;
				hex->terrain = idx;
			}
		}
	}

	/* Fix the chits - the desert probably moved
	 */
	layout_chits(map);
}

Hex *map_robber_hex(Map * map)
{
	return map->robber_hex;
}

Hex *map_pirate_hex(Map * map)
{
	return map->pirate_hex;
}

void map_move_robber(Map * map, gint x, gint y)
{
	if (map->robber_hex != NULL)
		map->robber_hex->robber = FALSE;
	map->robber_hex = map_hex(map, x, y);
	if (map->robber_hex != NULL)
		map->robber_hex->robber = TRUE;
}

void map_move_pirate(Map * map, gint x, gint y)
{
	map->pirate_hex = map_hex(map, x, y);
}

/* Allocate a new map
 */
Map *map_new(void)
{
	return g_malloc0(sizeof(Map));
}

static Hex *hex_new(Map * map, gint x, gint y)
{
	Hex *hex;

	g_assert(map != NULL);
	g_assert(x >= 0);
	g_assert(x < map->x_size);
	g_assert(y >= 0);
	g_assert(y < map->y_size);
	g_assert(map->grid[y][x] == NULL);

	hex = g_malloc0(sizeof(*hex));
	map->grid[y][x] = hex;

	hex->map = map;
	hex->x = x;
	hex->y = y;
	build_network(hex, NULL);
	connect_network(hex, NULL);
	return hex;
}

/** Copy a hex.
 * @param map The new owner
 * @param hex The original hex
 * @return A copy of the original hex, with the new owner.
 *         The copy is not connected (nodes and edges are NULL)
*/
static Hex *copy_hex(Map * map, const Hex * hex)
{
	Hex *copy;

	if (hex == NULL)
		return NULL;
	copy = g_malloc0(sizeof(*copy));
	copy->map = map;
	copy->y = hex->y;
	copy->x = hex->x;
	copy->terrain = hex->terrain;
	copy->resource = hex->resource;
	copy->facing = hex->facing;
	copy->chit_pos = hex->chit_pos;
	copy->roll = hex->roll;
	copy->robber = hex->robber;
	copy->shuffle = hex->shuffle;

	return copy;
}

static gboolean set_nosetup_nodes(const Hex * hex, gpointer closure)
{
	guint idx;
	Map *copy = closure;
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); ++idx) {
		const Node *node = hex->nodes[idx];
		/* only handle nodes which are owned by the hex, to
		 * prevent doing every node three times */
		if (hex->x != node->x || hex->y != node->y)
			continue;
		g_assert(map_node(copy, node->x, node->y, node->pos) !=
			 NULL);
		map_node(copy, node->x, node->y, node->pos)->no_setup =
		    node->no_setup;
	}
	return FALSE;
}

static GArray *copy_int_list(GArray * array)
{
	GArray *copy;
	guint idx;

	if (array == NULL) {
		return NULL;
	}
	copy = g_array_new(FALSE, FALSE, sizeof(gint));
	for (idx = 0; idx < array->len; idx++)
		g_array_append_val(copy, g_array_index(array, gint, idx));

	return copy;
}

/* Make a copy of an existing map
 */
Map *map_copy(const Map * map)
{
	Map *copy = map_new();
	int x, y;

	copy->y = map->y;
	copy->x_size = map->x_size;
	copy->y_size = map->y_size;
	for (y = 0; y < MAP_SIZE; y++)
		for (x = 0; x < MAP_SIZE; x++)
			copy->grid[y][x] = copy_hex(copy, map->grid[y][x]);
	map_traverse(copy, build_network, NULL);
	map_traverse(copy, connect_network, NULL);
	map_traverse_const(map, set_nosetup_nodes, copy);
	if (map->robber_hex == NULL)
		copy->robber_hex = NULL;
	else
		copy->robber_hex =
		    copy->grid[map->robber_hex->y][map->robber_hex->x];
	if (map->pirate_hex == NULL)
		copy->pirate_hex = NULL;
	else
		copy->pirate_hex =
		    copy->grid[map->pirate_hex->y][map->pirate_hex->x];
	copy->shrink_left = map->shrink_left;
	copy->shrink_right = map->shrink_right;
	copy->has_moved_ship = map->has_moved_ship;
	copy->have_bridges = map->have_bridges;
	copy->has_pirate = map->has_pirate;
	copy->chits = copy_int_list(map->chits);

	return copy;
}

/* Maps are sent from the server to the client a line at a time.  This
 * routine formats a line of a map for just that purpose.
 * It returns an allocated buffer, which must be freed by the caller.
 */
gchar *map_format_line(Map * map, gboolean write_secrets, gint y)
{
	gchar *line = NULL;
	gchar buffer[20];	/* Buffer for the info about one hex */
	gint x;

	for (x = 0; x < map->x_size; x++) {
		gchar *bufferpos = buffer;
		Hex *hex = map->grid[y][x];

		if (x > 0)
			*bufferpos++ = ',';
		if (hex == NULL) {
			*bufferpos++ = '-';
		} else {
			switch (hex->terrain) {
			case HILL_TERRAIN:
				*bufferpos++ = 'h';
				break;
			case FIELD_TERRAIN:
				*bufferpos++ = 'f';
				break;
			case MOUNTAIN_TERRAIN:
				*bufferpos++ = 'm';
				break;
			case PASTURE_TERRAIN:
				*bufferpos++ = 'p';
				break;
			case FOREST_TERRAIN:
				*bufferpos++ = 't';	/* tree */
				break;
			case DESERT_TERRAIN:
				*bufferpos++ = 'd';
				break;
			case GOLD_TERRAIN:
				*bufferpos++ = 'g';
				break;
			case SEA_TERRAIN:
				*bufferpos++ = 's';
				if (hex == map->pirate_hex)
					*bufferpos++ = 'R';
				if (hex->resource == NO_RESOURCE)
					break;
				switch (hex->resource) {
				case BRICK_RESOURCE:
					*bufferpos++ = 'b';
					break;
				case GRAIN_RESOURCE:
					*bufferpos++ = 'g';
					break;
				case ORE_RESOURCE:
					*bufferpos++ = 'o';
					break;
				case WOOL_RESOURCE:
					*bufferpos++ = 'w';
					break;
				case LUMBER_RESOURCE:
					*bufferpos++ = 'l';
					break;
				case ANY_RESOURCE:
					*bufferpos++ = '?';
					break;
				case NO_RESOURCE:
					break;
				case GOLD_RESOURCE:
					g_assert_not_reached();
				}
				*bufferpos++ = (gchar) (hex->facing + '0');
				break;
			case LAST_TERRAIN:
				*bufferpos++ = '-';
				break;
			default:
				g_assert_not_reached();
				break;
			}
			if (hex->chit_pos >= 0) {
				sprintf(bufferpos, "%d", hex->chit_pos);
				bufferpos += strlen(bufferpos);
			}
			if (write_secrets && !hex->shuffle) {
				*bufferpos++ = '+';
			}
		}
		*bufferpos = '\0';
		if (line) {
			gchar *old = line;
			line = g_strdup_printf("%s%s", line, buffer);
			g_free(old);
		} else {
			line = g_strdup(buffer);
		}
	}
	return line;
}

/* Read a map line into the grid
 */
gboolean map_parse_line(Map * map, const gchar * line)
{
	gint x = 0;

	for (;;) {
		Hex *hex;

		switch (*line++) {
		case '\0':
		case '\n':
			map->y++;
			return TRUE;
		case '-':
			x++;
			continue;
		case ',':
		case ' ':
		case '\t':
			continue;
		}

		if (x >= MAP_SIZE || map->y >= MAP_SIZE)
			continue;
		--line;

		hex = g_malloc0(sizeof(*hex));
		hex->map = map;
		hex->y = map->y;
		hex->x = x;
		hex->terrain = SEA_TERRAIN;
		hex->resource = NO_RESOURCE;
		hex->facing = 0;
		hex->chit_pos = -1;
		hex->shuffle = TRUE;

		switch (*line++) {
		case 's':	/* sea */
			hex->terrain = SEA_TERRAIN;
			if (*line == 'R') {
				++line;
				map->pirate_hex = hex;
				map->has_pirate = TRUE;
			}
			switch (*line++) {
			case 'b':
				hex->resource = BRICK_RESOURCE;
				break;
			case 'g':
				hex->resource = GRAIN_RESOURCE;
				break;
			case 'o':
				hex->resource = ORE_RESOURCE;
				break;
			case 'w':
				hex->resource = WOOL_RESOURCE;
				break;
			case 'l':
				hex->resource = LUMBER_RESOURCE;
				break;
			case 'm':	/* mine */
				hex->resource = GOLD_RESOURCE;
				break;
			case '?':
				hex->resource = ANY_RESOURCE;
				break;
			default:
				hex->resource = NO_RESOURCE;
				--line;
				break;
			}
			hex->facing = 0;
			if (hex->resource != NO_RESOURCE) {
				if (isdigit(*line))
					hex->facing = *line++ - '0';
			}
			break;
		case 't':	/* tree */
			hex->terrain = FOREST_TERRAIN;
			break;
		case 'p':
			hex->terrain = PASTURE_TERRAIN;
			break;
		case 'f':
			hex->terrain = FIELD_TERRAIN;
			break;
		case 'h':
			hex->terrain = HILL_TERRAIN;
			break;
		case 'm':
			hex->terrain = MOUNTAIN_TERRAIN;
			break;
		case 'd':
			hex->terrain = DESERT_TERRAIN;
			break;
		case 'g':
			hex->terrain = GOLD_TERRAIN;
			break;
		default:
			g_free(hex);
			continue;
		}

		/* Read the chit sequence number
		 */
		if (isdigit(*line)) {
			hex->chit_pos = 0;
			while (isdigit(*line))
				hex->chit_pos = hex->chit_pos * 10
				    + *line++ - '0';
		}

		/* Check if hex can be randomly shuffled
		 */
		if (*line == '+') {
			hex->shuffle = FALSE;
			line++;
		}
		if (hex->chit_pos < 0 && hex->terrain != SEA_TERRAIN) {
			g_warning
			    ("Land tile without chit sequence number");
			g_free(hex);
			return FALSE;
		}

		map->grid[map->y][x] = hex;
		if (x >= map->x_size)
			map->x_size = x + 1;
		if (map->y >= map->y_size)
			map->y_size = map->y + 1;
		x++;
	}
	return TRUE;
}

/* Finalise the map loading by building a network of nodes, edges and
 * hexes.  Since every second row of hexes is offset, we might be able
 * to shrink the left / right margins depending on the distribution of
 * hexes.
 * Returns true if the map could be finalised.
 */
gboolean map_parse_finish(Map * map)
{
	gint y;
	gboolean success;

	success = layout_chits(map);

	map_traverse(map, build_network, NULL);
	map_traverse(map, connect_network, NULL);

	map->shrink_left = TRUE;
	map->shrink_right = TRUE;
	for (y = 0; y < map->y_size; y += 2)
		if (map->grid[y][0] != NULL) {
			map->shrink_left = FALSE;
			break;
		}
	for (y = 1; y < map->y_size; y += 2)
		if (map->grid[y][map->x_size - 1] != NULL) {
			map->shrink_right = FALSE;
			break;
		}
	return success;
}

/** Free a hex.
 * Disconnect the hex from the grid.
 */
static void hex_free(Hex * hex)
{
	gint idx;

	g_assert(hex != NULL);
	/* Transfer ownership of edges to adjacent hexes. */
	for (idx = 0; idx < 6; idx++) {
		Edge *edge = get_edge(hex, idx);
		g_assert(edge != NULL);

		if (edge->pos == idx) {	/* if edge owned by hex */
			if (get_op_hex(hex, idx) != NULL) {
				/* change owner */
				edge->x = get_op_hex(hex, idx)->x;
				edge->y = get_op_hex(hex, idx)->y;
				edge->pos = (edge->pos + 3) % 6;
			} else {
				set_cc_node_edge(hex, idx, NULL);
				set_cw_node_edge(hex, idx, NULL);
				g_free(edge);
				continue;
			}
		}
		set_edge_hex(hex, idx, NULL);
	}
	/* Transfer ownership of nodes to adjacent hexes. */
	for (idx = 0; idx < 6; idx++) {
		Node *node = get_node(hex, idx);
		g_assert(node != NULL);
		if (node->pos == idx) {
			if (get_cc_hex(hex, idx) != NULL) {
				/* change owner */
				node->x = get_cc_hex(hex, idx)->x;
				node->y = get_cc_hex(hex, idx)->y;
				node->pos = (node->pos + 4) % 6;
			} else if (get_cw_hex(hex, idx) != NULL) {
				/* change owner */
				node->x = get_cw_hex(hex, idx)->x;
				node->y = get_cw_hex(hex, idx)->y;
				node->pos = (node->pos + 2) % 6;
			} else {
				g_free(node);
				continue;
			}
		}
		set_node_hex(hex, idx, NULL);
	}
	/* Remove from the grid */
	if (hex->map->grid[hex->y][hex->x] == hex)
		hex->map->grid[hex->y][hex->x] = NULL;
	g_free(hex);
}


static gboolean free_hex(Hex * hex, G_GNUC_UNUSED gpointer closure)
{
	hex_free(hex);
	return FALSE;
}

/* Free a map
 */
void map_free(Map * map)
{
	if (map == NULL) {
		return;
	}
	map_traverse(map, free_hex, NULL);
	if (map->chits != NULL) {
		g_array_free(map->chits, TRUE);
	}
	g_free(map);
}

void map_reset_hex(Map * map, gint x, gint y)
{
	Hex *hex;
	Hex *adjacent;
	int i;

	if (x < 0 || x >= map->x_size || y < 0 || y >= map->y_size) {
		g_assert_not_reached();
		return;
	}
	hex = map_hex(map, x, y);
	if (!hex) {
		/* Create a new hex on the previously empty place */
		hex = hex_new(map, x, y);
	}

	g_return_if_fail(hex != NULL);
	hex->terrain = LAST_TERRAIN;
	hex->resource = NO_RESOURCE;
	hex->chit_pos = -1;
	hex->roll = 0;
	hex->shuffle = TRUE;

	/* Clear any ports that face this hex */
	for (i = 0; i < 6; i++) {
		adjacent = hex_in_direction(hex, i);
		if (adjacent != NULL
		    && adjacent->terrain == SEA_TERRAIN
		    && adjacent->resource != NO_RESOURCE
		    && adjacent->facing == (i + 3) % 6) {
			adjacent->resource = NO_RESOURCE;
			adjacent->facing = 0;
		};
	};
}

void map_modify_row_count(Map * map, MapModify type,
			  MapModifyRowLocation location)
{
	gint x;
	gint y;
	gint max;
	gint min;
	Hex *shift_hex;

	if (type == MAP_MODIFY_INSERT && location == MAP_MODIFY_ROW_TOP) {
		/* Shift the map to the right, if needed */
		map->shrink_left = !map->shrink_left;
		if (map->shrink_left) {
			map->x_size++;
			for (y = 0; y < map->y_size; y++) {
				shift_hex = map->grid[y][0];
				while (shift_hex != NULL) {
					shift_hex =
					    move_hex(shift_hex, HEX_DIR_E);
				};
			};
		};
		map->y_size++;
		/* Move all except the top row */
		min = map->shrink_right ? 2 : 1;
		for (y = min; y < map->y_size - 1; y += 2) {
			shift_hex = map->grid[y][map->x_size - 1];
			while (shift_hex != NULL) {
				shift_hex =
				    move_hex(shift_hex, HEX_DIR_SW);
			};
		};
		/* Move the top row */
		min = 1;
		max = map->x_size;
		for (x = min; x < max; x++) {
			shift_hex = map->grid[0][x];
			while (shift_hex != NULL) {
				shift_hex =
				    move_hex(shift_hex, HEX_DIR_SW);
			};
		};
		/* Remove column, if needed */
		if (map->shrink_right) {
			map->x_size--;
		}
		map->shrink_right = !map->shrink_right;
		/* Create the new hexes */
		min = map->shrink_left ? 1 : 0;
		max = map->x_size;
		for (x = min; x < max; x++) {
			map_reset_hex(map, x, 0);
		};
	} else if (type == MAP_MODIFY_INSERT
		   && location == MAP_MODIFY_ROW_BOTTOM) {
		map->y_size++;
		if (map->y_size % 2 == 0) {
			min = 0;
			max =
			    map->shrink_right ? map->x_size -
			    1 : map->x_size;
		} else {
			min = map->shrink_left ? 1 : 0;
			max = map->x_size;
		};
		for (x = min; x < max; x++) {
			map_reset_hex(map, x, map->y_size - 1);
		};
	} else if (type == MAP_MODIFY_REMOVE
		   && location == MAP_MODIFY_ROW_TOP) {
		/* Remove the top row */
		min = map->shrink_left ? 1 : 0;
		max = map->x_size;
		for (x = min; x < max; x++) {
			map_reset_hex(map, x, 0);
			hex_free(map->grid[0][x]);
		};
		/* Shift the map to the right, if needed */
		map->shrink_left = !map->shrink_left;
		if (map->shrink_left) {
			map->x_size++;
			for (y = 1; y < map->y_size; y++) {
				shift_hex = map->grid[y][0];
				while (shift_hex != NULL) {
					shift_hex =
					    move_hex(shift_hex, HEX_DIR_E);
				};
			};
		};
		/* Move all except the bottom row */
		min = map->shrink_right ? 2 : 1;
		for (y = min; y < map->y_size - 1; y += 2) {
			shift_hex = map->grid[y][map->x_size - 1];
			while (shift_hex != NULL) {
				shift_hex =
				    move_hex(shift_hex, HEX_DIR_NW);
			};
		};
		/* Move the bottom row */
		if (map->y_size % 2 == 0) {
			min = 0;
			max =
			    map->shrink_right ? map->x_size -
			    1 : map->x_size;
		} else {
			min = map->shrink_left ? 1 : 0;
			max = map->x_size;
		};
		for (x = min; x < max; x++) {
			shift_hex = map->grid[map->y_size - 1][x];
			while (shift_hex != NULL) {
				shift_hex =
				    move_hex(shift_hex, HEX_DIR_NW);
			};
		};

		/* Remove column, if needed */
		if (map->shrink_right) {
			map->x_size--;
		};
		map->shrink_right = !map->shrink_right;

		map->y_size--;
	} else {
		if (map->y_size % 2 == 0) {
			min = 0;
			max =
			    map->shrink_right ? map->x_size -
			    1 : map->x_size;
		} else {
			min = map->shrink_left ? 1 : 0;
			max = map->x_size;
		};
		for (x = min; x < max; x++) {
			map_reset_hex(map, x, map->y_size - 1);
			hex_free(map->grid[map->y_size - 1][x]);
		};
		map->y_size--;
	}
}

void map_modify_column_count(Map * map, MapModify type,
			     MapModifyColumnLocation location)
{
	gint x;
	gint y;
	Hex *shift_hex;

	if (type == MAP_MODIFY_INSERT
	    && location == MAP_MODIFY_COLUMN_LEFT) {
		map->shrink_left = !map->shrink_left;
		if (map->shrink_left) {
			map->x_size++;
			for (y = 0; y < map->y_size; y++) {
				shift_hex = map->grid[y][0];
				while (shift_hex != NULL) {
					shift_hex =
					    move_hex(shift_hex, HEX_DIR_E);
				};
			};
		};
		for (y = map->shrink_left ? 1 : 0; y < map->y_size; y += 2) {
			map_reset_hex(map, 0, y);
		};
	} else if (type == MAP_MODIFY_INSERT
		   && location == MAP_MODIFY_COLUMN_RIGHT) {
		if (map->shrink_right) {
			y = 1;
		} else {
			y = 0;
			map->x_size++;
		};
		x = map->x_size - 1;

		while (y < map->y_size) {
			map_reset_hex(map, x, y);
			y += 2;
		};
		map->shrink_right = !map->shrink_right;
	} else if (type == MAP_MODIFY_REMOVE
		   && location == MAP_MODIFY_COLUMN_LEFT) {
		/* Clear the hexes */
		for (y = map->shrink_left ? 1 : 0; y < map->y_size; y += 2) {
			map_reset_hex(map, 0, y);
			hex_free(map->grid[y][0]);
		};
		if (map->shrink_left) {
			/* The map was already shrunk, so move all to the left */
			for (y = 0; y < map->y_size; y++) {
				x = (map->shrink_right
				     && y % 2 ==
				     1) ? map->x_size - 2 : map->x_size -
				    1;
				shift_hex = map->grid[y][x];
				while (shift_hex != NULL) {
					shift_hex =
					    move_hex(shift_hex, HEX_DIR_W);
				};
			};
			map->x_size--;
		};
		map->shrink_left = !map->shrink_left;
	} else {
		x = map->x_size - 1;
		for (y = map->shrink_right ? 0 : 1; y < map->y_size;
		     y += 2) {
			map_reset_hex(map, x, y);
			hex_free(map->grid[y][x]);
		};
		if (map->shrink_right) {
			map->x_size--;
		};
		map->shrink_right = !map->shrink_right;
	}
}

/** Move a hex in the given direction.
 *  This function must be called for all hexes on the grid,
 *  it cannot be use for single hexes.
 *  All related edges and nodes are moved too.
 *  @param hex Hex to move.
 *  @param direction Direction to move the hex to.
 *  @return The hex that was at the pointed position.
 */
static Hex *move_hex(Hex * hex, HexDirection direction)
{
	Hex *ret_hex;
	int idx;

	if (hex->map->grid[hex->y][hex->x] == hex) {
		hex->map->grid[hex->y][hex->x] = NULL;
	};
	switch (direction) {
	case HEX_DIR_E:
		hex->x++;
		break;
	case HEX_DIR_NE:
		if (hex->y % 2 == 1) {
			hex->x++;
		};
		hex->y--;
		break;
	case HEX_DIR_NW:
		if (hex->y % 2 == 0) {
			hex->x--;
		};
		hex->y--;
		break;
	case HEX_DIR_W:
		hex->x--;
		break;
	case HEX_DIR_SW:
		if (hex->y % 2 == 0) {
			hex->x--;
		};
		hex->y++;
		break;
	case HEX_DIR_SE:
		if (hex->y % 2 == 1) {
			hex->x++;
		};
		hex->y++;
		break;
	}
	ret_hex = map_hex(hex->map, hex->x, hex->y);

	hex->map->grid[hex->y][hex->x] = hex;
	for (idx = 0; idx < 6; idx++) {
		Edge *edge = hex->edges[idx];
		Node *node = hex->nodes[idx];
		if (edge != NULL && edge->pos == idx) {
			edge->x = hex->x;
			edge->y = hex->y;
		};
		if (node != NULL && node->pos == idx) {
			node->x = hex->x;
			node->y = hex->y;
		};
	};
	return ret_hex;
}
