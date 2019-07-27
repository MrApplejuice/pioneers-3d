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

#ifndef genetic_core_h
#define genetic_core_h

/** Number of simulations, could be raised to achieve better accuracy if computing time allows it*/
#define MAX_SIMS 100
/** Number of possible actions, single or paired -> 5 individual (SET,CIT,DEV, RSET RRSET) + 5*5 combined (SET+CIT,SET+DEV,etc) -10 of which are redundant (SET+CIT=CIT+SET) = 20 */
#define NUM_ACTIONS 20
#define MAX_TURNS 100

enum action { SET, CIT, DEV, RSET, RRSET };	/* Possible individual actions: Settlement, City, Development Card, Road+Settlement, Road+Road+Settlement */
typedef enum action strategy_t[2];	/* Pair of preferred actions to carry out in the next turns, possibly the first (or even the second) right now */

typedef float tradingMatrix_t[5][5];	/* If it is set to a positive number it means that I am interested in trading i resource in exchange of j resource */
struct tradingMatrixes_t {
	tradingMatrix_t internalTrade;	/* 1:1 trade with other players */
	tradingMatrix_t bankTrade;	/* 4:1 trade with the bank */
	tradingMatrix_t specificResource;	/* 2:1 trade through a port */
	tradingMatrix_t genericResource;	/* 3:1 trade through generic port */
};

/** A structure of type simulationsData will be used to hold the data of the MAX_SIMS simulations*/
struct simulationsData_t {
	int resourcesPool[MAX_SIMS][5];	/* Resources for every simulation */
	int conditionsMet[MAX_SIMS][NUM_ACTIONS];
	/* Boolean matrix that will hold true for every simulation that meets the requirements of certain action
	 * Possible actions are SET,CIT,DEV,RSET,RRSET, SET+SET,SET+CIT,SET+DEV...,CIT+SET,CIT+CIT,...RRSET+RSET,RRSET+RRSET
	 * 5 independent actions+15 combined pairs of actions.
	 * It is important to note that the order in which actions are represented in conditionsMet should match the order they are in resourcesNeededForAction*/
	int numberOfSimulationsOK[NUM_ACTIONS];	/* Number of simulations that meet the requirementS for every action */
	int turnsToAction[NUM_ACTIONS];	/* Number of turns needed for every action or pair of actions to reach the required probability of getting its resources */
	int timeCombinedAction[5][5];	/* It will hold the data of turnsToAction regarding combined actions, it is for ease of access, this information is already hold in turnsToAction */
};

/** A structure of type gameState_t will hold the information of my actual state in the game
 *resourcesSupply, resourcesAlreadyHave and actionValue are variables whose value depends on the actual situation of the player in the game */
struct gameState_t {
	int resourcesSupply[11][5];	/* Depends on the location of settlements and cities in the map, 11 different dice outcomes by 5 different resources */
	int resourcesAlreadyHave[5];	/* Resources I already have of Brick, Lumber, Grain, Wool and Ore */
	float actionValue[5];	/* Benefit of doing best SET, best CIT, pick DEV, best RSET and best RRSET */
};

/** Will hold the values of the chromosome that dictate how certain algorithm plays. They are fixed throughout the whole game */
struct chromosome_t {
	float resourcesValueMatrix[10][8];	/* value of Brick Lumber, Grain, Wool, Ore, Development Card, City and Port depending on my Victory Points (from 0 to 9) */
	float depreciation_constant;	/* the higher the depreciation_constant k is the more value it will give to resources it does not have at the moment, 0<=k<=1 */
	float turn;		/* the turn at which it will calculate the profit of following a particular strategy, 0<=turn<25 */
	float probability;	/* it will calculate how many turns it needs to perform something with this level of confidence, 0<probability<1 */
};

int actualAverageResourcesSupply(int resource,
				 const struct gameState_t *myGameState);
float resourcesIncrementValue(int increment, int resource,
			      int VictoryPoints,
			      const struct chromosome_t *chromosome,
			      const struct gameState_t *myGameState,
			      int port);

void printAction(enum action oneAction);
void printResource(int resource);
float bestStrategy(float turn, float probability,
		   struct simulationsData_t *Data, strategy_t myStrategy,
		   struct gameState_t myGameState, int showSimulation,
		   int myTurn, int num_players);

int checkRoadNow(enum action firstAction, enum action secondAction,
		 struct gameState_t myGameState);

#endif
