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

#ifndef __buildrec_h
#define __buildrec_h
#include "map.h"

/** information about building.  Used for undo */
typedef struct {
	BuildType type;	       /**< type of building performed */
	int x;		       /**< x-pos of hex of build action */
	int y;		       /**< x-pos of hex of build action */
	int pos;	       /**< location on hex of build action */
	BuildType prev_status; /**< build city without settlement only: previous status of node */
	const gint *cost;      /**< resources spent */
	int prev_x;	       /**< moving ships only: previous x hex */
	int prev_y;	       /**< moving ships only: previous y hex */
	int prev_pos;	       /**< moving ships only: previous position */
	/* this is an int, because only the server uses it, and the client
	 * doesn't know about Players. */
	int longest_road;      /**< who had the longest road before this build */
	int special_points_id; /**< Id of the special points or -1 */
} BuildRec;

int buildrec_count_type(GList * list, BuildType type);
BuildRec *buildrec_get(GList * list, BuildType type, gint idx);
BuildRec *buildrec_get_edge(GList * list, gint idx);
BuildRec *buildrec_new(BuildType type, gint x, gint y, gint pos);
GList *buildrec_free(GList * list);
gboolean buildrec_is_valid(GList * list, const Map * map, gint owner);
gboolean buildrec_can_setup_road(GList * list, const Edge * edge,
				 gboolean is_double);
gboolean buildrec_can_setup_ship(GList * list, const Edge * edge,
				 gboolean is_double);
gboolean buildrec_can_setup_settlement(GList * list, const Node * node,
				       gboolean is_double);
gboolean buildrec_can_setup_bridge(GList * list, const Edge * edge,
				   gboolean is_double);
gint buildrec_count_edges(GList * list);

#endif
