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
#include <math.h>
#include <ctype.h>

#include "game.h"
#include "cards.h"
#include "map.h"
#include "client.h"
#include "log.h"
#include "buildrec.h"

static GList *build_list;
static gboolean built;		/* have we build road / settlement / city? */
static gint num_edges, num_settlements;

void build_clear(void)
{
	build_list = buildrec_free(build_list);
	num_edges = num_settlements = 0;
}

void build_new_turn(void)
{
	build_list = buildrec_free(build_list);
	built = FALSE;
}

void build_remove(BuildType type, gint x, gint y, gint pos)
{
	GList *list;
	BuildRec *rec;

	g_assert(build_list != NULL);

	list = g_list_last(build_list);
	rec = list->data;
	build_list = g_list_remove(build_list, rec);
	g_assert(rec->type == type
		 && rec->x == x && rec->y == y && rec->pos == pos);
	g_free(rec);

	switch (type) {
	case BUILD_SETTLEMENT:
		--num_settlements;
		break;
	case BUILD_ROAD:
	case BUILD_SHIP:
	case BUILD_BRIDGE:
		--num_edges;
		break;
	default:
		break;
	}

	/* If the build_list is now empty (no more items to undo), clear built flag
	   so trading is reallowed with strict-trade */
	if (build_list == NULL)
		built = FALSE;

	player_build_remove(my_player_num(), type, x, y, pos);
}

/* Move a ship */
void build_move(gint sx, gint sy, gint spos, gint dx, gint dy, gint dpos,
		gint isundo)
{
	GList *list;
	BuildRec *rec;
	if (isundo) {
		callbacks.get_map()->has_moved_ship = FALSE;
		list = g_list_last(build_list);
		rec = list->data;
		if (rec->type != BUILD_MOVE_SHIP && rec->x != sx
		    && rec->y != sy && rec->pos != spos) {
			log_message(MSG_ERROR,
				    "undo ship move mismatch: %d<->%d %d<->%d %d<->%d %d<->%d\n",
				    BUILD_MOVE_SHIP, rec->type, sx, rec->x,
				    sy, rec->y, spos, rec->pos);
		}
		build_list = g_list_remove(build_list, rec);
		g_free(rec);
		/* If the build_list is now empty (no more items to undo),
		 * clear built flag so trading is reallowed with
		 * strict-trade */
		if (build_list == NULL)
			built = FALSE;
	} else {
		rec = buildrec_new(BUILD_MOVE_SHIP, sx, sy, spos);
		build_list = g_list_append(build_list, rec);
		built = TRUE;
		callbacks.get_map()->has_moved_ship = TRUE;
	}
	player_build_move(my_player_num(), sx, sy, spos, dx, dy, dpos,
			  isundo);
}

void build_add(BuildType type, gint x, gint y, gint pos, gboolean newbuild)
{
	BuildRec *rec = buildrec_new(type, x, y, pos);
	build_list = g_list_append(build_list, rec);
	built = TRUE;

	switch (type) {
	case BUILD_SETTLEMENT:
		++num_settlements;
		break;
	case BUILD_ROAD:
	case BUILD_SHIP:
	case BUILD_BRIDGE:
		++num_edges;
		break;
	default:
		break;
	}

	if (newbuild) {
		player_build_add(my_player_num(), type, x, y, pos, TRUE);
	}
}

gint build_count_edges(void)
{
	return num_edges;
}

gint build_count_settlements(void)
{
	return num_settlements;
}

gint build_count(BuildType type)
{
	return buildrec_count_type(build_list, type);
}

gboolean build_is_valid(void)
{
	return buildrec_is_valid(build_list, callbacks.get_map(),
				 my_player_num());
}

gboolean build_can_undo(void)
{
	return build_list != NULL;
}

gboolean have_built(void)
{
	return built;
}

/* Place some restrictions on road placement during setup phase
 */
gboolean build_can_setup_road(const Edge * edge, gboolean double_setup)
{
	return buildrec_can_setup_road(build_list, edge, double_setup);
}

/* Place some restrictions on ship placement during setup phase
 */
gboolean build_can_setup_ship(const Edge * edge, gboolean double_setup)
{
	return buildrec_can_setup_ship(build_list, edge, double_setup);
}

/* Place some restrictions on bridge placement during setup phase
 */
gboolean build_can_setup_bridge(const Edge * edge, gboolean double_setup)
{
	return buildrec_can_setup_bridge(build_list, edge, double_setup);
}

/* Place some restrictions on road placement during setup phase
 */
gboolean build_can_setup_settlement(const Node * node,
				    gboolean double_setup)
{
	return buildrec_can_setup_settlement(build_list, node,
					     double_setup);
}
