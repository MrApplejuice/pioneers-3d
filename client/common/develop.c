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
#include "cost.h"
#include "log.h"
#include "state.h"
#include "callback.h"

static gboolean bought_develop;	/* have we bought a development card? */
static guint num_playable_cards;	/* number of playable development cards */

static gboolean is_unique[NUM_DEVEL_TYPES];	/* is each card unique? */

static Deck *develop_deck;	/* our deck of development cards */

void develop_init(void)
{
	int idx;
	if (develop_deck != NULL)
		deck_free(develop_deck, NULL);
	develop_deck = deck_new();
	num_playable_cards = 0;
	for (idx = 0; idx < NUM_DEVEL_TYPES; idx++)
		is_unique[idx] = game_params->num_develop_type[idx] == 1;
}

void develop_bought_card_turn(DevelType type, gboolean bought_this_turn)
{
	deck_add_guint(develop_deck, type);
	if (bought_this_turn) {
		/* Cannot undo build after buying a development card
		 */
		build_clear();
		bought_develop = TRUE;
		/* Only log if the cards is bought in the current turn.
		 * This function is also called during reconnect
		 */
		if (is_unique[type])
			log_message(MSG_DEVCARD,
				    /* This development card is unique */
				    _(""
				      "You bought the %s development card.\n"),
				    get_devel_name(type));
		else
			log_message(MSG_DEVCARD,
				    /* This development card is not unique */
				    _(""
				      "You bought a %s development card.\n"),
				    get_devel_name(type));
	} else {
		num_playable_cards++;
	}
	player_modify_statistic(my_player_num(), STAT_DEVELOPMENT, 1);
	stock_use_develop();
	callbacks.bought_develop(type);
}

void develop_bought_card(DevelType type)
{
	develop_bought_card_turn(type, TRUE);
}

guint get_num_playable_cards(void)
{
	return num_playable_cards;
}

void develop_reset_have_played_bought(gboolean have_bought,
				      guint number_playable_cards)
{
	bought_develop = have_bought;
	num_playable_cards = number_playable_cards;
}

void develop_bought(gint player_num)
{
	log_message(MSG_DEVCARD, _("%s bought a development card.\n"),
		    player_name(player_num, TRUE));

	player_modify_statistic(player_num, STAT_DEVELOPMENT, 1);
	stock_use_develop();
}

void develop_played(gint player_num, guint card_idx, DevelType type)
{
	if (player_num == my_player_num()) {
		deck_card_play(develop_deck, num_playable_cards, card_idx);
		if (!is_victory_card(type))
			num_playable_cards = 0;
	}
	callbacks.played_develop(player_num, card_idx, type);

	if (is_unique[type])
		log_message(MSG_DEVCARD,
			    _("%s played the %s development card.\n"),
			    player_name(player_num, TRUE),
			    get_devel_name(type));
	else
		log_message(MSG_DEVCARD,
			    _("%s played a %s development card.\n"),
			    player_name(player_num, TRUE),
			    get_devel_name(type));

	player_modify_statistic(player_num, STAT_DEVELOPMENT, -1);
	switch (type) {
	case DEVEL_ROAD_BUILDING:
		if (player_num == my_player_num()) {
			if (stock_num_roads() == 0
			    && stock_num_ships() == 0
			    && stock_num_bridges() == 0)
				log_message(MSG_INFO,
					    _(""
					      "You have run out of road segments.\n"));
		}
		break;
	case DEVEL_CHAPEL:
		player_modify_statistic(player_num, STAT_CHAPEL, 1);
		break;
	case DEVEL_UNIVERSITY:
		player_modify_statistic(player_num, STAT_UNIVERSITY, 1);
		break;
	case DEVEL_GOVERNORS_HOUSE:
		player_modify_statistic(player_num, STAT_GOVERNORS_HOUSE,
					1);
		break;
	case DEVEL_LIBRARY:
		player_modify_statistic(player_num, STAT_LIBRARY, 1);
		break;
	case DEVEL_MARKET:
		player_modify_statistic(player_num, STAT_MARKET, 1);
		break;
	case DEVEL_SOLDIER:
		player_modify_statistic(player_num, STAT_SOLDIERS, 1);
		break;
	default:
		break;
	}
}

void monopoly_player(gint player_num, gint victim_num, gint num,
		     Resource type)
{
	gchar *buf;
	gchar *tmp;

	player_modify_statistic(player_num, STAT_RESOURCES, num);
	player_modify_statistic(victim_num, STAT_RESOURCES, -num);

	buf = resource_cards(num, type);
	if (player_num == my_player_num()) {
		/* I get the cards */
		/* $1=resources, $2=player that loses resources */
		log_message(MSG_STEAL, _("You get %s from %s.\n"),
			    buf, player_name(victim_num, FALSE));
		resource_modify(type, num);
	} else if (victim_num == my_player_num()) {
		/* I lose the cards */
		/* $1=player that steals, $2=resources */
		log_message(MSG_STEAL, _("%s took %s from you.\n"),
			    player_name(player_num, TRUE), buf);
		resource_modify(type, -num);
	} else {
		/* I'm a bystander */
		/* $1=player that steals, $2=resources, $3=player that loses resources */
		tmp = g_strdup(player_name(player_num, TRUE));
		log_message(MSG_STEAL, _("%s took %s from %s.\n"),
			    tmp, buf, player_name(victim_num, FALSE));
		g_free(tmp);
	}
	g_free(buf);
}

void develop_begin_turn(void)
{
	bought_develop = FALSE;
	num_playable_cards = deck_count(develop_deck);
}

gboolean can_play_develop(guint card)
{
	if (!deck_card_playable(develop_deck, num_playable_cards, card))
		return FALSE;
	if (deck_get_guint(develop_deck, card) == DEVEL_ROAD_BUILDING
	    && !road_building_can_build_road()
	    && !road_building_can_build_ship()
	    && !road_building_can_build_bridge())
		return FALSE;

	return TRUE;
}

gboolean can_play_any_develop(void)
{
	guint i;
	for (i = 0; i < deck_count(develop_deck); ++i)
		if (can_play_develop(i))
			return TRUE;
	return FALSE;
}

gboolean can_buy_develop(void)
{
	return have_rolled_dice()
	    && stock_num_develop() > 0 && can_afford(cost_development());
}

gboolean have_bought_develop(void)
{
	return bought_develop;
}

const Deck *get_devel_deck(void)
{
	return develop_deck;
}
