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

#ifndef __game_h
#define __game_h

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include "map.h"
#include "driver.h"

typedef enum {
	DEVEL_ROAD_BUILDING,
	DEVEL_MONOPOLY,
	DEVEL_YEAR_OF_PLENTY,
	DEVEL_CHAPEL,
	DEVEL_UNIVERSITY,
	DEVEL_GOVERNORS_HOUSE,
	DEVEL_LIBRARY,
	DEVEL_MARKET,
	DEVEL_SOLDIER
} DevelType;
#define NUM_DEVEL_TYPES (DEVEL_SOLDIER + 1)

#define MAX_PLAYERS 8		/* maximum number of players supported */
#define MAX_CHAT 496		/* maximum chat message size
				 * (512 - strlen("player 0 chat \n") - 1) */
#define MAX_NAME_LENGTH 30	/* maximum length for the name of a player */

/* Supported versions.  These are ordered, so that it is possible to see
 * if versions are greater or smaller than each other.  The actual values do
 * not matter and will change when older versions stop being supported.  No
 * part of the program may depend on their exact value, all comparisons must
 * always be done with the symbols.  */
/* Names for the versions are defined in common/game.c, and must be
 * changed when the enum changes.  */
typedef enum {
	UNKNOWN_VERSION, /** Unknown version */
	V0_10, /**< Lowest supported version */
	V0_11, /**< City walls, player style, robber undo */
	V0_12, /**< Trade protocol simplified */
	V14, /**< More rules */
	V15, /**< Dice deck */
	FIRST_VERSION = V0_10,
	LATEST_VERSION = V15
} ClientVersionType;

/** Convert to a ClientVersionType.
 * @param cvt The text to analyze
 * @return The version.
 */
ClientVersionType client_version_type_from_string(const gchar * cvt);

/** Convert from a ClientVersionType.
 * @param cvt The version
 * @return The string.
 */
const gchar *client_version_type_to_string(ClientVersionType cvt);

/** Will it be possible for a client to connect to a server?
 * @param client_version The version of the client
 * @param server_version The version of the server
 * @return TRUE when the client can connect to the server
 */
gboolean can_client_connect_to_server(ClientVersionType client_version,
				      ClientVersionType server_version);

typedef struct {
	gchar *title;		/* title of the game */
	gboolean random_terrain;	/* shuffle terrain location? */
	gboolean strict_trade;	/* trade only before build/buy? */
	gboolean domestic_trade;	/* player trading allowed? */
	guint num_players;	/* number of players in the game */
	gint sevens_rule;	/* what to do when a seven is rolled */
	/* 0 = normal, 1 = no 7s on first 2 turns (official rule variant),
	 * 2 = all 7s rerolled */
	gboolean use_dice_deck;	/* use dice deck instead of dice */
	guint num_dice_decks;	/* amount of dice decks to use */
	guint num_removed_dice_cards;	/* minimum amount of dice cards before reshuffling */
	guint victory_points;	/* target number of victory points */
	gboolean check_victory_at_end_of_turn;	/* check victory only at end of turn */
	gint num_build_type[NUM_BUILD_TYPES];	/* number of each build type */
	gint resource_count;	/* number of each resource */
	guint num_develop_type[NUM_DEVEL_TYPES];	/* number of each development */
	Map *map;		/* the game map */
	gboolean parsing_map;	/* currently parsing map? *//* Not in game_params[] */
	guint tournament_time;	/* time to start tournament time in minutes *//* Not in game_params[] */
	gboolean quit_when_done;	/* server quits after someone wins *//* Not in game_params[] */
	gboolean use_pirate;	/* is there a pirate in this game? */
	GArray *island_discovery_bonus;	/* list of VPs for discovering an island */
	gchar *comments;	/* information regarding the map */
	gchar *description;	/* description of the map */
} GameParams;

typedef struct {
	gint id;		/* identification for client-server communication */
	gchar *name;		/* name of the item */
	gint points;		/* number of points */
} Points;

typedef enum {
	PARAMS_WINNABLE,	/* the game can be won */
	PARAMS_WIN_BUILD_ALL,	/* the game can be won by building all */
	PARAMS_WIN_PERHAPS,	/* the game could be won */
	PARAMS_NO_WIN		/* the game cannot be won */
} WinnableState;

typedef enum {
	PLAYER_HUMAN,		/* the player is a human */
	PLAYER_COMPUTER,	/* the player is a computer player */
	PLAYER_UNKNOWN		/* it is unknown who is controlling the player */
} PlayerType;
#define NUM_PLAYER_TYPES (PLAYER_UNKNOWN + 1)

typedef void (*WriteLineFunc) (gpointer user_data, const gchar *);

/** Default style for a player. */
const gchar *default_player_style;

GameParams *params_new(void);
GameParams *params_copy(const GameParams * params);
GameParams *params_load_file(const gchar * fname);
gboolean params_is_equal(const GameParams * params1,
			 const GameParams * params2);
void params_free(GameParams * params);
void params_write_lines(const GameParams * params,
			ClientVersionType version, gboolean write_secrets,
			WriteLineFunc func, gpointer user_data);
gboolean params_write_file(const GameParams * params, const gchar * fname);
gboolean params_load_line(GameParams * params, const gchar * line);
gboolean params_load_finish(GameParams * params);
gboolean read_line_from_file(gchar ** line, FILE * f);
/** Check whether, in theory, the game could be won by a player.
 * @param params The game parameters
 * @retval win_message A message describing how/when the game can be won
 * @retval point_specification A message describing how the points are distributed
 * @return Whether the game can be won
 */
WinnableState params_check_winnable_state(const GameParams * params,
					  gchar ** win_message,
					  gchar ** point_specification);

/** Check whether the game cannot be started.
 * @param params The game parameters
 * @return TRUE if the game cannot be started
 */
gboolean params_game_is_unstartable(const GameParams * params);

/** Determine the type of the player, by analysing the style. */
PlayerType determine_player_type(const gchar * style);

Points *points_new(gint id, const gchar * name, gint points);
void points_free(Points * points);

/* Communication format
 *
 * The commands sent to and from the server use the following
 * format specifiers:
 *	%S - string from current position to end of line
 *		this takes a gchar ** argument, in which an allocated buffer
 *		is returned.  It must be freed by the caller.
 *	%d - integer
 *	%B - build type:
 *		'road' = BUILD_ROAD
 *		'ship' = BUILD_SHIP
 *		'bridge' = BUILD_BRIDGE
 *		'settlement' = BUILD_SETTLEMENT
 *		'city' = BUILD_CITY
 *	%R - list of 5 integer resource counts:
 *		brick, grain, ore, wool, lumber
 *	%D - development card type:
 *		0 = DEVEL_ROAD_BUILDING
 *		1 = DEVEL_MONOPOLY
 *		2 = DEVEL_YEAR_OF_PLENTY
 *		3 = DEVEL_CHAPEL
 *		4 = DEVEL_UNIVERSITY
 *		5 = DEVEL_GOVERNORS_HOUSE
 *		6 = DEVEL_LIBRARY
 *		7 = DEVEL_MARKET
 *		8 = DEVEL_SOLDIER
 *	%r - resource type:
 *		'brick' = BRICK_RESOURCE
 *		'grain' = GRAIN_RESOURCE
 *		'ore' = ORE_RESOURCE
 *		'wool' = WOOL_RESOURCE
 *		'lumber' = LUMBER_RESOURCE
 */
/** Parse a line.
 * @param line Line to parse
 * @param fmt Format of the line, see communication format
 * @retval ap Result of the parse
 * @return -1 if the line could not be parsed, otherwise the offset in the line
*/
ssize_t game_vscanf(const gchar * line, const gchar * fmt, va_list ap);
/** Parse a line.
 * @param line Line to parse
 * @param fmt Format of the line, see communication format
 * @return -1 if the line could not be parsed, otherwise the offset in the line
*/
ssize_t game_scanf(const gchar * line, const gchar * fmt, ...);
/** Print a line.
 * @param fmt Format of the line, see communication format
 * @param ap Arguments to the format
 * @return A string (you must use g_free to free the string)
*/
gchar *game_vprintf(const gchar * fmt, va_list ap);
/** Print a line.
 * @param fmt Format of the line, see communication format
 * @return A string (you must use g_free to free the string)
*/
gchar *game_printf(const gchar * fmt, ...);

/** Convert a string to an array of integers.
 * @param str A comma separated list of integers
 * @return An array of integers. If the array has length zero, NULL is returned. (you must use g_array_free to free the array)
 */
GArray *build_int_list(const gchar * str);
/** Convert an array of integers to a string.
 * @param name Prefix before the list of integers
 * @param array Array of integers
 * @return A string with a comma separated list of integers and name prefixed. (you must use g_free to free the string)
*/
gchar *format_int_list(const gchar * name, GArray * array);
#endif
