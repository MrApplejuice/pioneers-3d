/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2005,2010 Roland Clobus <rclobus@rclobus.nl>
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
#include "ai.h"
#include "cost.h"
#include <stdio.h>
#include <stdlib.h>
/*
 * This is a rudimentary AI for Pioneers.
 *
 * What it does _NOT_ do:
 *
 * -Make roads explicitly to get the longest road card
 * -Initiate trade with other players
 * -Do anything seafarers
 *
 */

typedef struct resource_values_s {
	float value[NO_RESOURCE];
	MaritimeInfo info;
	gint ports[NO_RESOURCE];
} resource_values_t;

static int quote_num;

/* things we can buy, in the order that we want them. */
typedef enum {
	BUY_CITY,
	BUY_SETTLEMENT,
	BUY_ROAD,
	BUY_DEVEL_CARD,
	BUY_LAST
} BuyType;

/*
 * Forward declarations
 */
static Edge *best_road_to_road_spot(Node * n, float *score,
				    const resource_values_t * resval);

static Edge *best_road_to_road(const resource_values_t * resval);

static Edge *best_road_spot(const resource_values_t * resval);

static Node *best_city_spot(const resource_values_t * resval);

static Node *best_settlement_spot(gboolean during_setup,
				  const resource_values_t * resval);

static int places_can_build_settlement(void);
static gint determine_monopoly_resource(void);

/*
 * Functions to keep track of what nodes we've visited
 */

typedef struct node_seen_set_s {

	Node *seen[MAP_SIZE * MAP_SIZE];
	int size;

} node_seen_set_t;

static void nodeset_reset(node_seen_set_t * set)
{
	set->size = 0;
}

static void nodeset_set(node_seen_set_t * set, Node * n)
{
	int i;

	for (i = 0; i < set->size; i++)
		if (set->seen[i] == n)
			return;

	set->seen[set->size] = n;
	set->size++;
}

static int nodeset_isset(node_seen_set_t * set, Node * n)
{
	int i;

	for (i = 0; i < set->size; i++)
		if (set->seen[i] == n)
			return 1;

	return 0;
}

typedef void iterate_node_func_t(Node * n, void *rock);

/*
 * Iterate over all the nodes on the map calling func() with 'rock'
 *
 */
static void for_each_node(iterate_node_func_t * func, void *rock)
{
	Map *map;
	int i, j, k;

	map = callbacks.get_map();
	for (i = 0; i < map->x_size; i++) {
		for (j = 0; j < map->y_size; j++) {
			for (k = 0; k < 6; k++) {
				Node *n = map_node(map, i, j, k);

				if (n)
					func(n, rock);
			}
		}
	}

}

/** Determine the required resources.
 *  @param assets The resources that are available
 *  @param cost   The cost to buy something
 *  @retval need  The additional resources required to buy this
 *  @return TRUE if the assets are enough
 */
static gboolean can_pay_for(const gint assets[NO_RESOURCE],
			    const gint cost[NO_RESOURCE],
			    gint need[NO_RESOURCE])
{
	gint i;
	gboolean have_enough;

	have_enough = TRUE;
	for (i = 0; i < NO_RESOURCE; i++) {
		if (assets[i] >= cost[i])
			need[i] = 0;
		else {
			need[i] = cost[i] - assets[i];
			have_enough = FALSE;
		}
	}
	return have_enough;
}

/* How much does this cost to build? */
static const gint *cost_of(BuyType bt)
{
	switch (bt) {
	case BUY_CITY:
		return cost_upgrade_settlement();
	case BUY_SETTLEMENT:
		return cost_settlement();
	case BUY_ROAD:
		return cost_road();
	case BUY_DEVEL_CARD:
		return cost_development();
	default:
		g_assert(0);
		return NULL;
	}
}

/*
 * Do I have the resources to buy this, is it available, and do I want it?
 */
static gboolean should_buy(const gint assets[NO_RESOURCE], BuyType bt,
			   const resource_values_t * resval,
			   gint need[NO_RESOURCE])
{
	if (!can_pay_for(assets, cost_of(bt), need))
		return FALSE;

	switch (bt) {
	case BUY_CITY:
		return (stock_num_cities() > 0 &&
			best_city_spot(resval) != NULL);
	case BUY_SETTLEMENT:
		return (stock_num_settlements() > 0 &&
			best_settlement_spot(FALSE, resval) != NULL);
	case BUY_ROAD:
		/* don't sprawl :) */
		return (stock_num_roads() > 0 &&
			places_can_build_settlement() <= 2 &&
			(best_road_spot(resval) != NULL ||
			 best_road_to_road(resval) != NULL));
	case BUY_DEVEL_CARD:
		return (stock_num_develop() > 0 && can_buy_develop());
	default:
		/* xxx bridge, ship */
		return FALSE;
	}
}

/*
 * If I buy this, what will I have left?  Note that some values in need[]
 * can be negative if I can't afford it.
 */
static void leftover_resources(const gint assets[NO_RESOURCE],
			       BuyType bt, gint need[NO_RESOURCE])
{
	gint i;
	const gint *cost = cost_of(bt);

	for (i = 0; i < NO_RESOURCE; i++)
		need[i] = assets[i] - cost[i];
}

/*
 * Probability of a dice roll
 */

static float dice_prob(gint roll)
{
	switch (roll) {
	case 2:
	case 12:
		return 3;
	case 3:
	case 11:
		return 6;
	case 4:
	case 10:
		return 8;
	case 5:
	case 9:
		return 11;
	case 6:
	case 8:
		return 14;
	default:
		return 0;
	}
}

/*
 * By default how valuable is this terrain?
 */

static float default_score_terrain(Terrain terrain)
{
	float score;

	switch (terrain) {
	case GOLD_TERRAIN:	/* gold */
		score = 1.25f;
		break;
	case HILL_TERRAIN:	/* brick */
		score = 1.1f;
		break;
	case FIELD_TERRAIN:	/* grain */
		score = 1.0f;
		break;
	case MOUNTAIN_TERRAIN:	/* rock */
		score = 1.05f;
		break;
	case PASTURE_TERRAIN:	/* sheep */
		score = 1.0f;
		break;
	case FOREST_TERRAIN:	/* wood */
		score = 1.1f;
		break;
	case DESERT_TERRAIN:
	case SEA_TERRAIN:
	default:
		score = 0.0f;
		break;
	}

	return score;
}

/* For each node I own see how much i produce with it. keep a
 * tally with 'produce'
 */

static void reevaluate_iterator(Node * n, void *rock)
{
	float *produce = (float *) rock;

	/* if i own this node */
	if ((n) && (n->owner == my_player_num())) {
		int l;
		for (l = 0; l < 3; l++) {
			Hex *h = n->hexes[l];
			float mult = 1.0;

			if (n->type == BUILD_CITY)
				mult = 2.0;

			if (h && h->terrain < DESERT_TERRAIN) {
				produce[h->terrain] +=
				    mult *
				    default_score_terrain(h->terrain) *
				    dice_prob(h->roll);
			}

		}
	}

}

/*
 * Reevaluate the value of all the resources to us
 */

static void reevaluate_resources(resource_values_t * outval)
{
	float produce[NO_RESOURCE];
	int i;

	for (i = 0; i < NO_RESOURCE; i++) {
		produce[i] = 0;
	}

	for_each_node(&reevaluate_iterator, (void *) produce);

	/* Now invert all the positive numbers and give any zeros a weight of 2
	 *
	 */
	for (i = 0; i < NO_RESOURCE; i++) {
		if (produce[i] == 0) {
			outval->value[i] =
			    default_score_terrain(resource_to_terrain(i));
		} else {
			outval->value[i] = 1.0f / produce[i];
		}

	}

	/*
	 * Save the maritime info too so we know if we can do port trades
	 */
	map_maritime_info(callbacks.get_map(), &outval->info,
			  my_player_num());

	for (i = 0; i < NO_RESOURCE; i++) {
		if (outval->info.specific_resource[i])
			outval->ports[i] = 2;
		else if (outval->info.any_resource)
			outval->ports[i] = 3;
		else
			outval->ports[i] = 4;
	}
}


/*
 *
 */
static float resource_value(Resource resource,
			    const resource_values_t * resval)
{
	if (resource < NO_RESOURCE)
		return resval->value[resource];
	else if (resource == GOLD_RESOURCE)
		return default_score_terrain(GOLD_TERRAIN);
	else
		return 0.0;
}


/*
 * How valuable is this hex to me?
 */
static float score_hex(Hex * hex, const resource_values_t * resval)
{
	float score;

	if (hex == NULL)
		return 0;

	/* multiple resource value by dice probability */
	score =
	    resource_value(terrain_to_resource(hex->terrain),
			   resval) * dice_prob(hex->roll);

	/* if we don't have a 3 for 1 port yet and this is one it's valuable! */
	if (!resval->info.any_resource) {
		if (hex->resource == ANY_RESOURCE)
			score += 0.5f;
	}

	return score;
}

/*
 * How valuable is this hex to others
 */
static float default_score_hex(Hex * hex)
{
	float score;

	if (hex == NULL)
		return 0;

	/* multiple resource value by dice probability */
	score = default_score_terrain(hex->terrain) * dice_prob(hex->roll);

	return score;
}

/*
 * Give a numerical score to how valuable putting a settlement/city on this spot is
 *
 */
static float score_node(const Node * node, gboolean city,
			const resource_values_t * resval)
{
	int i;
	float score = 0;

	/* if not a node, how did this happen? */
	g_assert(node != NULL);

	/* if already occupied, in water, or too close to others  give a score of -1 */
	if (is_node_on_land(node) == FALSE)
		return -1;
	if (is_node_spacing_ok(node) == FALSE)
		return -1;
	if (!city) {
		if (node->owner != -1)
			return -1;
	}

	for (i = 0; i < 3; i++) {
		score += score_hex(node->hexes[i], resval);
	}

	return score;
}

/*
 * Road connects here
 */
static int road_connects(Node * n)
{
	int i;

	if (n == NULL)
		return 0;

	for (i = 0; i < 3; i++) {
		Edge *e = n->edges[i];

		if ((e) && (e->owner == my_player_num()))
			return 1;
	}

	return 0;
}


/** Find the best spot for a settlement
 * @param during_setup Build a settlement during the setup phase?
 *                     During setup: no connected road is required,
 *                                   and the no_setup must be taken into account
 *                     Normal play:  settlement must be next to a road we own.
 */
static Node *best_settlement_spot(gboolean during_setup,
				  const resource_values_t * resval)
{
	int i, j, k;
	Node *best = NULL;
	float bestscore = -1.0;
	float score;
	Map *map = callbacks.get_map();

	for (i = 0; i < map->x_size; i++) {
		for (j = 0; j < map->y_size; j++) {
			for (k = 0; k < 6; k++) {
				Node *n = map_node(map, i, j, k);
				if (!n)
					continue;
				if (during_setup) {
					if (n->no_setup)
						continue;
				} else {
					if (!road_connects(n))
						continue;
				}

				score = score_node(n, FALSE, resval);
				if (score > bestscore) {
					best = n;
					bestscore = score;
				}
			}

		}
	}

	return best;
}


/*
 * What is the best settlement to upgrade to a city?
 *
 */
static Node *best_city_spot(const resource_values_t * resval)
{
	int i, j, k;
	Node *best = NULL;
	float bestscore = -1.0;
	Map *map = callbacks.get_map();

	for (i = 0; i < map->x_size; i++) {
		for (j = 0; j < map->y_size; j++) {
			for (k = 0; k < 6; k++) {
				Node *n = map_node(map, i, j, k);
				if (!n)
					continue;
				if ((n->owner == my_player_num())
				    && (n->type == BUILD_SETTLEMENT)) {
					float score =
					    score_node(n, TRUE, resval);

					if (score > bestscore) {
						best = n;
						bestscore = score;
					}
				}
			}

		}
	}

	return best;
}

/*
 * Return the opposite end of this node, edge
 *
 */
static Node *other_node(Edge * e, Node * n)
{
	if (e->nodes[0] == n)
		return e->nodes[1];
	else
		return e->nodes[0];
}

/*
 *
 *
 */
static Edge *traverse_out(Node * n, node_seen_set_t * set, float *score,
			  const resource_values_t * resval)
{
	float bscore = 0.0;
	Edge *best = NULL;
	int i;

	/* mark this node as seen */
	nodeset_set(set, n);

	for (i = 0; i < 3; i++) {
		Edge *e = n->edges[i];
		Edge *cur_e = NULL;
		Node *othernode;
		float cur_score;

		if (!e)
			continue;

		othernode = other_node(e, n);
		g_assert(othernode != NULL);

		/* if our road traverse it */
		if (e->owner == my_player_num()) {

			if (!nodeset_isset(set, othernode))
				cur_e =
				    traverse_out(othernode, set,
						 &cur_score, resval);

		} else if (can_road_be_built(e, my_player_num())) {

			/* no owner, how good is the other node ? */
			cur_e = e;

			cur_score = score_node(othernode, FALSE, resval);

			/* umm.. can we build here? */
			/*if (!can_settlement_be_built(othernode, my_player_num ()))
			   cur_e = NULL;       */
		}

		/* is this the best edge we've seen? */
		if ((cur_e != NULL) && (cur_score > bscore)) {
			best = cur_e;
			bscore = cur_score;

		}
	}

	*score = bscore;
	return best;
}

/*
 * Best road to a road
 *
 */
static Edge *best_road_to_road_spot(Node * n, float *score,
				    const resource_values_t * resval)
{
	float bscore = -1.0;
	Edge *best = NULL;
	int i, j;

	for (i = 0; i < 3; i++) {
		Edge *e = n->edges[i];
		if (e) {
			Node *othernode = other_node(e, n);

			if (can_road_be_built(e, my_player_num())) {

				for (j = 0; j < 3; j++) {
					Edge *e2 = othernode->edges[j];
					if (e2 == NULL)
						continue;

					/* We need to look further, temporarily mark this edge as having our road on it. */
					e->owner = my_player_num();
					e->type = BUILD_ROAD;

					if (can_road_be_built
					    (e2, my_player_num())) {
						float nscore =
						    score_node(other_node
							       (e2,
								othernode),
							       FALSE,
							       resval);

						if (nscore > bscore) {
							bscore = nscore;
							best = e;
						}
					}
					/* restore map to its real state */
					e->owner = -1;
					e->type = BUILD_NONE;
				}
			}

		}
	}

	*score = bscore;
	return best;
}

/*
 * Best road to road on whole map
 *
 */
static Edge *best_road_to_road(const resource_values_t * resval)
{
	int i, j, k;
	Edge *best = NULL;
	float bestscore = -1.0;
	Map *map = callbacks.get_map();

	for (i = 0; i < map->x_size; i++) {
		for (j = 0; j < map->y_size; j++) {
			for (k = 0; k < 6; k++) {
				Node *n = map_node(map, i, j, k);
				Edge *e;
				float score;

				if ((n) && (n->owner == my_player_num())) {
					e = best_road_to_road_spot(n,
								   &score,
								   resval);
					if (score > bestscore) {
						best = e;
						bestscore = score;
					}
				}
			}
		}
	}

	return best;
}

/*
 * Best road spot
 *
 */
static Edge *best_road_spot(const resource_values_t * resval)
{
	int i, j, k;
	Edge *best = NULL;
	float bestscore = -1.0;
	node_seen_set_t nodeseen;
	Map *map = callbacks.get_map();

	/*
	 * For every node that we're the owner of traverse out to find the best
	 * node we're one road away from and build that road
	 *
	 *
	 * xxx loops
	 */

	for (i = 0; i < map->x_size; i++) {
		for (j = 0; j < map->y_size; j++) {
			for (k = 0; k < 6; k++) {
				Node *n = map_node(map, i, j, k);

				if ((n != NULL)
				    && (n->owner == my_player_num())) {
					float score = -1.0;
					Edge *e;

					nodeset_reset(&nodeseen);

					e = traverse_out(n, &nodeseen,
							 &score, resval);

					if (score > bestscore) {
						best = e;
						bestscore = score;
					}
				}
			}

		}
	}

	return best;
}


/*
 * Any road at all that's valid for us to build
 */

static void rand_road_iterator(Node * n, void *rock)
{
	int i;
	Edge **out = (Edge **) rock;

	if (n == NULL)
		return;

	for (i = 0; i < 3; i++) {
		Edge *e = n->edges[i];

		if ((e) && (can_road_be_built(e, my_player_num())))
			*out = e;
	}
}

/*
 * Find any road we can legally build
 *
 */
static Edge *find_random_road(void)
{
	Edge *e = NULL;

	for_each_node(&rand_road_iterator, &e);

	return e;
}


static void places_can_build_iterator(Node * n, void *rock)
{
	int *count = (int *) rock;

	if (can_settlement_be_built(n, my_player_num()))
		(*count)++;
}

static int places_can_build_settlement(void)
{
	int count = 0;

	for_each_node(&places_can_build_iterator, (void *) &count);

	return count;
}

/*
 * How many resource cards does player have?
 *
 */
static int num_assets(gint assets[NO_RESOURCE])
{
	int i;
	int count = 0;

	for (i = 0; i < NO_RESOURCE; i++) {
		count += assets[i];
	}

	return count;
}

static int player_get_num_resource(int player)
{
	return player_get(player)->statistics[STAT_RESOURCES];
}

/*
 * Does this resource list contain one element? If so return it
 * otherwise return NO_RESOURCE
 */
static int which_one(gint assets[NO_RESOURCE])
{
	int i;
	int res = NO_RESOURCE;
	int tot = 0;

	for (i = 0; i < NO_RESOURCE; i++) {

		if (assets[i] > 0) {
			tot += assets[i];
			res = i;
		}
	}

	if (tot == 1)
		return res;

	return NO_RESOURCE;
}

/*
 * Does this resource list contain just one kind of element? If so return it
 * otherwise return NO_RESOURCE
 */
static int which_resource(gint assets[NO_RESOURCE])
{
	int i;
	int res = NO_RESOURCE;
	int n_nonzero = 0;

	for (i = 0; i < NO_RESOURCE; i++) {

		if (assets[i] > 0) {
			n_nonzero++;
			res = i;
		}
	}

	if (n_nonzero == 1)
		return res;

	return NO_RESOURCE;
}

/*
 * What resource do we want the most?
 *
 * NOTE: If a resource is not available (players or bank), the
 * resval->value[resource] should be zero.
 */
static int resource_desire(gint assets[NO_RESOURCE],
			   const resource_values_t * resval)
{
	int i;
	BuyType bt;
	int res = NO_RESOURCE;
	float value = 0.0;
	gint need[NO_RESOURCE];

	/* do i need just 1 more for something? */
	for (bt = 0; bt < BUY_LAST; bt++) {
		if (should_buy(assets, bt, resval, need))
			continue;
		res = which_one(need);
		if (res == NO_RESOURCE || resval->value[res] == 0)
			continue;
		return res;
	}

	/* desire the one we don't produce the most */
	res = NO_RESOURCE;
	for (i = 0; i < NO_RESOURCE; i++) {
		if ((resval->value[i] > value) && (assets[i] < 2)) {
			res = i;
			value = resval->value[i];
		}
	}

	return res;
}


static void findit_iterator(Node * n, void *rock)
{
	Node **node = (Node **) rock;
	int i;

	if (!n)
		return;
	if (n->owner != my_player_num())
		return;

	/* if i own this node */
	for (i = 0; i < 3; i++) {
		if (n->edges[i] == NULL)
			continue;
		if (n->edges[i]->owner == my_player_num())
			return;
	}

	*node = n;
}


/* Find the settlement with no roads yet
 *
 */

static Node *void_settlement(void)
{
	Node *ret = NULL;

	for_each_node(&findit_iterator, (void *) &ret);

	return ret;
}

/*
 * Game setup
 * Build one house and one road
 */
static void greedy_setup_house(void)
{
	Node *node;
	resource_values_t resval;

	reevaluate_resources(&resval);

	if (stock_num_settlements() == 0) {
		ai_panic(N_("No settlements in stock to use for setup"));
		return;
	}

	node = best_settlement_spot(TRUE, &resval);

	if (node == NULL) {
		ai_panic(N_("There is no place to setup a settlement"));
		return;
	}

	/*node_add(player, BUILD_SETTLEMENT, node->x, node->y, node->pos, FALSE); */
	cb_build_settlement(node);
}


/*
 * Game setup
 * Build one house and one road
 */
static void greedy_setup_road(void)
{
	Node *node;
	Edge *e = NULL;
	guint i;
	resource_values_t resval;
	float tmp;

	reevaluate_resources(&resval);

	if (stock_num_roads() == 0) {
		ai_panic(N_("No roads in stock to use for setup"));
		return;
	}

	node = void_settlement();

	e = best_road_to_road_spot(node, &tmp, &resval);

	/* if didn't find one just pick one randomly */
	if (e == NULL) {
		for (i = 0; i < G_N_ELEMENTS(node->edges); i++) {
			if (is_edge_on_land(node->edges[i])) {
				e = node->edges[i];
				break;
			}
		}
		if (e == NULL) {
			ai_panic(N_("There is no place to setup a road"));
			return;
		}
	}

	cb_build_road(e);
}

/*
 * Determine if there are any trades that I can do which will give me
 * enough to buy something.
 */
static gboolean find_optimal_trade(gint assets[NO_RESOURCE],
				   const resource_values_t * resval,
				   gint * amount,
				   Resource * trade_away,
				   Resource * want_resource)
{
	Resource res = NO_RESOURCE;
	Resource temp;
	gint need[NO_RESOURCE];
	BuyType bt;

	for (bt = 0; bt < BUY_LAST; bt++) {

		/* If we should buy something, why haven't we bought it? */
		if (should_buy(assets, bt, resval, need))
			continue;

		/* See what we need, and if we can get it. */
		res = which_one(need);
		if (res == NO_RESOURCE || get_bank()[res] == 0)
			continue;

		/* See what we have left after we buy this (one value
		 * will be negative), and whether we have enough of something
		 * to trade for what's missing.
		 */
		leftover_resources(assets, bt, need);
		for (temp = 0; temp < NO_RESOURCE; temp++) {
			if (temp == res)
				continue;
			if (need[temp] > resval->ports[temp]) {
				*amount = resval->ports[temp];
				*trade_away = temp;
				*want_resource = res;
				return TRUE;
			}
		}
	}
	return FALSE;
}

/** I am allowed to do a maritime trade, but will I do it?
 * @param assets The resources I already have
 * @param resval The value of the resources
 * @retval amount The amount to trade
 * @retval trade_away The resource to trade away
 * @retval want_resource The resource I want to have
 * @return TRUE if I want to do the trade
 */
static gboolean will_do_maritime_trade(gint assets[NO_RESOURCE],
				       const resource_values_t * resval,
				       gint * amount,
				       Resource * trade_away,
				       Resource * want_resource)
{
	Resource res, want, discard;

	/* See if we can trade at all. */
	for (res = 0; res < NO_RESOURCE; res++) {
		if (assets[res] >= resval->ports[res])
			break;
	}
	if (res == NO_RESOURCE)
		return FALSE;

	/* See if we can do a single trade that allows us to buy something. */
	if (find_optimal_trade(assets, resval, amount, trade_away,
			       want_resource))
		return TRUE;

	/*
	 * We can trade, but we won't be able to buy anything.
	 *
	 * Try a simple heuristic - if there's a resource we can trade away
	 * and still have at least 1 left, and we need something (and we can
	 * get it), do the trade.  Try to use the best port for this.
	 */
	want = resource_desire(assets, resval);
	if (want == NO_RESOURCE || get_bank()[want] == 0)
		return FALSE;

	discard = NO_RESOURCE;
	for (res = 0; res < NO_RESOURCE; res++) {
		if (res == want)
			continue;
		if (assets[res] > resval->ports[res] &&
		    (discard == NO_RESOURCE
		     || resval->ports[discard] > resval->ports[res]))
			discard = res;
	}

	if (discard != NO_RESOURCE) {
		*trade_away = discard;
		*want_resource = want;
		*amount = resval->ports[discard];
		return TRUE;
	}

	return FALSE;
}

/** I can play the card, but will I do it?
 *  @param cardtype The type of card to consider
 *  @return TRUE if the card is to be played
 */
static gboolean will_play_development_card(DevelType cardtype)
{
	gint amount, i;

	if (is_victory_card(cardtype)) {
		return TRUE;
	}

	switch (cardtype) {
	case DEVEL_SOLDIER:
		return TRUE;
	case DEVEL_YEAR_OF_PLENTY:
		/* only when the bank is full enough */
		amount = 0;
		for (i = 0; i < NO_RESOURCE; i++)
			amount += get_bank()[i];
		if (amount >= 2) {
			return TRUE;
		}
		break;
	case DEVEL_ROAD_BUILDING:
		/* don't if don't have two roads left */
		if (stock_num_roads() < 2)
			break;
		return TRUE;
	case DEVEL_MONOPOLY:
		return determine_monopoly_resource() != NO_RESOURCE;
	default:
		break;
	}
	return FALSE;
}

/*
 * What to do? what to do?
 *
 */

static void greedy_turn(void)
{
	resource_values_t resval;
	guint i;
	gint need[NO_RESOURCE], assets[NO_RESOURCE];

	/* play soldier card before the turn when an own resource is blocked */
	Hex *hex = map_robber_hex(callbacks.get_map());
	if (hex && !have_rolled_dice() && can_play_any_develop()) {
		const Deck *deck = get_devel_deck();
		for (i = 0; i < deck_count(deck); i++) {
			DevelType cardtype = deck_get_guint(deck, i);
			if (cardtype == DEVEL_SOLDIER
			    && can_play_develop(i)) {
				int j;
				for (j = 0; j < 6; j++) {
					if (hex->nodes[j]->owner ==
					    my_player_num()) {
						cb_play_develop(i);
						return;
					}
				}
			}
		}
	}

	if (!have_rolled_dice()) {
		cb_roll();
		return;
	}

	/* Don't wait before the dice roll, that will take too long */
	ai_wait();
	for (i = 0; i < NO_RESOURCE; ++i)
		assets[i] = resource_asset(i);

	reevaluate_resources(&resval);

	/* if can then buy city */
	if (should_buy(assets, BUY_CITY, &resval, need)) {

		Node *n = best_city_spot(&resval);
		if (n != NULL) {
			cb_build_city(n);
			return;
		}
	}

	/* if can then buy settlement */
	if (should_buy(assets, BUY_SETTLEMENT, &resval, need)) {

		Node *n = best_settlement_spot(FALSE, &resval);
		if (n != NULL) {
			cb_build_settlement(n);
			return;
		}

	}

	if (should_buy(assets, BUY_ROAD, &resval, need)) {

		Edge *e = best_road_spot(&resval);

		if (e == NULL) {
			e = best_road_to_road(&resval);
		}

		if (e != NULL) {
			cb_build_road(e);
			return;
		}
	}

	/* if we can buy a development card and there are some left */
	if (should_buy(assets, BUY_DEVEL_CARD, &resval, need)) {
		cb_buy_develop();
		return;
	}

	/* if we have a lot of cards see if we can trade anything */
	if (num_assets(assets) >= 3) {
		if (can_trade_maritime()) {
			gint amount;
			Resource trade_away, want_resource;
			if (will_do_maritime_trade
			    (assets, &resval, &amount, &trade_away,
			     &want_resource)) {
				cb_maritime(amount, trade_away,
					    want_resource);
				return;
			}
		}
	}

	/* play development cards */
	if (can_play_any_develop()) {
		const Deck *deck = get_devel_deck();
		gint num_victory_cards = 0;
		gint victory_point_target, my_points;

		for (i = 0; i < deck_count(deck); i++) {
			DevelType cardtype = deck_get_guint(deck, i);

			/* if it's a vp card, note this for later */
			if (is_victory_card(cardtype)) {
				num_victory_cards++;
				continue;
			}

			/* can't play card we just bought */
			if (can_play_develop(i)) {
				if (will_play_development_card(cardtype)) {
					cb_play_develop(i);
					return;
				}
			}
		}

		/* if we have enough victory cards to win, then play them */
		victory_point_target = game_victory_points();
		my_points = player_get_score(my_player_num());
		if (num_victory_cards + my_points >= victory_point_target) {
			for (i = 0; i < deck_count(deck); i++) {
				DevelType cardtype =
				    deck_get_guint(deck, i);

				if (is_victory_card(cardtype)) {
					cb_play_develop(i);
					return;
				}
			}
		}
	}
	cb_end_turn();
}

static float score_node_hurt_opponents(Node * node)
{
	/* no building there */
	if (node->owner == -1)
		return 0;

	/* do I have a house there? */
	if (my_player_num() == node->owner) {
		if (node->type == BUILD_SETTLEMENT) {
			return -2.0;
		} else {
			return -3.5;
		}
	}

	/* opponent has house there */
	if (node->type == BUILD_SETTLEMENT) {
		return 1.5;
	} else {
		return 2.5;
	}
}

/*
 * How much does putting the robber here hurt my opponents?
 */
static float score_hex_hurt_opponents(Hex * hex)
{
	int i;
	float score = 0;

	if (hex == NULL)
		return -1000;

	/* don't move the pirate. */
	if (!can_robber_or_pirate_be_moved(hex)
	    || hex->terrain == SEA_TERRAIN)
		return -1000;

	for (i = 0; i < 6; i++) {
		score += score_node_hurt_opponents(hex->nodes[i]);
	}

	/* multiply by resource/roll value */
	score *= default_score_hex(hex);

	return score;
}

/*
 * Find the best (worst for opponents) place to put the robber
 *
 */
static void greedy_place_robber(void)
{
	int i, j;
	float bestscore = -1000;
	Hex *besthex = NULL;
	Map *map = callbacks.get_map();

	ai_wait();
	for (i = 0; i < map->x_size; i++) {
		for (j = 0; j < map->y_size; j++) {
			Hex *hex = map_hex(map, i, j);
			float score = score_hex_hurt_opponents(hex);

			if (score > bestscore) {
				bestscore = score;
				besthex = hex;
			}

		}
	}
	cb_place_robber(besthex);
}

static void greedy_steal_building(void)
{
	int i;
	int victim = -1;
	int victim_resources = -1;
	Hex *hex = map_robber_hex(callbacks.get_map());

	/* which opponent to steal from */
	for (i = 0; i < 6; i++) {
		int numres = 0;

		/* if has owner (and isn't me) */
		if ((hex->nodes[i]->owner != -1) &&
		    (hex->nodes[i]->owner != my_player_num())) {

			numres =
			    player_get_num_resource(hex->nodes[i]->owner);
		}

		if (numres > victim_resources) {
			victim = hex->nodes[i]->owner;
			victim_resources = numres;
		}
	}
	cb_rob(victim);
	ai_chat_self_moved_robber();
}

/*
 * A devel card game us two free roads. let's build them
 *
 */

static void greedy_free_road(void)
{
	Edge *e;
	resource_values_t resval;

	reevaluate_resources(&resval);

	e = best_road_spot(&resval);

	if (e == NULL) {
		e = best_road_to_road(&resval);
	}

	if (e == NULL) {
		e = find_random_road();
	}

	if (e != NULL) {

		cb_build_road(e);
		return;

	} else {
		log_message(MSG_ERROR,
			    "unable to find spot to build free road\n");
		cb_disconnect();
	}
}

/*
 * We played a year of plenty card. pick the two resources we most need
 */

static void greedy_year_of_plenty(const gint bank[NO_RESOURCE])
{
	gint want[NO_RESOURCE];
	gint assets[NO_RESOURCE];
	int i;
	int r1, r2;
	resource_values_t resval;

	ai_wait();
	for (i = 0; i < NO_RESOURCE; i++) {
		want[i] = 0;
		assets[i] = resource_asset(i);
	}

	/* what two resources do we desire most */
	reevaluate_resources(&resval);

	r1 = resource_desire(assets, &resval);

	/* If we don't desire anything anymore, ask for a road.
	 * This happens if we have at least 2 of each resource
	 */
	if (r1 == NO_RESOURCE)
		r1 = BRICK_RESOURCE;

	assets[r1]++;

	reevaluate_resources(&resval);

	r2 = resource_desire(assets, &resval);

	if (r2 == NO_RESOURCE)
		r2 = LUMBER_RESOURCE;

	assets[r1]--;

	/* If we want something that is not in the bank, request something else */
	/* WARNING: This code can cause a lockup if the bank is empty, but
	 * then the year of plenty must not have been playable */
	while (bank[r1] < 1)
		r1 = (r1 + 1) % NO_RESOURCE;
	while (bank[r2] < (r1 == r2 ? 2 : 1))
		r2 = (r2 + 1) % NO_RESOURCE;

	want[r1]++;
	want[r2]++;

	cb_choose_plenty(want);
}

/*
 * We played a monopoly card.  Pick a resource
 */

static gint other_players_have(Resource res)
{
	return game_resources() - get_bank()[res] - resource_asset(res);
}

static float monopoly_wildcard_value(const resource_values_t * resval,
				     const gint assets[NO_RESOURCE],
				     gint resource)
{
	return (float) (other_players_have(resource) +
			assets[resource]) / resval->ports[resource];
}

/** Determine the best resource to get with a monopoly card.
 * @return the resource
*/
static gint determine_monopoly_resource(void)
{
	gint assets[NO_RESOURCE];
	int i;
	gint most_desired;
	gint most_wildcards;
	resource_values_t resval;

	for (i = 0; i < NO_RESOURCE; i++)
		assets[i] = resource_asset(i);

	/* order resources by preference */
	reevaluate_resources(&resval);

	/* try to get something we need */
	most_desired = resource_desire(assets, &resval);

	/* try to get the optimal maritime trade. */
	most_wildcards = 0;
	for (i = 1; i < NO_RESOURCE; i++) {
		if (monopoly_wildcard_value(&resval, assets, i) >
		    monopoly_wildcard_value(&resval, assets,
					    most_wildcards))
			most_wildcards = i;
	}

	/* choose the best */
	if (most_desired != NO_RESOURCE
	    && other_players_have(most_desired) >
	    monopoly_wildcard_value(&resval, assets, most_wildcards)) {
		return most_desired;
	} else if (monopoly_wildcard_value(&resval, assets, most_wildcards)
		   >= 1.0) {
		return most_wildcards;
	} else {
		return NO_RESOURCE;
	}
}

static void greedy_monopoly(void)
{
	ai_wait();
	cb_choose_monopoly(determine_monopoly_resource());
}

/*
 * Of these resources which is least valuable to us
 *
 * Get rid of the one we have the most of
 * if there's a tie let resource_values_t settle it
 */

static int least_valuable(gint assets[NO_RESOURCE],
			  const resource_values_t * resval)
{
	int ret = NO_RESOURCE;
	int res;
	int most = 0;
	float mostval = -1;

	for (res = 0; res < NO_RESOURCE; res++) {
		if (assets[res] > most) {
			if (resval->value[res] > mostval) {
				ret = res;
				most = assets[res];
				mostval = resval->value[res];
			}
		}
	}

	return ret;
}

/*
 * Which resource do we desire the least?
 */

static int resource_desire_least(gint my_assets[NO_RESOURCE],
				 const resource_values_t * resval)
{
	BuyType bt;
	int res;
	gint assets[NO_RESOURCE];
	gint need[NO_RESOURCE];
	int leastval;

	/* make copy of what we got */
	for (res = 0; res != NO_RESOURCE; res++) {
		assets[res] = my_assets[res];
	}

	/* eliminate things we need to build stuff */
	for (bt = 0; bt < BUY_LAST; bt++) {
		if (should_buy(assets, bt, resval, need)) {
			cost_buy(cost_of(bt), assets);
		}
	}

	/* of what's left what do do we care for least */
	leastval = least_valuable(assets, resval);
	if (leastval != NO_RESOURCE)
		return leastval;

	/* otherwise least valuable of what we have in total */
	leastval = least_valuable(my_assets, resval);
	if (leastval != NO_RESOURCE)
		return leastval;

	/* last resort just pick something */
	for (res = 0; res < NO_RESOURCE; res++) {
		if (my_assets[res] > 0)
			return res;
	}

	/* Should never get here */
	g_assert_not_reached();
	return 0;
}


/*
 * A seven was rolled. we need to discard some resources :(
 *
 */
static void greedy_discard(int num)
{
	int res;
	gint todiscard[NO_RESOURCE];
	int i;
	resource_values_t resval;
	gint assets[NO_RESOURCE];

	/* zero out */
	for (res = 0; res != NO_RESOURCE; res++) {
		todiscard[res] = 0;
		assets[res] = resource_asset(res);
	}

	for (i = 0; i < num; i++) {

		reevaluate_resources(&resval);

		res = resource_desire_least(assets, &resval);

		todiscard[res]++;
		assets[res]--;
	}

	cb_discard(todiscard);
}

/*
 * Domestic Trade
 *
 */
static int quote_next_num(void)
{
	return quote_num++;
}

static void greedy_quote_start(void)
{
	quote_num = 0;
}

static int trade_desired(gint assets[NO_RESOURCE], gint give, gint take,
			 gboolean free_offer)
{
	int i, n;
	int res = NO_RESOURCE;
	resource_values_t resval;
	float value = 0.0;
	gint need[NO_RESOURCE];

	if (!free_offer) {
		/* don't give away cards we have only once */
		if (assets[give] <= 1) {
			return 0;
		}

		/* make it as if we don't have what we're trading away */
		assets[give] -= 1;
	}

	reevaluate_resources(&resval);
	for (n = 1; n <= 3; ++n) {
		/* do i need something more for something? */
		if (!should_buy(assets, BUY_CITY, &resval, need)) {
			if ((res = which_resource(need)) == take
			    && need[res] == n)
				break;
		}
		if (!should_buy(assets, BUY_SETTLEMENT, &resval, need)) {
			if ((res = which_resource(need)) == take
			    && need[res] == n)
				break;
		}
		if (!should_buy(assets, BUY_ROAD, &resval, need)) {
			if ((res = which_resource(need)) == take
			    && need[res] == n)
				break;
		}
		if (!should_buy(assets, BUY_DEVEL_CARD, &resval, need)) {
			if ((res = which_resource(need)) == take
			    && need[res] == n)
				break;
		}
	}
	if (!free_offer)
		assets[give] += 1;
	if (n <= 3)
		return n;

	/* desire the one we don't produce the most */
	for (i = 0; i < NO_RESOURCE; i++) {
		if ((resval.value[i] > value) && (assets[i] < 2)) {
			res = i;
			value = resval.value[i];
		}
	}

	if (res == take && assets[give] > 2) {
		return 1;
	}

	return 0;
}

static void greedy_consider_quote(G_GNUC_UNUSED gint partner,
				  gint we_receive[NO_RESOURCE],
				  gint we_supply[NO_RESOURCE])
{
	gint give, take, ntake;
	gint give_res[NO_RESOURCE], take_res[NO_RESOURCE],
	    my_assets[NO_RESOURCE];
	gint i;
	gboolean free_offer;

	free_offer = TRUE;
	for (i = 0; i < NO_RESOURCE; ++i) {
		my_assets[i] = resource_asset(i);
		free_offer &= we_supply[i] == 0;
	}

	for (give = 0; give < NO_RESOURCE; give++) {
		/* A free offer is always accepted */
		if (!free_offer) {
			if (we_supply[give] == 0)
				continue;
			if (my_assets[give] == 0)
				continue;
		}
		for (take = 0; take < NO_RESOURCE; take++) {
			/* Don't do stupid offers */
			if (!free_offer && take == give)
				continue;
			if (we_receive[take] == 0)
				continue;
			if ((ntake =
			     trade_desired(my_assets, give, take,
					   free_offer)) > 0)
				goto doquote;
		}
	}

	/* Do not decline anything for free, just take it all */
	if (free_offer) {
		cb_quote(quote_next_num(), we_supply, we_receive);
		log_message(MSG_INFO, "Taking the whole free offer.\n");
		return;
	}

	log_message(MSG_INFO, _("Rejecting trade.\n"));
	cb_end_quote();
	return;

      doquote:
	for (i = 0; i < NO_RESOURCE; ++i) {
		give_res[i] = (give == i && !free_offer) ? 1 : 0;
		take_res[i] = take == i ? ntake : 0;
	}
	cb_quote(quote_next_num(), give_res, take_res);
	log_message(MSG_INFO, "Quoting.\n");
}

static void greedy_setup(gint num_settlements, gint num_roads)
{
	ai_wait();
	if (num_settlements > 0)
		greedy_setup_house();
	else if (num_roads > 0)
		greedy_setup_road();
	else
		cb_end_turn();
}

static void greedy_roadbuilding(gint num_roads)
{
	ai_wait();
	if (num_roads > 0)
		greedy_free_road();
	else
		cb_end_turn();
}

static void greedy_discard_add(gint player_num, gint discard_num)
{
	ai_chat_discard(player_num, discard_num);
	if (player_num == my_player_num()) {
		ai_wait();
		greedy_discard(discard_num);
	}
}

static void greedy_gold_choose(gint gold_num, const gint * bank)
{
	resource_values_t resval;
	gint assets[NO_RESOURCE];
	gint want[NO_RESOURCE];
	gint my_bank[NO_RESOURCE];
	gint i;
	int r1;

	for (i = 0; i < NO_RESOURCE; i++) {
		want[i] = 0;
		assets[i] = resource_asset(i);
		my_bank[i] = bank[i];
	}

	for (i = 0; i < gold_num; i++) {
		gint j;

		reevaluate_resources(&resval);

		/* If the bank has been emptied, don't desire it */
		for (j = 0; j < NO_RESOURCE; j++) {
			if (my_bank[j] == 0) {
				resval.value[j] = 0;
			}
		}

		r1 = resource_desire(assets, &resval);
		/* If we don't want anything, start emptying the bank */
		if (r1 == NO_RESOURCE) {
			r1 = 0;
			/* Potential deadlock, but bank is always full enough */
			while (my_bank[r1] == 0)
				r1++;
		}
		want[r1]++;
		assets[r1]++;
		my_bank[r1]--;
	}
	cb_choose_gold(want);
}

void greedy_init(void)
{
	callbacks.setup = &greedy_setup;
	callbacks.turn = &greedy_turn;
	callbacks.robber = &greedy_place_robber;
	callbacks.steal_building = &greedy_steal_building;
	callbacks.roadbuilding = &greedy_roadbuilding;
	callbacks.plenty = &greedy_year_of_plenty;
	callbacks.monopoly = &greedy_monopoly;
	callbacks.discard_add = &greedy_discard_add;
	callbacks.quote_start = &greedy_quote_start;
	callbacks.quote = &greedy_consider_quote;
	callbacks.gold_choose = &greedy_gold_choose;
}
