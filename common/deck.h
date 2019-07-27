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

/** @file  deck.h
 * A deck that contains cards. Cards can be pointers or guints. <BR>
 * <BR>
 * To add a card to the deck, use deck_add(), or deck_add_guint(). <BR>
 * To add multiple of cards of the same type to the deck, use deck_add_amount(), or deck_add_amount_guint(). <BR>
 * To look at a specific card in the deck, use deck_get(). <BR>
 * To remove or draw a specific card from the deck use deck_remove(), or deck_remove_guint(). <BR>
 * To remove or draw a random card from the deck, use deck_remove_random(), or deck_remove_random_guint(). <BR>
 * To access the amount of cards in the deck, use deck_count(). <BR>
 * To move all cards from one deck to another, use deck_combine().
 */

#ifndef __deck_h
#define __deck_h

#include <glib.h>
/** A deck that contains cards.
 */
typedef struct _Deck Deck;

/** Creates a new empty Deck.
 * @return The newly allocated Deck.
 */
Deck *deck_new(void);

/** Frees the memory allocated for the Deck.
 * @param deck The Deck to free.
 * @param element_free_func The function to free all of the cards in the deck.
 */
void deck_free(Deck * deck, GDestroyNotify element_free_func);

/** Add single card to the deck.
 @param deck The deck to add a card to.
 @param card The card to add to the deck.
 */
void deck_add(Deck * deck, gpointer card);

/** Add single card to the deck.
 @param deck The deck to add a card to.
 @param card The card to add to the deck.
 */
void deck_add_guint(Deck * deck, guint card);

/** Add multiple cards to the deck.
 @param deck The deck to add a card to.
 @param amount The amount of cards to add.
 @param card The cards to add to the deck.
 */
void deck_add_amount(Deck * deck, guint amount, gpointer card);

/** Add multiple cards to the deck.
 @param deck The deck to add a card to.
 @param amount The amount of cards to add.
 @param card The cards to add to the deck.
 */
void deck_add_amount_guint(Deck * deck, guint amount, guint card);

/** Gets the card at position index in the deck.
 * @param deck The Deck containing the card.
 * @param index The position of the card in the deck.
 * @return The card in the deck at position index.
 */
gpointer deck_get(const Deck * deck, guint index);

/** Gets the card at position index in the deck.
 * @param deck The Deck containing the card.
 * @param index The position of the card in the deck.
 * @return The card in the deck at position index.
 */
guint deck_get_guint(const Deck * deck, guint index);

/** Removes a specific card from the deck.
 * @param deck The Deck to remove card from.
 * @param index The position of the card in the deck.
 * @return The card that was removed from the deck.
 */
gpointer deck_remove(Deck * deck, guint index);

/** Removes a specific card from the deck.
 * @param deck The Deck to remove card from.
 * @param index The position of the card in the deck.
 * @return The card that was removed from the deck.
 */
guint deck_remove_guint(Deck * deck, guint index);

/** Removes a random card from the deck
 * @param deck The Deck to remove card from.
 * @return The card that was removed from the deck.
 */
gpointer deck_remove_random(Deck * deck);

/** Removes a random card from the deck
 * @param deck The Deck to remove card from.
 * @return The card that was removed from the deck.
 */
guint deck_remove_random_guint(Deck * deck);

/** Gets the number of cards in a deck.
 * @param deck The Deck to return the count of.
 * @return The number of the cards in the deck.
 */
guint deck_count(const Deck * deck);

/** Moves all cards in source Deck to destination Deck.
 * @param source The source Deck.
 * @param destination The destination Deck.
 */
void deck_combine(Deck * source, Deck * destination);

#endif
