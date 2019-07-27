/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2013 Roland Clobus <rclobus@rclobus.nl>
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
#include "set.h"

struct _Set {
	GHashTable *hash; /**< @private A Set is implemented by a hash table */
};

Set *set_new(GHashFunc hash_func, GEqualFunc equal_func,
	     GDestroyNotify destroy)
{
	Set *set = g_malloc0(sizeof(*set));
	set->hash =
	    g_hash_table_new_full(hash_func, equal_func, destroy, NULL);
	return set;
}

void set_add(Set * set, gpointer element)
{
	g_hash_table_replace(set->hash, element, element);
}

gboolean set_contains(Set * set, gpointer element)
{
	return g_hash_table_lookup_extended(set->hash, element, NULL,
					    NULL);
}

gboolean set_remove(Set * set, gpointer element)
{
	return g_hash_table_remove(set->hash, element);
}

guint set_size(Set * set)
{
	return g_hash_table_size(set->hash);
}

void set_free(Set * set)
{
	g_hash_table_destroy(set->hash);
	g_free(set);
}

/** Data for the g_hash_table_foreach function */
struct SetForEachData {
	SetForEachFunc func; /**< The function to call */
	gpointer user_data; /**< The argument to that function */
};

/** Invoke the foreach function.
 * @private @memberof _Set
 */
static void set_foreach_func(gpointer key, G_GNUC_UNUSED gpointer value,
			     gpointer user_data)
{
	struct SetForEachData *data = user_data;

	data->func(key, data->user_data);
}

void set_foreach(Set * set, SetForEachFunc func, gpointer user_data)
{
	struct SetForEachData data;

	data.func = func;
	data.user_data = user_data;

	g_hash_table_foreach(set->hash, set_foreach_func, &data);
}
