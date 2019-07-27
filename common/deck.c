/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2011-2013 Micah Bunting <Amnykon@gmail.com>
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
#include "deck.h"
#include "random.h"

struct _Deck {
	GPtrArray *array;
};

Deck *deck_new()
{
	Deck *deck = g_malloc0(sizeof(*deck));
	deck->array = g_ptr_array_new();
	return deck;
}

void deck_free(Deck * deck, GDestroyNotify element_free_func)
{
	if (deck == NULL) {
		return;
	}
	g_ptr_array_set_free_func(deck->array, element_free_func);
	g_ptr_array_free(deck->array, TRUE);
	g_free(deck);
}

void deck_add(Deck * deck, gpointer card)
{
	g_assert(deck != NULL);
	g_ptr_array_add(deck->array, card);
}

void deck_add_guint(Deck * deck, guint card)
{
	deck_add(deck, GUINT_TO_POINTER(card));
}

void deck_add_amount(Deck * deck, guint amount, gpointer card)
{
	guint idx;
	for (idx = 0; idx < amount; idx++) {
		deck_add(deck, card);
	}
}

void deck_add_amount_guint(Deck * deck, guint amount, guint card)
{
	deck_add_amount(deck, amount, GUINT_TO_POINTER(card));
}

gpointer deck_get(const Deck * deck, guint index)
{
	g_return_val_if_fail(deck != NULL, NULL);
	g_return_val_if_fail(index < deck_count(deck), NULL);

	return g_ptr_array_index(deck->array, index);
}

guint deck_get_guint(const Deck * deck, guint index)
{
	return GPOINTER_TO_UINT(deck_get(deck, index));
}

gpointer deck_remove(Deck * deck, guint index)
{
	g_return_val_if_fail(deck != NULL, NULL);
	g_return_val_if_fail(deck_count(deck) != 0, NULL);

	return g_ptr_array_remove_index(deck->array, index);
}

guint deck_remove_guint(Deck * deck, guint index)
{
	return GPOINTER_TO_UINT(deck_remove(deck, index));
}

gpointer deck_remove_random(Deck * deck)
{
	return deck_remove(deck, random_guint(deck_count(deck)));
}

guint deck_remove_random_guint(Deck * deck)
{
	return GPOINTER_TO_UINT(deck_remove_random(deck));
}

guint deck_count(const Deck * deck)
{
	g_assert(deck != NULL);
	return deck->array->len;
}

void deck_combine(Deck * source, Deck * destination)
{
	g_assert(source != NULL);
	g_assert(destination != NULL);

	while (deck_count(source) != 0) {
		deck_add(destination, deck_remove(source, 0));
	}
}
