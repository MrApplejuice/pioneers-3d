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

#ifndef __cards_h
#define __cards_h

#include "deck.h"
#include "game.h"

gboolean is_victory_card(DevelType type);
const gchar *get_devel_name(DevelType type);
const gchar *get_devel_description(DevelType description);

gboolean deck_card_playable(const Deck * deck, guint num_playable_cards,
			    guint idx);
gboolean deck_card_play(Deck * deck, guint num_playable_cards, guint idx);

gint deck_card_amount(const Deck * deck, DevelType type);
gint deck_card_oldest_card(const Deck * deck, DevelType type);

#endif
