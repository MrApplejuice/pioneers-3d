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

/* Implementation of a Set, using a Hash. Based on the glib documentation for HashTables */

#ifndef include_set_h
#define include_set_h

#include <glib.h>

/** An unordered collection of elements.
 * Each element occurs only once.
 */
typedef struct _Set Set;

/** Create a new Set.
 * @memberof _Set
 * @param hash_func a function to create a hash value from the element
 * @param equal_func a function to check two elements for equality
 * @param destroy a function to free the memory allocated for the element,
 *   or NULL if you don't want to supply such a function.
 * @return a new Set. Free this Set with set_free
 */
Set *set_new(GHashFunc hash_func, GEqualFunc equal_func,
	     GDestroyNotify destroy);

/** Add a new element to the Set.
 * @memberof _Set
 * @param set a Set
 * @param element the element to add
 */
void set_add(Set * set, gpointer element);

/** Check whether an element is in the Set.
 * @memberof _Set
 * @param set a Set
 * @param element the element to check
 * @return TRUE if the element is in the Set
 */
gboolean set_contains(Set * set, gpointer element);

/** Remove an element from the Set.
 * @memberof _Set
 * @param set a Set
 * @param element the element to remove
 * @return TRUE if the element was in the Set
 */
gboolean set_remove(Set * set, gpointer element);

/** Returns the number of elements in the Set.
 * @memberof _Set
 * @param set a Set
 * @return The number of elements in the Set.
 */
guint set_size(Set * set);

/** Free the memory associated with the Set.
 * @memberof _Set
 * @param set a Set
 */
void set_free(Set * set);

typedef void (*SetForEachFunc) (gpointer element, gpointer user_data);

/** Iterate through all elements in the Set.
 * @memberof _Set
 * @param set a Set
 * @param func a function to call for each element
 * @param user_data the argument to the function func
 */
void set_foreach(Set * set, SetForEachFunc func, gpointer user_data);
#endif
