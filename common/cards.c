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
#include <string.h>
#include <glib.h>

#include "game.h"
#include "cards.h"

gboolean is_victory_card(DevelType type)
{
	return type == DEVEL_CHAPEL
	    || type == DEVEL_UNIVERSITY
	    || type == DEVEL_GOVERNORS_HOUSE
	    || type == DEVEL_LIBRARY || type == DEVEL_MARKET;
}

gboolean deck_card_playable(const Deck * deck, guint num_playable_cards,
			    guint idx)
{
	if (idx >= deck_count(deck))
		return FALSE;

	if (is_victory_card(deck_get_guint(deck, idx)))
		return TRUE;

	return idx < num_playable_cards;
}

gboolean deck_card_play(Deck * deck, guint num_playable_cards, guint idx)
{
	if (!deck_card_playable(deck, num_playable_cards, idx)) {
		return FALSE;
	}
	deck_remove(deck, idx);

	return TRUE;
}

gint deck_card_amount(const Deck * deck, DevelType type)
{
	guint idx;
	gint amount = 0;

	for (idx = 0; idx < deck_count(deck); ++idx)
		if (deck_get_guint(deck, idx) == type)
			++amount;
	return amount;
}

gint deck_card_oldest_card(const Deck * deck, DevelType type)
{
	guint idx;
	for (idx = 0; idx < deck_count(deck); ++idx)
		if (deck_get_guint(deck, idx) == type)
			return idx;
	return -1;
}

const gchar *get_devel_name(DevelType type)
{
	switch (type) {
	case DEVEL_ROAD_BUILDING:
		/* Name of the development card */
		return _("Road building");
	case DEVEL_MONOPOLY:
		/* Name of the development card */
		return _("Monopoly");
	case DEVEL_YEAR_OF_PLENTY:
		/* Name of the development card */
		return _("Year of plenty");
	case DEVEL_CHAPEL:
		/* Name of the development card */
		return _("Chapel");
	case DEVEL_UNIVERSITY:
		/* Name of the development card */
		return _("Pioneer university");
	case DEVEL_GOVERNORS_HOUSE:
		/* Name of the development card */
		return _("Governor's house");
	case DEVEL_LIBRARY:
		/* Name of the development card */
		return _("Library");
	case DEVEL_MARKET:
		/* Name of the development card */
		return _("Market");
	case DEVEL_SOLDIER:
		/* Name of the development card */
		return _("Soldier");
	}
	g_assert_not_reached();
	return "";
}

const gchar *get_devel_description(DevelType type)
{
	switch (type) {
	case DEVEL_ROAD_BUILDING:
		/* Description of the 'Road Building' development card */
		return _("Build two new roads");
	case DEVEL_MONOPOLY:
		/* Description of the 'Monopoly' development card */
		return _("Select a resource type and take every card of "
			 "that type held by all other players");
	case DEVEL_YEAR_OF_PLENTY:
		/* Description of the 'Year of Plenty' development card */
		return _("Take two resource cards of any type from the "
			 "bank (cards may be of the same or different "
			 "types)");
	case DEVEL_CHAPEL:
	case DEVEL_UNIVERSITY:
	case DEVEL_GOVERNORS_HOUSE:
	case DEVEL_LIBRARY:
	case DEVEL_MARKET:
		/* Description of a development card of 1 victory point */
		return _("One victory point");
	case DEVEL_SOLDIER:
		/* Description of the 'Soldier' development card */
		return _("Move the robber to a different space and take "
			 "one resource card from another player adjacent "
			 "to that space");
	}
	g_assert_not_reached();
	return "";
}
