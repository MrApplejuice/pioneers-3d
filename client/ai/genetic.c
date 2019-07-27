/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2013 Rodrigo Espiga Gómez <rodrigoespiga@hotmail.com>
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
#include "genetic_core.h"
#include "cost.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * This is computer a player for Pioneers based on a genetic algorithm.
 */

/** default chromosome */
struct chromosome_t thisChromosome = (struct chromosome_t) {
	{
	 {1.371242, 1.984368, 1.336144, 1.111876, 1.206090, 0.052862,
	  1.506242, 1.684672},
	 /* weight of every resource, value of Development Card, relative value of City (compared to equivalen Settlement) and Ports, when I have 0 Victory Points */
	 {1.262966, 1.813653, 1.542921, 1.117926, 1.338699, 3.004164, 1.682371, 3.554963},	/* when I have 1 */
	 {1.770271, 1.405915, 1.947900, 1.267916, 1.645481, 0.266591, 1.194223, 9.644183},	/* 2 */
	 {1.371931, 1.962052, 1.071650, 1.335991, 1.076773, 3.367637, 1.703712, 6.706741},	/* 3 */
	 {1.680568, 1.306249, 1.743218, 1.761761, 1.025129, 1.121913, 0.284101, 1.559953},	/* 4 */
	 {1.674057, 1.994260, 1.067930, 1.046954, 1.594052, 3.381377, 0.250717, 4.673997},	/* 5 */
	 {1.116162, 1.225133, 1.141381, 1.093680, 1.976306, 3.066351, 0.672955, 9.152541},	/* 6 */
	 {1.402262, 1.515350, 1.835050, 1.781056, 1.091004, 4.470943, 0.112013, 9.202737},	/* 7 */
	 {1.009420, 1.065439, 1.938907, 1.966596, 1.279951, 0.777970, 1.659520, 3.782044},	/* 8 */
	 {1.828237, 1.597190, 1.909004, 1.690179, 1.643655, 2.087759, 1.848852, 2.809332},	/* 9 */
	 },
	1.714336, 0.580583, 0.265190	/* depreciation, turn and probability */
};

typedef struct resource_values_s {
	float value[NO_RESOURCE];
	MaritimeInfo info;
	gint ports[NO_RESOURCE];
} resource_values_t;

static int quote_num;
static gboolean default_chromosome_used = TRUE;

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
static Edge *best_road_to_road_spot(Node * n, float *score, const struct chromosome_t
				    *myChromosome,
				    const struct gameState_t *myGameState);

static Edge *best_road_to_road(const struct chromosome_t *myChromosome,
			       const struct gameState_t *myGameState,
			       float *destinationScore);

static Edge *best_road_spot(const struct chromosome_t *myChromosome,
			    const struct gameState_t *myGameState,
			    float *destinationScore);

static Node *best_city_spot(const struct chromosome_t *myChromosome,
			    const struct gameState_t *myGameState);

static Node *best_settlement_spot(gboolean during_setup,
				  const struct chromosome_t *myChromosome,
				  const struct gameState_t *myGameState);

static int places_can_build_settlement(void);
static gint determine_monopoly_resource(void);

void updateTradingMatrix(const struct chromosome_t *myChromosome,
			 float profit,
			 struct tradingMatrixes_t *tradeThisForThat,
			 struct gameState_t myGameState, int myTurn);
void outputGameState(const struct gameState_t myGameState);
void outputStrategy(strategy_t myStrategy,
		    const struct simulationsData_t mySimulation,
		    const struct gameState_t myGameState);

float best_maritime_trade(const struct tradingMatrixes_t
			  thisTradingMatrixes, int *amount,
			  Resource * trade_away, Resource * want_resource);
float bestActualAverageResourcesSupply(const struct gameState_t
				       *myGameState);



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

static void genetic_for_each_node(iterate_node_func_t * func, void *rock,
				  node_seen_set_t * nodesSeen)
{
	Map *map;
	int i, j, k;

	map = callbacks.get_map();
	for (i = 0; i < map->x_size; i++) {
		for (j = 0; j < map->y_size; j++) {
			for (k = 0; k < 6; k++) {
				Node *n = map_node(map, i, j, k);

				if (n)
					if (!nodeset_isset(nodesSeen, n)) {
						nodeset_set(nodesSeen, n);
						func(n, rock);
					}
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
			   const struct chromosome_t *myChromosome,
			   const struct gameState_t *myGameState,
			   gint need[NO_RESOURCE])
{
	float destinationScore;
	if (!can_pay_for(assets, cost_of(bt), need))
		return FALSE;

	switch (bt) {
	case BUY_CITY:
		return (stock_num_cities() > 0 &&
			best_city_spot(myChromosome, myGameState) != NULL);
	case BUY_SETTLEMENT:
		return (stock_num_settlements() > 0 &&
			best_settlement_spot(FALSE, myChromosome,
					     myGameState) != NULL);
	case BUY_ROAD:
		/* don't sprawl :) */
		return (stock_num_roads() > 0 &&
			places_can_build_settlement() <= 2 &&
			(best_road_spot
			 (myChromosome, myGameState,
			  &destinationScore) != NULL
			 || best_road_to_road(myChromosome, myGameState,
					      &destinationScore) != NULL));
	case BUY_DEVEL_CARD:
		return (stock_num_develop() > 0 && can_buy_develop());
	default:
		/* xxx bridge, ship */
		return FALSE;
	}
}

#if 0

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
#endif

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

static int dice_AVR(gint roll)
/* Average Resources Supply given each 36 turns by an hexagon with that number*/
{
	switch (roll) {
	case 2:
	case 12:
		return 1;
	case 3:
	case 11:
		return 2;
	case 4:
	case 10:
		return 3;
	case 5:
	case 9:
		return 4;
	case 6:
	case 8:
		return 5;
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

/* For each node I own update the matrix resourcesSupply accordingly*/
/* Remember that resourcesSupply[11][5] row index represents different dice outcomes from 2 to 12
 */

static void genetic_reevaluate_iterator(Node * n, void *rock)
{
	struct gameState_t *myGameState = (struct gameState_t *) rock;

	/* if i own this node */
	if ((n) && (n->owner == my_player_num())) {
		int l;
		for (l = 0; l < 3; l++) {
			Hex *h = n->hexes[l];
			int supply = 1;

			if (n->type == BUILD_CITY)
				supply = 2;

			if (h && h->terrain < DESERT_TERRAIN) {
				int i = (h->roll) - 2;	/* rolls 2 to 12 map as 0 to 10 row indexes */
				int j = terrain_to_resource(h->terrain);
				myGameState->resourcesSupply[i][j] +=
				    supply;
			}

		}
	}


}

/* Updates the information in struct gameState_t pointed by myGameState*/
static void reevaluate_gameState_supply_matrix_and_resources(struct
							     gameState_t
							     *myGameState)
{
	int i, j;
	node_seen_set_t nodesSeen;
	nodeset_reset(&nodesSeen);

	for (i = 0; i <= 10; i++) {
		for (j = 0; j < NO_RESOURCE; j++) {
			myGameState->resourcesSupply[i][j] = 0;
		}
	}

	genetic_for_each_node(&genetic_reevaluate_iterator,
			      (void *) myGameState, &nodesSeen);

	for (i = 0; i < NO_RESOURCE; ++i)
		myGameState->resourcesAlreadyHave[i] = resource_asset(i);
	for (i = 0; i < 5; i++)
		myGameState->actionValue[i] = 0;
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


#if 0
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
#endif


#if 0
/*
 * How valuable is this hex to me?
 */

static float score_hex(Hex * hex, const resource_values_t * resval)
{
	float score;

	if (hex == NULL)
		return 0;


	score =
	    resource_value(terrain_to_resource(hex->terrain),
			   resval) * dice_prob(hex->roll);


	if (!resval->info.any_resource) {
		if (hex->resource == ANY_RESOURCE)
			score += 0.5f;
	}

	return score;
}
#endif

float bestActualAverageResourcesSupply(const struct gameState_t
				       *myGameState)
{
	int i;
	float best = 0;
	float this;
	for (i = 0; i <= 4; i++) {
		this = actualAverageResourcesSupply(i, myGameState);
		if (this > best)
			best = this;
	}
	return (best);
}



/* Given a maritime hex with port and a not NULL node, returns TRUE if that port is facing to the node, thus granting access to port to the node */
static int facingOK(const Node * node, Hex * hex)
{
	Node *nodeOne, *nodeTwo;
	nodeOne = hex->nodes[hex->facing];
	if (hex->facing != 0)
		nodeTwo = hex->nodes[(hex->facing) - 1];
	else
		nodeTwo = hex->nodes[5];
	if ((nodeOne == node) || (nodeTwo == node))
		return (1);	/* This node is one of the two nodes affected by this hex's port */
	else
		return (0);
}



/* It returns the value given to an hex surrounding a node. We also need to know the node to check (for maritime hexes) if it has acces to the port */
static float genetic_score_hex(const Node * node, Hex * hex,
			       const struct chromosome_t *myChromosome,
			       const struct gameState_t *myGameState)
{
	Resource resrc;
	int victoryPoints = player_get_score(my_player_num());
	if (victoryPoints > 9)
		victoryPoints = 9;	/* max index in chromosome */
	MaritimeInfo info;
	float value = 0;
	float port_bonus = 0;	/* bonus for being a port */
	float port_constant =
	    myChromosome->resourcesValueMatrix[victoryPoints][7];
	int port = 4;		/* Bank trade, used to calculate depreciation of resources */
	int nodeHasPort = 0;	/* Does it actually have access to the port or is just close to it? */
	if (hex == NULL)
		return 0;
	int increment = dice_AVR(hex->roll);	/* Average resources supply each 36 turns given by that number */
	resrc = terrain_to_resource(hex->terrain);
	map_maritime_info(callbacks.get_map(), &info, my_player_num());
	/* I want to decrease the devaluation the hex resource suffers (increase the hex value) if a I have a generic port or a specific port to export that resource */
	if (info.any_resource)
		port = 3;	/* I have a generic port, depreciation will be less */
	else if (resrc < NO_RESOURCE && info.specific_resource[resrc])
		port = 2;	/* I have a port to export this resource, depreciation will be even less */
	nodeHasPort = facingOK(node, hex);
	if (resrc < NO_RESOURCE) {
		/* Normal hex, its value depends on its resource supply */
		value =
		    resourcesIncrementValue(increment, resrc,
					    victoryPoints, myChromosome,
					    myGameState, port);
	} else if ((hex->resource == ANY_RESOURCE) && (!info.any_resource)) {
		/* This is a generic port and I do not have one, its value depends on my best supplied resource */
		port_bonus =
		    bestActualAverageResourcesSupply(myGameState) / 36.0;
		port_bonus = port_bonus * port_constant * nodeHasPort;
	} else if ((hex->terrain == SEA_TERRAIN)
		   && (hex->resource < NO_RESOURCE)) {
		/* This hex has a specific port, the better the supply I have of that resource, the most valuable will be the port */
		port_bonus =
		    actualAverageResourcesSupply(hex->resource,
						 myGameState) / 24.0;
		port_bonus = port_bonus * port_constant * nodeHasPort;
	} else if (resrc == GOLD_RESOURCE)
		value = 5;
	return (value + port_bonus);
	/* It will return either value (for normal hexes) or port_bonus (for a maritime hex with port, if the node I am calculating this hex for, has access to that port) */
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

#if 0
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
#endif

static float genetic_score_node(const Node * node, gboolean city,
				const struct chromosome_t *myChromosome,
				const struct gameState_t *myGameState)
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
		if (node->owner != -1)	/* I want a settlement, and this is already occupied */
			return -1;
	}

	for (i = 0; i < 3; i++) {
		score +=
		    genetic_score_hex(node, node->hexes[i], myChromosome,
				      myGameState);
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
				  const struct chromosome_t *myChromosome,
				  const struct gameState_t *myGameState)
{
	int i, j, k, l;
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

				score =
				    genetic_score_node(n, FALSE,
						       myChromosome,
						       myGameState);

				/* If another player can already build in this node, give it a score bonus so I try harder to build there before another player does it */
				if (score > 0) {
					for (l = 0; l < 3; l++) {
						if (n->edges[l]) {
							if (((n->edges
							      [l])->owner
							     != -1)
							    &&
							    ((n->edges
							      [l])->owner
							     !=
							     my_player_num
							     ()))
								score += 1;
						}
					}
				}


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
static Node *best_city_spot(const struct chromosome_t *myChromosome,
			    const struct gameState_t *myGameState)
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
					    genetic_score_node(n, TRUE,
							       myChromosome,
							       myGameState);

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
			  const struct chromosome_t *myChromosome,
			  const struct gameState_t *myGameState)
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
						 &cur_score, myChromosome,
						 myGameState);

		} else if (can_road_be_built(e, my_player_num())) {

			/* no owner, how good is the other node ? */
			cur_e = e;

			cur_score =
			    genetic_score_node(othernode, FALSE,
					       myChromosome, myGameState);

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
static Edge *best_road_to_road_spot(Node * n, float *score, const struct chromosome_t
				    *myChromosome,
				    const struct gameState_t *myGameState)
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
						    genetic_score_node
						    (other_node
						     (e2, othernode),
						     FALSE,
						     myChromosome,
						     myGameState);

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
static Edge *best_road_to_road(const struct chromosome_t *myChromosome,
			       const struct gameState_t *myGameState,
			       float *destinationScore)
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
								   myChromosome,
								   myGameState);
					if (score > bestscore) {
						best = e;
						bestscore = score;
					}
				}
			}
		}
	}
	*destinationScore = bestscore;
	return best;
}

/*
 * Best road spot
 *
 */
static Edge *best_road_spot(const struct chromosome_t *myChromosome,
			    const struct gameState_t *myGameState,
			    float *destinationScore)
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
							 &score,
							 myChromosome,
							 myGameState);

					if (score > bestscore) {
						best = e;
						bestscore = score;
					}
				}
			}

		}
	}
	*destinationScore = bestscore;

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

#if 0
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
#endif

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
			   const struct chromosome_t *myChromosome,
			   const struct gameState_t *myGameState)
{

	BuyType bt;
	int res = NO_RESOURCE;
	gint need[NO_RESOURCE];

	/* This code is temporary, it should use bestStrategy to find out which one resource increments its expected profit the most */

	/* do i need just 1 more for something? */
	for (bt = 0; bt < BUY_LAST; bt++) {
		if (should_buy
		    (assets, bt, myChromosome, myGameState, need))
			continue;
		res = which_one(need);
		if (res == NO_RESOURCE)
			continue;
		return res;
	}
	res = 1;

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
static void genetic_setup_house(void)
{
	Node *node;
	struct gameState_t myGameState;
	/*resource_values_t resval;

	   reevaluate_resources(&resval); */

	/* myGameState->resourcesSupply matrix will be used to value hexes by genetic_score_hex, we need to update it */
	reevaluate_gameState_supply_matrix_and_resources(&myGameState);



	if (stock_num_settlements() == 0) {
		ai_panic(N_("No settlements in stock to use for setup"));
		return;
	}

	node = best_settlement_spot(TRUE, &thisChromosome, &myGameState);

	if (node == NULL) {
		ai_panic(N_("There is no place to setup a settlement"));
		return;
	}

	/* node_add(player, BUILD_SETTLEMENT, node->x, node->y, node->pos, FALSE); */
	cb_build_settlement(node);
}


/*
 * Game setup
 * Build one house and one road
 */
static void genetic_setup_road(void)
{
	Node *node;
	Edge *e = NULL;
	guint i;
	float tmp;
	struct gameState_t myGameState;
	/* resource_values_t resval;
	   reevaluate_resources(&resval); */

	/* myGameState->resourcesSupply matrix will be used to value hexes by genetic_score_hex, we need to update it */
	reevaluate_gameState_supply_matrix_and_resources(&myGameState);

	if (stock_num_roads() == 0) {
		ai_panic(N_("No roads in stock to use for setup"));
		return;
	}

	node = void_settlement();

	e = best_road_to_road_spot(node, &tmp, &thisChromosome,
				   &myGameState);

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

#if 0
static void set_myPorts(gint * myPorts)
{
	MaritimeInfo info;
	int i;
	map_maritime_info(callbacks.get_map(), &info, my_player_num());
	for (i = 0; i < NO_RESOURCE; i++) {
		if (info.specific_resource[i])
			myPorts[i] = 2;
		else if (info.any_resource)
			myPorts[i] = 3;
		else
			myPorts[i] = 4;
	}
	return;
}
#endif





void updateTradingMatrix(const struct chromosome_t *myChromosome,
			 float profit,
			 struct tradingMatrixes_t *tradeThisForThat,
			 struct gameState_t myGameState, int myTurn)
{
	/* It will check if any resource trade will improve profit in a given turn and will update the trading matrix consequently.
	 * At this point it will only check whether any one resource by one resource trading is beneficial to me.
	 * If a 4:1, 3:1 or 2:1 trading is possible and beneficial it will set according matrix to that profit too
	 * IMPROVEMENT: You can use a constant k[1..2] so that I am only interested in trading if profitAfterTrade is k times higher than profit.
	 * That k could improve when Im trading with "winning" adversaries.*/
	int give, take;		/* I give resource give, I get resource take */
	float profitAfterTrade;
	struct simulationsData_t thisSimulation;
	strategy_t thisStrategy;
	float turn = myChromosome->turn;
	float prob = myChromosome->probability;
	MaritimeInfo info;
	map_maritime_info(callbacks.get_map(), &info, my_player_num());

	for (give = 0; give <= 4; give++) {
		for (take = 0; take <= 4; take++) {
			tradeThisForThat->specificResource[give][take] = 0;
			tradeThisForThat->genericResource[give][take] = 0;
			tradeThisForThat->bankTrade[give][take] = 0;
			tradeThisForThat->internalTrade[give][take] = 0;


		}
	}
	for (give = 0; give <= 4; give++) {
		if ((myGameState.resourcesAlreadyHave[give] >= 2) && (info.specific_resource[give])) {	/* I have a specific port to export give */
			for (take = 0; take <= 4; take++) {
				if ((give != take) && (get_bank()[take])) {	/* There is no point in trading the same resource */
					myGameState.resourcesAlreadyHave
					    [give] -= 2;
					myGameState.resourcesAlreadyHave
					    [take]++;
					profitAfterTrade =
					    bestStrategy(turn, prob,
							 &thisSimulation,
							 thisStrategy,
							 myGameState, 0,
							 myTurn,
							 num_players());
					if (profitAfterTrade > profit) {
						tradeThisForThat->specificResource
						    [give]
						    [take] =
						    profitAfterTrade;
					}
					myGameState.resourcesAlreadyHave
					    [give] += 2;
					myGameState.resourcesAlreadyHave
					    [take]--;
				}
			}
		}
		/* It will check this only if it is not possible to do 2:1 trade */
		else if ((myGameState.resourcesAlreadyHave[give] >= 3) && (info.any_resource)) {	/* I have a generic port to export any resource */
			for (take = 0; take <= 4; take++) {
				if ((give != take) && (get_bank()[take])) {	/* There is no point in trading the same resource */
					myGameState.resourcesAlreadyHave
					    [give] -= 3;
					myGameState.resourcesAlreadyHave
					    [take]++;
					profitAfterTrade =
					    bestStrategy(turn, prob,
							 &thisSimulation,
							 thisStrategy,
							 myGameState, 0,
							 myTurn,
							 num_players());
					if (profitAfterTrade > profit) {
						tradeThisForThat->genericResource
						    [give]
						    [take] =
						    profitAfterTrade;
					}
					myGameState.resourcesAlreadyHave
					    [give] += 3;
					myGameState.resourcesAlreadyHave
					    [take]--;
				}
			}
		}
		/* It will check this only if it is neither possible to do 2:1 nor 3:1 trade */
		else if (myGameState.resourcesAlreadyHave[give] >= 4) {	/* I have at least 4 of this resource, I could do 4:1 trade */
			for (take = 0; take <= 4; take++) {
				if ((give != take) && (get_bank()[take])) {	/* There is no point in trading the same resource */
					myGameState.resourcesAlreadyHave
					    [give] -= 4;
					myGameState.resourcesAlreadyHave
					    [take]++;
					profitAfterTrade =
					    bestStrategy(turn, prob,
							 &thisSimulation,
							 thisStrategy,
							 myGameState, 0,
							 myTurn,
							 num_players());
					if (profitAfterTrade > profit) {
						tradeThisForThat->bankTrade
						    [give][take] =
						    profitAfterTrade;
					}
					myGameState.resourcesAlreadyHave
					    [give] += 4;
					myGameState.resourcesAlreadyHave
					    [take]--;
				}
			}
		}
		/* At this point the highest value of these 3 matrixes represents the most beneficial trade of all */

		/* It will always check all possible trades with other players */
#if 0

		if (myGameState.resourcesAlreadyHave[give]) {	/* I cannot trade something I dont have */
			for (take = 0; take <= 4; take++) {
				if (give != take) {	/* There is no point in trading the same resource */
					myGameState.resourcesAlreadyHave
					    [give]--;
					myGameState.resourcesAlreadyHave
					    [take]++;
					profitAfterTrade =
					    bestStrategy(turn, prob,
							 &thisSimulation,
							 thisStrategy,
							 myGameState, 0,
							 num_players());
					if (profitAfterTrade > profit) {
						tradeThisForThat->
						    internalTrade[give]
						    [take] =
						    profitAfterTrade;
					}
					myGameState.resourcesAlreadyHave
					    [give]++;
					myGameState.resourcesAlreadyHave
					    [take]--;
				}
			}
		}
#endif
	}
}

float best_maritime_trade(const struct tradingMatrixes_t
			  thisTradingMatrixes, int *amount,
			  Resource * trade_away, Resource * want_resource)
/* Inspects thisTradingMatrixes and outputs the most favorable maritime trade */
{
	float best = 0;
	int portType = 0;
	int give, take, bestgive, besttake;
	bestgive = 0;
	besttake = 0;
	for (give = 0; give < 5; give++) {
		for (take = 0; take < 5; take++) {
			if (take != give) {
				if (thisTradingMatrixes.bankTrade[give]
				    [take] > best) {
					best =
					    thisTradingMatrixes.bankTrade
					    [give][take];
					portType = 4;
					bestgive = give;
					besttake = take;
				}
				if (thisTradingMatrixes.specificResource
				    [give][take] > best) {
					best =
					    thisTradingMatrixes.specificResource
					    [give][take];
					portType = 2;
					bestgive = give;
					besttake = take;
				}
				if (thisTradingMatrixes.genericResource
				    [give][take] > best) {
					best =
					    thisTradingMatrixes.genericResource
					    [give][take];
					portType = 3;
					bestgive = give;
					besttake = take;
				}
			}
		}
	}
	*amount = portType;
	*trade_away = bestgive;
	*want_resource = besttake;
	return (best);
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

void outputGameState(const struct gameState_t myGameState)
{
	int i, j;
	printf("\033[2J");	/*  clear the screen  */
	printf("\033[H");	/*  position cursor at top-left corner */
	/*int sysret;
	   sysret=system("clear");
	   if (sysret) return; */
	printf("\t\t\t\tBr\tGr\tOr\tWo\tLu\n");
	for (i = 0; i <= 10; i++) {
		printf("\t\t\t\t");
		for (j = 0; j < 5; j++) {
			printf("%d\t", myGameState.resourcesSupply[i][j]);
		}
		printf("If %d is rolled\n", i + 2);
	}
	printf("\n\t\t\t\tSET\tCIT\tDEV\tRSET\tRRSET\n");
	printf("\t\tActionValues:\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n\n",
	       myGameState.actionValue[SET], myGameState.actionValue[CIT],
	       myGameState.actionValue[DEV], myGameState.actionValue[RSET],
	       myGameState.actionValue[RRSET]);
	printf("\t\t\t\tBr\tGr\tOr\tWo\tLu\n");
	printf("\t\tResources:");
	for (i = 0; i < NO_RESOURCE; ++i)
		printf("\t%d", myGameState.resourcesAlreadyHave[i]);
	printf("\tTotal resources:\t%d\t\tVictory Points:\t%d\n\n",
	       player_get(my_player_num())->statistics[STAT_RESOURCES],
	       player_get_score(my_player_num()));
	printf("Statistics:\t");
	for (i = 0; i <= STAT_DEVELOPMENT; i++) {
		printf("%d\t", player_get(my_player_num())->statistics[i]);
	}
	printf
	    ("\nStock of:\tRoads %d\tSettlements %d\tCities %d\tDev. Cards %d\n",
	     stock_num_roads(), stock_num_settlements(),
	     stock_num_cities(), stock_num_develop());
}


void outputStrategy(strategy_t myStrategy,
		    const struct simulationsData_t mySimulation,
		    const struct gameState_t myGameState)
{
	int time_a, time_b;
	time_a = mySimulation.turnsToAction[myStrategy[0]];
	time_b =
	    mySimulation.timeCombinedAction[myStrategy[0]][myStrategy[1]];
	printf("\n\t\tMy strategy is to do ");
	printAction(myStrategy[0]);
	printf(" at time %d, and ", time_a);
	printAction(myStrategy[1]);
	printf(" at time %d\n", time_b);
	if (time_a == 0) {
		printf("\t\tSince I can do ");
		printAction(myStrategy[0]);
		printf(" now, I will do it\n");
	} else if (((myStrategy[0] == RSET) || (myStrategy[0] == RRSET)
		    || (myStrategy[0] == CIT))
		   &&
		   (checkRoadNow
		    (myStrategy[0], myStrategy[1], myGameState))) {
		printf("\t\tI cannot do ");
		printAction(myStrategy[0]);
		printf(" complete, but I can already build the road.\n");
	}
	printf("\n");

}

/* Returns TRUE if I am at 1 point from winning the game */
static int lastMinute(void)
{
	const Deck *deck = get_devel_deck();
	gint num_victory_cards = 0;
	gint victory_point_target, my_points;
	guint i;

	for (i = 0; i < deck_count(deck); i++) {
		DevelType cardtype = deck_get_guint(deck, i);

		/* if it's a vp card, note this for later */
		if (is_victory_card(cardtype)) {
			num_victory_cards++;
			continue;
		}
	}

	victory_point_target = game_victory_points();
	my_points = player_get_score(my_player_num());
	if ((num_victory_cards + my_points) == (victory_point_target - 1)) {
		return (1);
	} else
		return (0);
}


/*
 * What to do? what to do?
 *
 */

static void genetic_turn(void)
{
	/* resource_values_t resval; */
	struct gameState_t thisGameState;
	struct simulationsData_t thisSimulation;
	guint i;
	strategy_t thisStrategy;
	float turn, probability;
	turn = thisChromosome.turn;
	probability = thisChromosome.probability;
	Node *city_node;
	Node *sett_node;
	Edge *road_edge;
	Edge *long_road_edge;
	float destinationRoadScore, destinationLongRoadScore;

	int time_a;
	float thisStrategyProfit;
	/* int assets[5],need[5]; */
	struct tradingMatrixes_t thisTradingMatrixes;
	Resource trade_away, want_resource;
	int amount;

	printf("Entering genetic_turn...\n");
	int victoryPoints = player_get_score(my_player_num());

	if (victoryPoints > 9)
		victoryPoints = 9;	/* Maximum index in the chromosome */



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
	/* reevaluate_resources(&resval); */

	/* This is were I should read the chromosome, now it is using the default */

	/* I set myGameState with the information needed */
	reevaluate_gameState_supply_matrix_and_resources(&thisGameState);
	/* I need to know the value of the best possible Settlement, City, Road to Settlement and Long Road to Settlement to set thisGameState.actionValue for each of them */
	/* Note that even though I am passing thisGameState to should_buy it does not need to access thisGameState.actionValue */

	/*Best City */
	city_node = best_city_spot(&thisChromosome, &thisGameState);
	if ((city_node != NULL) && (stock_num_cities())) {
		thisGameState.actionValue[CIT] =
		    genetic_score_node(city_node, TRUE, &thisChromosome,
				       &thisGameState)
		    * (thisChromosome.resourcesValueMatrix[victoryPoints][6]);	/* The relative value of city at this point in the game */
	} else
		thisGameState.actionValue[CIT] = 0;


	/* Best Settlement */
	sett_node =
	    best_settlement_spot(FALSE, &thisChromosome, &thisGameState);
	if ((sett_node != NULL) && stock_num_settlements()) {
		thisGameState.actionValue[SET] =
		    genetic_score_node(sett_node, FALSE, &thisChromosome,
				       &thisGameState);
	} else
		thisGameState.actionValue[SET] = 0;

	/* Best Road to Settlement, that is, best destination to build a Settlement after building a Road */
	road_edge =
	    best_road_spot(&thisChromosome, &thisGameState,
			   &destinationRoadScore);
	if ((road_edge != NULL) && stock_num_roads()
	    && stock_num_settlements()) {
		thisGameState.actionValue[RSET] = destinationRoadScore;
	}
	if (destinationRoadScore < 0)
		destinationRoadScore = 0;

	/* Best Long Road to Settlement, this is, best destination to build a Settlement after building two Roads */
	long_road_edge =
	    best_road_to_road(&thisChromosome, &thisGameState,
			      &destinationLongRoadScore);
	if ((long_road_edge != NULL) && (stock_num_roads() >= 2)
	    && stock_num_settlements()) {
		thisGameState.actionValue[RRSET] =
		    destinationLongRoadScore;
	}
	if (destinationLongRoadScore < 0)
		destinationLongRoadScore = 0;

	/* value of buying Development Card is fixed in the chromosome */
	if (stock_num_develop() > 0)
		thisGameState.actionValue[DEV] =
		    thisChromosome.resourcesValueMatrix[victoryPoints][5];
	else
		thisGameState.actionValue[DEV] = 0;

	/* Check if I am close to the end of the game */
	if (lastMinute()) {	/* Am I at the end of the game? */
		/* If so, the fastest I can do becomes the best right now. Every action has the same value and turn is close to 0 */
		for (i = SET; i <= RRSET; i++) {
			if (thisGameState.actionValue[i])
				thisGameState.actionValue[i] = 10;
		}
		if (thisGameState.actionValue[DEV] != 0)
			thisGameState.actionValue[DEV] = 0;
		turn = 0.01;
	}

	outputGameState(thisGameState);

	printf("Calculating best strategy...\n");
	thisStrategyProfit =
	    bestStrategy(turn, probability, &thisSimulation, thisStrategy,
			 thisGameState, 0, 1, num_players());
	ai_wait();
	printf
	    ("\t\t\t\tSET\tCIT\tDEV\tRSET\tRRSET\tS+SET\tS+CIT\tS+DEV\tS+RSET\tS+RRSET\tC+CIT\tC+DEV\tC+RSET\tC+RRSET\tD+DEV\tD+RSET\tD+RRSET\tR+RSET\tR+RRSET\tRR+RRSET\n");
	printf("\t\tTime to Action:\t");
	for (i = 0; i < NUM_ACTIONS; i++)
		printf("%d\t", thisSimulation.turnsToAction[i]);
	printf("\n");
	outputStrategy(thisStrategy, thisSimulation, thisGameState);

	printf
	    ("Will I do it? Lets see if I can trade to do something better...\n");


	/* Trading code should go here. Now that I know my expected profit, I can see if there is a way to improve it trading */
	printf("Updating trading matrixes...\n");
	updateTradingMatrix(&thisChromosome, thisStrategyProfit,
			    &thisTradingMatrixes, thisGameState, 1);
	/* ai_wait(); */
	if (best_maritime_trade
	    (thisTradingMatrixes, &amount, &trade_away, &want_resource)
	    && can_trade_maritime()) {
		printf
		    ("According to trading matrixes I will trade %d of %d for 1 of %d\n",
		     amount, trade_away, want_resource);
		cb_maritime(amount, trade_away, want_resource);
		return;
	} else
		printf
		    ("According to trading matrixes there is no favorable trade possible\n");





	time_a = thisSimulation.turnsToAction[thisStrategy[0]];
	if (time_a == 0) {	/* I can do what I want NOW */
		/* Under certain uncommon circumstances is possible for bestStrategy to choose an strategy whose first action yields profit 0,
		 * so we should check that there is in fact possible to do what I want to do before trying to do it*/
		printf("Resources:");
		for (i = 0; i < 5; i++)
			printf(" %d ", resource_asset(i));
		printf("\n");
		switch (thisStrategy[0]) {
		case SET:{
				if ((sett_node != NULL)
				    && (stock_num_settlements())) {
					printf("Building Settlement...\n");
					cb_build_settlement(sett_node);
					return;
				}
				break;
			}
		case CIT:{
				if ((city_node != NULL)
				    && (stock_num_cities())) {
					printf("Building City...\n");
					cb_build_city(city_node);
					return;
				}
				break;
			}
		case RSET:{
				if ((road_edge != NULL)
				    && (stock_num_roads())) {
					printf
					    ("Building Road as part of RSET...\n");
					cb_build_road(road_edge);
					return;
				}
				break;
			}
		case RRSET:{
				if ((long_road_edge != NULL)
				    && (stock_num_roads())) {
					printf
					    ("Building Road as part of RRSET...\n");
					cb_build_road(long_road_edge);
					return;
				}
				break;
			}
		case DEV:{
				if (can_buy_develop()) {
					printf
					    ("Buying Development Card...\n");
					cb_buy_develop();
					return;
				}
				break;
			}
		}
	}
	/* Even if it is not possible to do anything completely now now, maybe I can build road anyway, if that does not affect my chosen strategy
	   In that case, I will do it in the best place possible, influenced by my strategy */
	else if ((checkRoadNow
		  (thisStrategy[0], thisStrategy[1], thisGameState))
		 && stock_num_roads()) {
		switch (thisStrategy[0]) {
		case SET:
		case CIT:
		case DEV:
			switch (thisStrategy[1]) {
			case RSET:
				if (destinationRoadScore) {
					printf
					    ("Building Road of RSET in the meantime...\n");
					cb_build_road(road_edge);
					return;
				}
				break;
			case RRSET:
				if (destinationLongRoadScore) {
					printf
					    ("Building Road of RRSET in the meantime...\n");
					cb_build_road(long_road_edge);
					return;
				}
				break;
			default:
				if ((destinationRoadScore)
				    && (destinationRoadScore >=
					destinationLongRoadScore)) {
					printf
					    ("Building Road of RSET in the meantime...\n");
					cb_build_road(road_edge);
					return;
				} else if ((destinationLongRoadScore)
					   && (destinationLongRoadScore >
					       destinationRoadScore)) {
					printf
					    ("Building Road of RRSET in the meantime...\n");
					cb_build_road(long_road_edge);
					return;
				}
				break;
			}
			/* IMPROVEMENT: Instead of building the best road, If thisStrategy[1] is RSET or RRSET it should chose that road to build
			   if ((destinationRoadScore)&&(destinationRoadScore>=destinationLongRoadScore)) {
			   cb_build_road(road_edge);
			   return;
			   }
			   else if ((destinationLongRoadScore)&&(destinationLongRoadScore>destinationRoadScore)) {
			   cb_build_road(long_road_edge);
			   return;
			   }
			   break; */
		case RSET:
			if (destinationRoadScore) {
				printf
				    ("Building Road of RSET in the meantime...\n");
				cb_build_road(road_edge);
				return;
			}
			break;
		case RRSET:
			if (destinationLongRoadScore) {
				printf
				    ("Building Road of RRSET in the meantime...\n");
				cb_build_road(long_road_edge);
				return;
			}
			break;
		default:
			break;
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
	printf("Finishing my turn...\n");
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
static void genetic_place_robber(void)
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

static void genetic_steal_building(void)
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

static void genetic_free_road(void)
{
	Edge *e;
	float destinationScore;
	struct gameState_t myGameState;

	/*
	   resource_values_t resval;
	   reevaluate_resources(&resval);
	 */
	reevaluate_gameState_supply_matrix_and_resources(&myGameState);

	e = best_road_spot(&thisChromosome, &myGameState,
			   &destinationScore);

	if (e == NULL) {
		e = best_road_to_road(&thisChromosome, &myGameState,
				      &destinationScore);
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

static void genetic_year_of_plenty(const gint bank[NO_RESOURCE])
{
	gint want[NO_RESOURCE];
	gint assets[NO_RESOURCE];
	int i;
	int r1, r2;
	/* resource_values_t resval; */
	struct gameState_t myGameState;

	ai_wait();
	for (i = 0; i < NO_RESOURCE; i++) {
		want[i] = 0;
		assets[i] = resource_asset(i);
	}


	/* what two resources do we desire most */
	/* reevaluate_resources(&resval); */
	reevaluate_gameState_supply_matrix_and_resources(&myGameState);

	r1 = resource_desire(assets, &thisChromosome, &myGameState);

	/* If we don't desire anything anymore, ask for a road.
	 * This happens if we have at least 2 of each resource
	 */
	if (r1 == NO_RESOURCE)
		r1 = BRICK_RESOURCE;

	assets[r1]++;

	/* reevaluate_resources(&resval); */
	reevaluate_gameState_supply_matrix_and_resources(&myGameState);

	r2 = resource_desire(assets, &thisChromosome, &myGameState);

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
static gint determine_monopoly_resource()
{
	gint assets[NO_RESOURCE];
	int i;
	gint most_desired;
	gint most_wildcards;
	resource_values_t resval;
	struct gameState_t myGameState;



	for (i = 0; i < NO_RESOURCE; i++)
		assets[i] = resource_asset(i);

	/* order resources by preference */
	reevaluate_resources(&resval);
	reevaluate_gameState_supply_matrix_and_resources(&myGameState);

	/* try to get something we need */
	most_desired =
	    resource_desire(assets, &thisChromosome, &myGameState);

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

static void genetic_monopoly(void)
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

#if 0
static int least_valuable(gint assets[NO_RESOURCE],
			  resource_values_t * resval)
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
#endif

/*
 * Which resource do we desire the least?
 */

/** Calculates the value of every possible action and sets them in myGameState->actionValues */
static void reevaluate_gameState_actionValues(const struct chromosome_t
					      *myChromosome, struct gameState_t
					      *myGameState)
{
	Node *city_node;
	Node *sett_node;
	Edge *road_edge;
	Edge *long_road_edge;
	float destinationRoadScore, destinationLongRoadScore;
	int victoryPoints = player_get_score(my_player_num());
	if (victoryPoints > 9)
		victoryPoints = 9;

	/* I need to know the value of the best possible Settlement, City, Road to Settlement and Long Road to Settlement to set thisGameState.actionValue for each of them */


	/* Best City */
	city_node = best_city_spot(myChromosome, myGameState);
	if ((city_node != NULL) && (stock_num_cities())) {
		myGameState->actionValue[CIT] =
		    genetic_score_node(city_node, TRUE, myChromosome,
				       myGameState);
	} else
		myGameState->actionValue[CIT] = 0;


	/* Best Settlement */
	sett_node = best_settlement_spot(FALSE, myChromosome, myGameState);
	if ((sett_node != NULL) && stock_num_settlements()) {
		myGameState->actionValue[SET] =
		    genetic_score_node(sett_node, FALSE, myChromosome,
				       myGameState);
	} else
		myGameState->actionValue[SET] = 0;

	/* Best Road to Settlement, that is, best destination to build a Settlement after building a Road */
	road_edge =
	    best_road_spot(myChromosome, myGameState,
			   &destinationRoadScore);
	if ((road_edge != NULL) && stock_num_roads()
	    && stock_num_settlements()) {
		myGameState->actionValue[RSET] = destinationRoadScore;
	} else
		myGameState->actionValue[RSET] = 0;

	/* Best Long Road to Settlement, this is, best destination to build a Settlement after building two Roads */
	long_road_edge =
	    best_road_to_road(myChromosome, myGameState,
			      &destinationLongRoadScore);
	if ((long_road_edge != NULL) && (stock_num_roads() >= 2)
	    && stock_num_settlements()) {
		myGameState->actionValue[RRSET] = destinationLongRoadScore;
	} else
		myGameState->actionValue[RRSET] = 0;

	/* value of buying Development Card is fixed in the chromosome */
	myGameState->actionValue[DEV] =
	    myChromosome->resourcesValueMatrix[victoryPoints][5];
	return;

}

/* given totalDiscards resources I should discard it sets the array todiscard to that resources */
static int update_todiscard_resources(int totalDiscards, const struct chromosome_t
				      *myChromosome,
				      struct gameState_t *myGameState,
				      gint todiscard[NO_RESOURCE])
{

	int giveaway, give, i;
	float profitLossAfterDiscard, minimumProfitLoss;
	float turn = myChromosome->turn;
	float prob = myChromosome->probability;
	struct simulationsData_t thisSimulation;
	strategy_t thisStrategy;
	float actualProfit;

	printf("Resources:");
	for (i = 0; i < 5; i++)
		printf(" %d ", resource_asset(i));
	printf("\n");

	int discards = 0;

	giveaway = -1;
	for (i = 0; i < NO_RESOURCE; i++) {
		todiscard[i] = 0;
	}

	reevaluate_gameState_supply_matrix_and_resources(myGameState);	/* Also set resourcesAlreadyHave to the real values */
	reevaluate_gameState_actionValues(myChromosome, myGameState);


	while (discards < totalDiscards) {
		/* First I calculate the best that I could do with my resources, before giving anything away */
		minimumProfitLoss = 999;
		actualProfit =
		    bestStrategy(turn, prob, &thisSimulation, thisStrategy,
				 *myGameState, 0, 1, num_players());
		for (give = 0; give <= 4; give++) {	/* which resource is the best to get rid of it? */
			if (myGameState->resourcesAlreadyHave[give] >= 1) {	/* I have something to discard */
				/*printf("Testing resource %d on discard %d\n", give,discards+1); */
				myGameState->resourcesAlreadyHave[give]--;
				profitLossAfterDiscard =
				    actualProfit - bestStrategy(turn, prob,
								&thisSimulation,
								thisStrategy,
								*myGameState,
								0, 0,
								num_players
								());
				if (profitLossAfterDiscard < minimumProfitLoss) {	/* this discard is the best so far */
					minimumProfitLoss =
					    profitLossAfterDiscard;
					giveaway = give;
				}
				myGameState->resourcesAlreadyHave[give]++;
			}
		}
		todiscard[giveaway]++;
		myGameState->resourcesAlreadyHave[giveaway]--;
		discards++;
		printf
		    ("The discard number %d of the total %d I have to discard will be ",
		     discards, totalDiscards);
		printResource(giveaway);
		printf(" (and now I have %d of it left)\n",
		       myGameState->resourcesAlreadyHave[giveaway]);
	}

	if ((giveaway == -1) || (discards != totalDiscards))
		printf("giveaway=-1!!! or wrong number of discards\n");
	return (discards);
	/* Should never get here */
	g_assert_not_reached();
	return 0;
}


/*
 * A seven was rolled. we need to discard some resources :(
 *
 */
static void genetic_discard(int num)
{
	gint todiscard[NO_RESOURCE];
	int i;



	struct gameState_t myGameState;

	update_todiscard_resources(num, &thisChromosome, &myGameState,
				   todiscard);
	printf("Resources:");
	for (i = 0; i < 5; i++)
		printf(" %d ", resource_asset(i));
	printf("\n");
	for (i = 0; i < NO_RESOURCE; i++)
		printf("Resource %d discard %d\n", i, todiscard[i]);

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

static void genetic_quote_start(void)
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
	struct gameState_t myGameState;

	/* This code is temporary, it should use bestStrategy to find out if the expected profit is higher with the new resources */
	reevaluate_gameState_supply_matrix_and_resources(&myGameState);

	if (!free_offer) {
		/* don't give away cards we have only once */
		if (assets[give] <= 1) {
			return 0;
		}

		/* make it as if we don't have what we're trading away */
		assets[give] -= 1;
	}

	for (n = 1; n <= 3; ++n) {
		/* do i need something more for something? */
		if (!should_buy
		    (assets, BUY_CITY, &thisChromosome, &myGameState,
		     need)) {
			if ((res = which_resource(need)) == take
			    && need[res] == n)
				break;
		}
		if (!should_buy
		    (assets, BUY_SETTLEMENT, &thisChromosome, &myGameState,
		     need)) {
			if ((res = which_resource(need)) == take
			    && need[res] == n)
				break;
		}
		if (!should_buy
		    (assets, BUY_ROAD, &thisChromosome, &myGameState,
		     need)) {
			if ((res = which_resource(need)) == take
			    && need[res] == n)
				break;
		}
		if (!should_buy
		    (assets, BUY_DEVEL_CARD, &thisChromosome, &myGameState,
		     need)) {
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
	reevaluate_resources(&resval);
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

static void genetic_consider_quote(G_GNUC_UNUSED gint partner,
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

static void genetic_setup(gint num_settlements, gint num_roads)
{
	ai_wait();
	if (num_settlements > 0)
		genetic_setup_house();
	else if (num_roads > 0)
		genetic_setup_road();
	else
		cb_end_turn();
}

static void genetic_roadbuilding(gint num_roads)
{
	ai_wait();
	if (num_roads > 0)
		genetic_free_road();
	else
		cb_end_turn();
}

static void genetic_discard_add(gint player_num, gint discard_num)
{
	ai_chat_discard(player_num, discard_num);
	if (player_num == my_player_num()) {
		ai_wait();
		genetic_discard(discard_num);
	}
}

static void genetic_gold_choose(gint gold_num, const gint * bank)
{
	resource_values_t resval;
	gint assets[NO_RESOURCE];
	gint want[NO_RESOURCE];
	gint my_bank[NO_RESOURCE];
	gint i;
	int r1;

	struct gameState_t myGameState;
	reevaluate_gameState_supply_matrix_and_resources(&myGameState);

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

		r1 = resource_desire(assets, &thisChromosome,
				     &myGameState);
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

static void outputChromosome(void)
{
	int i, j;
	for (i = 0; i <= 9; i++) {
		for (j = 0; j <= 7; j++) {
			printf("%.3f\t",
			       thisChromosome.resourcesValueMatrix[i][j]);
		}
		printf("\n");
	}
	printf("%.3f\t%.3f\t%.3f\n", thisChromosome.depreciation_constant,
	       thisChromosome.turn, thisChromosome.probability);
}

static void genetic_game_over(gint player_num, G_GNUC_UNUSED gint points)
{
	if (player_num == my_player_num()) {
		printf
		    ("FINAL RESULT GENETIC: I won!  (%s) with %2d points using ",
		     my_player_name(), player_get_score(my_player_num()));
		if (default_chromosome_used) {
			printf("DEFAULT\n");
		} else
			printf("%s\n", chromosomeFile);
		outputChromosome();
		/* AI chat when it wins */
		ai_chat(N_("Yippie!"));
	} else {
		printf
		    ("FINAL RESULT GENETIC: I lost! (%s) with %2d points using ",
		     my_player_name(), player_get_score(my_player_num()));
		if (default_chromosome_used) {
			printf("DEFAULT\n");
		} else
			printf("%s\n", chromosomeFile);
		outputChromosome();
		/* AI chat when another player wins */
		ai_chat(N_("My congratulations"));
	}
	cb_disconnect();
}

static void genetic_init_game(void)
{
	int i, j;
	struct chromosome_t tempChromosome;
	FILE *chromFilePointer;
	char line[80];

	if (chromosomeFile == NULL) {
		printf("No chromosome file specified, default used.\n");
		return;
	}
	printf("Reading chromosome from file: %s\n", chromosomeFile);
	if ((chromFilePointer = fopen(chromosomeFile, "r")) == NULL) {
		printf
		    ("Opening of chromosome file %s failed, default used\n",
		     chromosomeFile);
		return;
	}
	printf("Reading chromosome file: %s...\n", chromosomeFile);
	for (i = 0; i <= 9; i++) {
		if (fgets(line, 80, chromFilePointer) == NULL) {
			printf
			    ("Some values in the chromosome are missing! Using default then...\n");
			fclose(chromFilePointer);
			return;
		}
		if (sscanf
		    (line, "%f %f %f %f %f %f %f %f",
		     &tempChromosome.resourcesValueMatrix[i][0],
		     &tempChromosome.resourcesValueMatrix[i][1],
		     &tempChromosome.resourcesValueMatrix[i][2],
		     &tempChromosome.resourcesValueMatrix[i][3],
		     &tempChromosome.resourcesValueMatrix[i][4],
		     &tempChromosome.resourcesValueMatrix[i][5],
		     &tempChromosome.resourcesValueMatrix[i][6],
		     &tempChromosome.resourcesValueMatrix[i][7]) != 8) {
			printf
			    ("Some values in the chromosome are missing! Using default then...\n");
			fclose(chromFilePointer);
			return;
		}
		printf("%s", line);
	}
	if (fgets(line, 80, chromFilePointer) == NULL) {
		printf
		    ("Some values in the chromosome are missing! Using default then...\n");
		fclose(chromFilePointer);
		return;
	}
	if (sscanf
	    (line, "%f %f %f", &(tempChromosome.depreciation_constant),
	     &(tempChromosome.turn), &(tempChromosome.probability)) != 3) {
		printf
		    ("Some values in the chromosome are missing! Using default then...\n");
		fclose(chromFilePointer);
		return;
	}
	printf("%s", line);
	printf("Finishing reading the chromosome\n");
	fclose(chromFilePointer);
	default_chromosome_used = FALSE;
	for (i = 0; i <= 9; i++) {
		for (j = 0; j <= 7; j++) {
			thisChromosome.resourcesValueMatrix[i][j] =
			    tempChromosome.resourcesValueMatrix[i][j];
		}
	}
	thisChromosome.depreciation_constant =
	    tempChromosome.depreciation_constant;
	thisChromosome.turn = tempChromosome.turn;
	thisChromosome.probability = tempChromosome.probability;


	return;
}

void genetic_init(void)
{
	callbacks.setup = &genetic_setup;
	callbacks.turn = &genetic_turn;
	callbacks.robber = &genetic_place_robber;
	callbacks.steal_building = &genetic_steal_building;
	callbacks.roadbuilding = &genetic_roadbuilding;
	callbacks.plenty = &genetic_year_of_plenty;
	callbacks.monopoly = &genetic_monopoly;
	callbacks.discard_add = &genetic_discard_add;
	callbacks.quote_start = &genetic_quote_start;
	callbacks.quote = &genetic_consider_quote;
	callbacks.gold_choose = &genetic_gold_choose;
	callbacks.game_over = &genetic_game_over;

	callbacks.init_game = &genetic_init_game;
}
