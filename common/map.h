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

#ifndef __map_h
#define __map_h

#include <glib.h>

/* The order of the Terrain enums is EXTREMELY important!  The order
 * must match the resources indicated in enum Resource.
 */
typedef enum {
	HILL_TERRAIN,
	FIELD_TERRAIN,
	MOUNTAIN_TERRAIN,
	PASTURE_TERRAIN,
	FOREST_TERRAIN,
	DESERT_TERRAIN,
	SEA_TERRAIN,
	GOLD_TERRAIN,
	LAST_TERRAIN		/* New terrain types go before this */
} Terrain;

/* The order of the Resource enums up to NO_RESOURCE is EXTREMELY important!
 * The numbers are used to index arrays in cost_*(), and to identify
 * resource accumulators in player statistics.  The NO_RESOURCE marks
 * the end of the actual resources.
 */
typedef enum {
	BRICK_RESOURCE,
	GRAIN_RESOURCE,
	ORE_RESOURCE,
	WOOL_RESOURCE,
	LUMBER_RESOURCE,
	NO_RESOURCE,		/* All normal producing resources go before this */
	ANY_RESOURCE,		/* Used for 3:1 ports */
	GOLD_RESOURCE		/* Gold */
} Resource;

/** Maps a Terrain to a Resource.
 * This does not take ports into account.
 * @param terrain The terrain
 * @return The resource the terrain produces
 */
Resource terrain_to_resource(Terrain terrain);

/** Maps a Resource to a Terrain.
 * @param resource The resource
 * @return The terrain that produces this resource
 */
Terrain resource_to_terrain(Resource resource);

/* Types of structure that can be built
 */
typedef enum {
	BUILD_NONE,		/* vacant node/edge */
	BUILD_ROAD,		/* road was built */
	BUILD_BRIDGE,		/* bridge was built */
	BUILD_SHIP,		/* ship was built */
	BUILD_SETTLEMENT,	/* settlement was built */
	BUILD_CITY,		/* city was built */
	BUILD_CITY_WALL,	/* city wall was built */
	BUILD_MOVE_SHIP		/* a ship was moved (only used for undo list) */
} BuildType;

#define NUM_BUILD_TYPES (BUILD_CITY_WALL + 1)

/* Maps are built up from a network of hexes, edges, and nodes.
 *
 * Each hex has connections to six edges, and six nodes.  Each node
 * connects to three hexes and three edges, and each edge connects to
 * two hexes and two nodes.
 */
typedef struct _Node Node;
typedef struct _Edge Edge;
typedef struct _Hex Hex;
typedef struct _Map Map;
struct _Hex {
	Map *map;		/* owner map */
	gint x;			/* x-pos on grid */
	gint y;			/* y-pos on grid */

	Node *nodes[6];		/* adjacent nodes */
	Edge *edges[6];		/* adjacent edges */
	Terrain terrain;	/* type of terrain for this hex */
	Resource resource;	/* resource at this port */
	gint facing;		/* direction port is facing */
	gint chit_pos;		/* position in chit layout sequence */

	gint roll;		/* 2..12 number allocated to hex */
	gboolean robber;	/* is the robber here */
	gboolean shuffle;	/* can the hex be shuffled? */
};

struct _Node {
	Map *map;		/* owner map */
	gint x;			/* x-pos of owner hex */
	gint y;			/* y-pos of owner hex */
	gint pos;		/* location of node on hex */

	Hex *hexes[3];		/* adjacent hexes */
	Edge *edges[3];		/* adjacent edges */
	gint owner;		/* building owner, -1 == no building */
	BuildType type;		/* type of node (if owner defined) */

	gboolean visited;	/* used for longest road */
	gboolean no_setup;	/* setup is not allowed on this node */
	gboolean city_wall;	/* has city wall */
};

struct _Edge {
	Map *map;		/* owner map */
	gint x;			/* x-pos of owner hex */
	gint y;			/* y-pos of owner hex */
	gint pos;		/* location of edge on hex */

	Hex *hexes[2];		/* adjacent hexes */
	Node *nodes[2];		/* adjacent nodes */
	gint owner;		/* road owner, -1 == no road */
	BuildType type;		/* type of edge (if owner defined) */

	gboolean visited;	/* used for longest road */
};

/* All of the hexes are stored in a 2 dimensional array laid out as
 * shown in map.c
 */
#define MAP_SIZE 32		/* maximum map dimension */

struct _Map {
	gint y;			/* current y-pos during parse */

	gboolean have_bridges;	/* are bridges legal on map? */
	gboolean has_pirate;	/* is the pirate allowed in this game? */
	gint x_size;		/* number of hexes across map */
	gint y_size;		/* number of hexes down map */
	Hex *grid[MAP_SIZE][MAP_SIZE];	/* hexes arranged onto a grid */
	Hex *robber_hex;	/* which hex is the robber on */
	Hex *pirate_hex;	/* which hex is the pirate on */
	gboolean has_moved_ship;	/* has the player moved a ship already? */

	gboolean shrink_left;	/* shrink left x-margin? */
	gboolean shrink_right;	/* shrink right x-margin? */
	GArray *chits;		/* chit number sequence */
};

typedef struct {
	gint owner;
	gboolean any_resource;
	gboolean specific_resource[NO_RESOURCE];
} MaritimeInfo;

typedef enum {
	HEX_DIR_E,
	HEX_DIR_NE,
	HEX_DIR_NW,
	HEX_DIR_W,
	HEX_DIR_SW,
	HEX_DIR_SE
} HexDirection;

/* map.c
 */
Hex *map_hex(Map * map, gint x, gint y);
const Hex *map_hex_const(const Map * map, gint x, gint y);
Hex *hex_in_direction(const Hex * hex, HexDirection direction);
void map_move_in_direction(HexDirection direction, gint * x, gint * y);
Edge *map_edge(Map * map, gint x, gint y, gint pos);
const Edge *map_edge_const(const Map * map, gint x, gint y, gint pos);
Node *map_node(Map * map, gint x, gint y, gint pos);
const Node *map_node_const(const Map * map, gint x, gint y, gint pos);
typedef gboolean(*HexFunc) (Hex * hex, gpointer closure);
gboolean map_traverse(Map * map, HexFunc func, gpointer closure);
typedef gboolean(*ConstHexFunc) (const Hex * hex, gpointer closure);
gboolean map_traverse_const(const Map * map, ConstHexFunc func,
			    gpointer closure);
void map_shuffle_terrain(Map * map);
Hex *map_robber_hex(Map * map);
Hex *map_pirate_hex(Map * map);
void map_move_robber(Map * map, gint x, gint y);
void map_move_pirate(Map * map, gint x, gint y);

Map *map_new(void);
Map *map_copy(const Map * map);
gchar *map_format_line(Map * map, gboolean write_secrets, gint y);
gboolean map_parse_line(Map * map, const gchar * line);
gboolean map_parse_finish(Map * map);
void map_free(Map * map);

typedef enum {
	MAP_MODIFY_INSERT,
	MAP_MODIFY_REMOVE
} MapModify;

typedef enum {
	MAP_MODIFY_ROW_TOP,
	MAP_MODIFY_ROW_BOTTOM
} MapModifyRowLocation;

typedef enum {
	MAP_MODIFY_COLUMN_LEFT,
	MAP_MODIFY_COLUMN_RIGHT
} MapModifyColumnLocation;

/** Modify the amount of rows.
 * @param map The map to modify.
 * @param type Insert or delete.
 * @param location At the top or the bottom.
*/
void map_modify_row_count(Map * map, MapModify type,
			  MapModifyRowLocation location);

/** Modify the amount of columns.
 * @param map The map to modify.
 * @param type Insert or delete.
 * @param location At left or right.
*/
void map_modify_column_count(Map * map, MapModify type,
			     MapModifyColumnLocation location);
/** Reset the hex to the default values.
 * @param map The map to modify.
 * @param x X coordinate.
 * @param y Y coordinate.
 */
void map_reset_hex(Map * map, gint x, gint y);

/* map_query.c
 */
/* simple checks */
gboolean is_edge_adjacent_to_node(const Edge * edge, const Node * node);
gboolean is_edge_on_land(const Edge * edge);
gboolean is_edge_on_sea(const Edge * edge);
gboolean is_node_on_land(const Node * node);
gboolean node_has_road_owned_by(const Node * node, gint owner);
gboolean node_has_ship_owned_by(const Node * node, gint owner);
gboolean node_has_bridge_owned_by(const Node * node, gint owner);
gboolean is_node_spacing_ok(const Node * node);
gboolean is_node_proximity_ok(const Node * node);
gboolean is_node_next_to_robber(const Node * node);
/* cursor checks */
gboolean can_road_be_setup(const Edge * edge);
gboolean can_road_be_built(const Edge * edge, gint owner);
gboolean can_ship_be_setup(const Edge * edge);
gboolean can_ship_be_built(const Edge * edge, gint owner);
gboolean can_ship_be_moved(const Edge * edge, gint owner);
gboolean can_bridge_be_setup(const Edge * edge);
gboolean can_bridge_be_built(const Edge * edge, gint owner);
gboolean can_settlement_be_setup(const Node * node);
gboolean can_settlement_be_built(const Node * node, gint owner);
gboolean can_settlement_be_upgraded(const Node * node, gint owner);
gboolean can_city_be_built(const Node * node, int owner);
gboolean can_city_wall_be_built(const Node * node, int owner);
gboolean can_robber_or_pirate_be_moved(const Hex * hex);
/* map global queries */
gboolean map_can_place_road(const Map * map, gint owner);
gboolean map_can_place_ship(const Map * map, gint owner);
gboolean map_can_place_bridge(const Map * map, gint owner);
gboolean map_can_place_settlement(const Map * map, gint owner);
gboolean map_can_place_city_wall(const Map * map, gint owner);
gboolean map_can_upgrade_settlement(const Map * map, gint owner);

gboolean map_building_spacing_ok(Map * map, gint owner, BuildType type,
				 gint x, gint y, gint pos);
gboolean map_building_connect_ok(const Map * map, gint owner, gint x,
				 gint y, gint pos);
gboolean map_building_vacant(Map * map, BuildType type, gint x, gint y,
			     gint pos);
gboolean map_road_vacant(Map * map, gint x, gint y, gint pos);
gboolean map_road_connect_ok(const Map * map, gint owner, gint x, gint y,
			     gint pos);
gboolean map_ship_vacant(Map * map, gint x, gint y, gint pos);
gboolean map_ship_connect_ok(const Map * map, gint owner, gint x, gint y,
			     gint pos);
gboolean map_bridge_vacant(Map * map, gint x, gint y, gint pos);
gboolean map_bridge_connect_ok(const Map * map, gint owner, gint x, gint y,
			       gint pos);
/* information gathering */
void map_longest_road(Map * map, guint * lengths, guint num_players);
gboolean map_is_island_discovered(Map * map, Node * node, gint owner);
void map_maritime_info(const Map * map, MaritimeInfo * info, gint owner);
guint map_count_islands(const Map * map);

#endif
