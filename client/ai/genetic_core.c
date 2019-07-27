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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include "genetic_core.h"

static int totalResources(const struct gameState_t *myGameState);
static float totalAverageResourceSupplyPerTurn(const struct gameState_t
					       *myGameState);
static float depreciateStrategyValue(int num_resources, int num_players,
				     float ARSperTurn);
static float depreciationFunction(float k, int actualARS, int port);
static int enoughResources(int sim, int act,
			   struct simulationsData_t *Data);
static void updateSimulation(int sim, float probability,
			     struct simulationsData_t *Data);
static void updateConditionsMet(float probability,
				struct simulationsData_t *Data);
static void updateTurnsToAction(float probability, int currentTurn,
				struct simulationsData_t *Data);
//static void outputSims(int number, int turn, struct simulationsData_t *Data);
static void set_timeCombinedAction(struct simulationsData_t *Data);
static void numberOfTurnsForProbability(float probability,
					struct simulationsData_t *Data,
					struct gameState_t myGameState,
					int showSimulation);
static float strategyProfit(float time_a, float time_b, float turn,
			    strategy_t oneStrategy,
			    struct gameState_t myGameState, int myTurn,
			    int num_players);

static int resourcesNeededForAction[NUM_ACTIONS][5] = {
	/*Resources of every type needed to perform every action possible */
	{1, 1, 0, 1, 1}, /*SET*/ {0, 2, 3, 0, 0}, /*CIT*/ {0, 1, 1, 1, 0}, /*DEV*/ {2, 1, 0, 1, 2}, /*RSET*/ {3, 1, 0, 1, 3}, /*RRSET*/ {2, 2, 0, 2, 2},	/*SET+SET */
	{1, 3, 3, 1, 1},	/*SET+CIT */
	{1, 2, 1, 2, 1},	/*SET+DEV */
	{3, 2, 0, 2, 3},	/*SET+RSET */
	{4, 2, 0, 2, 4},	/*SET+RRSET */

	{0, 4, 6, 0, 0},	/*CIT+CIT */
	{0, 3, 4, 1, 0},	/*CIT+DEV */
	{2, 3, 3, 1, 2},	/*CIT+RSET */
	{3, 3, 3, 1, 3},	/*CIT+RRSET */

	{0, 2, 2, 2, 0},	/*DEV+DEV */
	{2, 2, 1, 2, 2},	/*DEV+RSET */
	{3, 2, 1, 2, 3},	/*DEV+RRSET */

	{4, 2, 0, 2, 4},	/*RSET+RSET */
	{5, 2, 0, 2, 5},	/*RSET+RRSET */

	{6, 2, 0, 2, 6}		/*RRSET+RRSET */
};				/*First index is the action, second index the kind of resource */

int totalResources(const struct gameState_t *myGameState)
{
	int i, total;
	total = 0;
	for (i = 0; i < 5; i++) {
		total += myGameState->resourcesAlreadyHave[i];
	}
	return (total);
}


int actualAverageResourcesSupply(int resource,
				 const struct gameState_t *myGameState)
{
	/*returns the ARS of a particular resource looking at resourcesSupply matrix */
	int i, total, multiplier;
	total = 0;
	for (i = 0; i <= 10; i++) {	/*possible dice rolls */
		switch (i) {
		case 0:	/*rolls 2 or 12, the odds are 1 each 36 turns */
		case 10:
			multiplier = 1;
			break;
		case 1:	/*rolls 3 or 11, the odds are 2 each 36 turns, etc. */
		case 9:
			multiplier = 2;
			break;
		case 2:
		case 8:
			multiplier = 3;
			break;
		case 3:
		case 7:
			multiplier = 4;
			break;
		case 4:
		case 6:
			multiplier = 5;
			break;
		case 5:
			multiplier = 0;
			break;
		}
		total +=
		    (myGameState->resourcesSupply[i][resource]) *
		    multiplier;
	}
	return (total);
}

float totalAverageResourceSupplyPerTurn(const struct gameState_t
					*myGameState)
{
	int i;
	int totalActualARS = 0;
	float total;
	for (i = 0; i < 5; i++) {
		totalActualARS +=
		    actualAverageResourcesSupply(i, myGameState);
	}
	total = totalActualARS / 36.0;
	return (total);
}

float depreciateStrategyValue(int num_resources, int num_players,
			      float ARSperTurn)
{

	/*If a strategy requires ending my turn with too many resources, the risk of being affected by the thief has to be taken in account. */
	/*If I already have 8 or more resources, the thief could affect me num_players times. But if I am close to that limit there is also a chance tto be affected too */
	int thief_oportunity = 0;	/*number of times the apparition of the thief could catch me with too many resources in my hand */
	float probabilityNoThief;
	int i;
	for (i = 0; i < num_players; i++) {
		if ((num_resources + (i * ARSperTurn)) >= 8) {
			thief_oportunity = num_players - i;
			break;
		}
	}
	/*Alternate way:
	 * thief_oportunity=floor((8-num_resources)/ARSperTurn)
	 */
	probabilityNoThief = pow(5.0 / 6.0, thief_oportunity);
	/*I will assume for simplicity that the best strategy I would be able to do with half my resources has half the value. */
	return ((1 * probabilityNoThief) +
		(0.5 * (1 - probabilityNoThief)));
	/*For a typical 4 player game, ending my turn having 8 or more resources means a depreciation of  1*0.482 + 0.5*0.517 = 0.741 of the calculated strategy */
}


float depreciationFunction(float k, int actualARS, int port)
/*depreciation of the value of a resource depending on genetic value k and my actual supply of that resource
 * k=0 means no depreciation at all no matter my actualARS, and the higher the k and my actualARS the closer it will get to 0.25 (for port=4, bank trade)
 * It considers that this should be the maximum depreciation a resource could suffer
 * having in mind that you could always trade any resource on a 4:1 basis.
    port is the trading ratio I have for that resource, and marks the maximum depreciation a resource could suffer no matter the amount of it I already have*/
{
	return ((k * actualARS + 1) / (port * k * actualARS + 1));
}


float resourcesIncrementValue(int increment, int resource,
			      int VictoryPoints,
			      const struct chromosome_t *chromosome,
			      const struct gameState_t *myGameState,
			      int port)
/*return the value it gives to an increment in the supply a particular resource, that depends on the increment itself, my actual supplies
 *of it and values determined by the chromosome*/
{
	float value, weight;
	int actualARS;
	float depreciation;
	actualARS = actualAverageResourcesSupply(resource, myGameState);
	depreciation = depreciationFunction(chromosome->depreciation_constant, actualARS, port);	/*Its values go from (0.25..1] */
	weight = chromosome->resourcesValueMatrix[VictoryPoints][resource];	/*value given by the chromosome to that resource at this point in the game */
	value = increment * depreciation * weight;
	return (value);

}


void printAction(enum action oneAction)
{
	switch (oneAction) {
	case 0:
		printf("Settlement");
		break;
	case 1:
		printf("City");
		break;
	case 2:
		printf("Development Card");
		break;
	case 3:
		printf("Road to Settlement");
		break;
	case 4:
		printf("Long Road to Settlement");
		break;
	}
}

void printResource(int resource)
{
	switch (resource) {
	case 0:
		printf("Brick");
		break;
	case 1:
		printf("Grain");
		break;
	case 2:
		printf("Ore");
		break;
	case 3:
		printf("Wool");
		break;
	case 4:
		printf("Lumber");
		break;
	}
}

int enoughResources(int sim, int act, struct simulationsData_t *Data)
{
	/*Returns TRUE if there are enough resources in simulation sim to perform action action */
	int resource;
	for (resource = 0; resource < 5; resource++) {	/*Check for every type of resource */
		if (Data->resourcesPool[sim][resource] <
		    resourcesNeededForAction[act][resource])
			return (0);
	}
	return (1);
}

void updateSimulation(int sim, float probability,
		      struct simulationsData_t *Data)
{
	/*Checks resourcesPool to update conditionsMet for that simulation for every action and numberOfSimulationsOK */
	int act;
	for (act = 0; act < NUM_ACTIONS; act++) {
		if ((Data->numberOfSimulationsOK[act] <
		     (MAX_SIMS * probability))
		    && (Data->conditionsMet[sim][act] != 1)) {
			/*If there already are enough simulations OK for this action or conditions were already met it doesn't need to check again */
			if (enoughResources(sim, act, Data)) {
				Data->conditionsMet[sim][act] = 1;
				Data->numberOfSimulationsOK[act]++;
			}
		}
	}
}

void updateConditionsMet(float probability, struct simulationsData_t *Data)
{
	/*Update every simulation in conditionsMet */
	/*We need probability in order to avoid checking for actions unnecessarily */
	int sim;
	for (sim = 0; sim < MAX_SIMS; sim++) {
		updateSimulation(sim, probability, Data);
	}
	/*At this point numberOfSimulationsOK will hold, for avery action, the number of simulations that, up to the turn calculated, meet the requirements to do it */
}

void updateTurnsToAction(float probability, int currentTurn,
			 struct simulationsData_t *Data)
{
	/*If the percentage of simulations OK for an action is over probability will set turnsToAction for that action to turn */
	int act;
	for (act = 0; act < NUM_ACTIONS; act++) {
		if ((Data->turnsToAction[act] == MAX_TURNS) &&	/*It will set only the first time it reaches probability */
		    (Data->numberOfSimulationsOK[act] >=
		     MAX_SIMS * probability))
			Data->turnsToAction[act] = currentTurn;
	}
}

#if 0

/*This should ne rewriten in orden to take acount of the order change in resources (Now it should go Br,Gr,Or,Wo and Lu)*/

void outputSims(int number, int turn, struct simulationsData_t *Data)
{
	int resource, act, simulation;

	//system("clear");
	printf
	    ("BRICK\tLUMBER\tGRAIN\tWOOL\tORE\t\tSET\tCIT\tDEV\tRSET\tRRSET\tS+SET\tS+CIT\tS+DEV\tS+RSET\tS+RRSET\tC+CIT\tC+DEV\tC+RSET\tC+RRSET\tD+DEV\tD+RSET\tD+RRSET\tR+RSET\tR+RRSET\tRR+RRSET\n");
	for (simulation = 0; simulation < number; simulation++) {
		for (resource = 0; resource < 5; resource++) {
			printf("%d\t",
			       Data->resourcesPool[simulation][resource]);
		}
		printf("\t");
		for (act = 0; act < NUM_ACTIONS; act++) {
			printf("%d\t",
			       Data->conditionsMet[simulation][act]);
		}
		printf("\n");
	}
	printf
	    ("BRICK\tLUMBER\tGRAIN\tWOOL\tORE\t\tSET\tCIT\tDEV\tRSET\tRRSET\tS+SET\tS+CIT\tS+DEV\tS+RSET\tS+RRSET\tC+CIT\tC+DEV\tC+RSET\tC+RRSET\tD+DEV\tD+RSET\tD+RRSET\tR+RSET\tR+RRSET\tRR+RRSET\n");
	printf("\nNumber of Simulations OK for every action->");
	for (act = 0; act < NUM_ACTIONS; act++) {
		printf("\t%d", Data->numberOfSimulationsOK[act]);
	}
	printf("\nTurns to Action for every action->\t");
	for (act = 0; act < NUM_ACTIONS; act++) {
		printf("\t%d", Data->turnsToAction[act]);
	}
	printf("\n");
	printf("Turn %d\n", turn);
}
#endif

void set_timeCombinedAction(struct simulationsData_t *Data)
{
	/*      Puts the information of turnsToAction in a way that is easier to access from bestStrategy procedure
	 *                  SET     CIT     DEV     RSET    RRSET
	 *          SET     5       6       7       8       9   ------->for example, means turnsToAction[9];
	 *          CIT     6       10      11      12      13
	 *          DEV     7       11      14      15      16
	 *          RSET    8       12      15      17      18
	 *          RRSET   9       13      16      18      19  */

	Data->timeCombinedAction[0][0] = Data->turnsToAction[5];
	Data->timeCombinedAction[0][1] = Data->turnsToAction[6];
	Data->timeCombinedAction[0][2] = Data->turnsToAction[7];
	Data->timeCombinedAction[0][3] = Data->turnsToAction[8];
	Data->timeCombinedAction[0][4] = Data->turnsToAction[9];
	Data->timeCombinedAction[1][0] = Data->turnsToAction[6];
	Data->timeCombinedAction[1][1] = Data->turnsToAction[10];
	Data->timeCombinedAction[1][2] = Data->turnsToAction[11];
	Data->timeCombinedAction[1][3] = Data->turnsToAction[12];
	Data->timeCombinedAction[1][4] = Data->turnsToAction[13];
	Data->timeCombinedAction[2][0] = Data->turnsToAction[7];
	Data->timeCombinedAction[2][1] = Data->turnsToAction[11];
	Data->timeCombinedAction[2][2] = Data->turnsToAction[14];
	Data->timeCombinedAction[2][3] = Data->turnsToAction[15];
	Data->timeCombinedAction[2][4] = Data->turnsToAction[16];
	Data->timeCombinedAction[3][0] = Data->turnsToAction[8];
	Data->timeCombinedAction[3][1] = Data->turnsToAction[12];
	Data->timeCombinedAction[3][2] = Data->turnsToAction[15];
	Data->timeCombinedAction[3][3] = Data->turnsToAction[17];
	Data->timeCombinedAction[3][4] = Data->turnsToAction[18];
	Data->timeCombinedAction[4][0] = Data->turnsToAction[9];
	Data->timeCombinedAction[4][1] = Data->turnsToAction[13];
	Data->timeCombinedAction[4][2] = Data->turnsToAction[16];
	Data->timeCombinedAction[4][3] = Data->turnsToAction[18];
	Data->timeCombinedAction[4][4] = Data->turnsToAction[19];
}

void numberOfTurnsForProbability(float probability,
				 struct simulationsData_t *Data,
				 struct gameState_t myGameState,
				 int showSimulation)
{
	/* Sets turnsToAction values to the number of turns needed to have a certain probability to get the resources needed to perform each NUM_ACTIONS possible actions
	 * It does so by simulating MAX_SIMS times the dice outcomes of a single turn and checking how many of those simulations would fulfill the requirements of
	 * resourcesNeededForAction of every action, and updating numberOfSimulationsOK and conditionsMet matrix consequently
	 * When the percentage of simulations that meet the requirements for a certain action is over probability, then it means that given that amount of turns,
	 * then that percentage of simulations would fulfill those requirements, and it will set that number of turns for that action in turnsToAction.
	 * At the end of the process turnsToAction will hold the number of turns needed for every possible action to be performed with the required probability.
	 * The number of simulations MAX_SIMS could be easily increased in order to get a more accurate simulation process*/

	int currentTurn = 0;
	int simulation;
	int dice_roll1, dice_roll2, dice_roll;
	int i, j;

	/*Initialize Data->resourcesPool to resourcesAlreadyHave[] for every resource and conditionsMet to FALSE for every action */
	for (i = 0; i < MAX_SIMS; i++) {
		for (j = 0; j < 5; j++) {
			Data->resourcesPool[i][j] =
			    myGameState.resourcesAlreadyHave[j];
		}
		for (j = 0; j < NUM_ACTIONS; j++) {
			Data->conditionsMet[i][j] = 0;
		}
	}
	/*Init turnsToAction to the maximum and numberOfSimulationsOK to 0  */
	for (i = 0; i < NUM_ACTIONS; i++) {
		Data->turnsToAction[i] = MAX_TURNS;
		Data->numberOfSimulationsOK[i] = 0;
	}
	updateConditionsMet(probability, Data);	/*Some conditions could already be met at the beginning */
	updateTurnsToAction(probability, currentTurn, Data);
	if (showSimulation) {
		/*
		   outputSims(30, currentTurn, Data);
		   printf("Press any key to run the simulation");
		   getchar();
		 */
	}

	/*Just to avoid the warning */
	/*srandom(time(NULL)); */
	while (currentTurn < MAX_TURNS) {	/*It will simulate up to MAX_TURNS-1 for simplicity sake. */
		/*Should it be optimized? What is the chance of being able to perform everything in a number of turns before the maximum? Maybe with a very low level of probability and under
		 *certain circumstances, but very uncommon in any case.*/
		currentTurn++;
		for (simulation = 0; simulation < MAX_SIMS; simulation++) {
			/* For every simulation it rolls the dice and increases its resources accordingly */
			dice_roll1 = g_random_int_range(1, 7);	/*(random() % 6) + 1; */
			dice_roll2 = g_random_int_range(1, 7);	/*(random() % 6) + 1; */
			dice_roll = dice_roll1 + dice_roll2;
			/*printf("\n(%d+%d)=%d\n ",dice_roll1,dice_roll2,dice_roll); */
			/*updates resourcesPool for this simulation */
			for (i = 0; i <= 4; i++) {
				Data->resourcesPool[simulation][i] =
				    Data->resourcesPool[simulation][i] +
				    myGameState.resourcesSupply[dice_roll -
								2][i];
				/*printResource(i);printf("+%d=",resourcesSupply[dice_roll-2][i]);printf("%d ",Data->resourcesPool[simulation][i]); */
			}
		}
		/*End of all the simulations for this turn, all simulations have their resourcesPool resources updated according to their dice rolls.
		 *Now check for every simulation if conditions are met and update it, and numberOfSimulationsOK accordingly*/
		updateConditionsMet(probability, Data);
		/*Check for every action if it is OK enough times and update turnsToAction for that action to turn if needed */
		updateTurnsToAction(probability, currentTurn, Data);
		/*
		   if (showSimulation) {
		   outputSims(30, currentTurn, Data);

		   } */
	}			/*while */
	set_timeCombinedAction(Data);
	return;
}

float strategyProfit(float time_a, float time_b, float turn,
		     strategy_t oneStrategy,
		     struct gameState_t myGameState, int myTurn,
		     int num_players)
{
	/*Returns the benefit I get in a given turn if I try to do firstAction ending at time_a and then secondAction ending at time_b
	 * time_a, time_b and turn should be possitive numbers, turn>=0, time_b>=time_a>=0*/
	float profit;
	float firstActionValue = myGameState.actionValue[oneStrategy[0]];
	float secondActionValue = myGameState.actionValue[oneStrategy[1]];
	float m1, m2;
	float tooManyResDepreciation = 1;
	float ARSperTurn;
	int endOfTurnResources;	/*With how many resources I am gonna finish my turn with? */
	int roadsWillBuild;
	if (firstActionValue == 0)
		return 0;	/*It is impossible to do first action (not enough tokens, no place for it, etc.), so this strategy is worthless */
	/*If this strategy requires doing nothing now, the amount of resources I finsh the turn with can decrease this strategy value depending on the risk of being affected by the thief */
	if ((time_a != 0) && (myTurn)) {	/*I am waiting to do something, will I finish my turn with a "risky" amount of resources? */
		endOfTurnResources = totalResources(&myGameState);
		roadsWillBuild =
		    checkRoadNow(oneStrategy[0], oneStrategy[1],
				 myGameState);
		endOfTurnResources -= roadsWillBuild * 2;	/*If I am gonna build some road in this turn subtract those resources */
		ARSperTurn =
		    totalAverageResourceSupplyPerTurn(&myGameState);
		tooManyResDepreciation =
		    depreciateStrategyValue(endOfTurnResources,
					    num_players, ARSperTurn);

#if 0
		printf
		    ("My estimation is that I WILL BUILD %d ROADS at this turn as part of the following strategy:\n",
		     roadsWillBuild);
		if (tooManyResDepreciation < 1) {
			printf("Strategy waiting to do ");
			printAction(oneStrategy[0]);
			printf(" at turn %.2f and then ", time_a);
			printAction(oneStrategy[1]);
			printf
			    (" ends turn with too many resources (%d and ARSperTurn of %.2f), so suffers a depreciation of %.3f\n",
			     endOfTurnResources, ARSperTurn,
			     tooManyResDepreciation);
		} else {
			printf("Strategy waiting to do ");
			printAction(oneStrategy[0]);
			printf(" at turn %.2f and then ", time_a);
			printAction(oneStrategy[1]);
			printf
			    (" ends turn unaffected (%d and ARSperTurn of %.2f), so suffers no depreciation\n",
			     endOfTurnResources, ARSperTurn);
		}
#endif
	}

	if (turn < time_a) {	/*so time_a is not 0 */
		if (time_a == time_b) {
			profit = (((firstActionValue +
				    secondActionValue) / time_a) * turn);
		} else {
			m1 = (firstActionValue / time_a);
			profit = (m1 * turn);
		}
	} else if (turn < time_b) {	/*So time_b is not 0, although time_a could be */
		m2 = (secondActionValue / (time_b - time_a));
		profit = (firstActionValue + m2 * (turn - time_a));
	} else if (turn >= time_b) {
		profit = (firstActionValue + secondActionValue);
	} else
		profit = (0);	/*It should never get here */
	return (profit * tooManyResDepreciation);

	/*Notice how extreme cases are handled:
	   If a=b=0 it means I can do both actions at the same time right now and it will go through the third branch no matter the turn.
	   If a=b but not 0 it means I can do both actions at the same time in the future, and it will go through the first branch if turn is before that moment
	   and through the third from that moment on.
	   If a=0 but not b, it means I can do my first action right now, it will go through the second branch until b is reached. */
}

float bestStrategy(float turn, float probability,
		   struct simulationsData_t *Data, strategy_t myStrategy,
		   struct gameState_t myGameState, int showSimulation,
		   int myTurn, int num_players)
{
	/*Updates myStrategy[] with the pair of preferred actions to be performed in the present/future as their profit is considered when we look up to turn in the future
	 * If myTurn is 1 it means that I'm doing that calculations during my turn
	 * It returns that maximum profit*/
	float time_a, time_b, time_best_firstAction,
	    time_best_secondAction;
	enum action firstAction, secondAction;
	float profit, max_profit;
	strategy_t oneStrategy;

	numberOfTurnsForProbability(probability, Data, myGameState, showSimulation);	/*Sets Data->turnsToAction by simulating */
	time_best_firstAction = MAX_TURNS;
	time_best_secondAction = MAX_TURNS;
	max_profit = 0;
	/*printf("\n\nBeginning the bestStrategy loop...\n"); */
	for (firstAction = SET; firstAction <= 4; firstAction++) {	/*First five actions of turnsToAction are individual actions SET, CIT, DEV, RSET and RRSET */
		time_a = Data->turnsToAction[firstAction];
		for (secondAction = SET; secondAction <= 4; secondAction++) {
			time_b = Data->timeCombinedAction[firstAction]
			    [secondAction];
			oneStrategy[0] = firstAction;
			oneStrategy[1] = secondAction;
			profit =
			    strategyProfit(time_a, time_b, turn,
					   oneStrategy, myGameState,
					   myTurn, num_players);
			if ((profit > max_profit)
			    || ((profit == max_profit)
				&& (time_a < time_best_firstAction))
			    || ((profit == max_profit)
				&& (time_a == time_best_firstAction)
				&& (time_b < time_best_secondAction))) {
				/*Updates the best strategy if:
				 * 1 - The profit calculated iś better than the profit calculated so far OR
				 * 2 - Being equal, this strategy lets me do the firstAction before OR
				 * 3 - Being equal the profit and the time needed to perform firstAction, this strategy lets me do secondAction before*/
				max_profit = profit;
				myStrategy[0] = firstAction;
				time_best_firstAction = time_a;
				myStrategy[1] = secondAction;
				time_best_secondAction = time_b;
			}
		}
	}
	/*printf("Finishing the bestStrategy loop...\n\n"); */
	return (max_profit);
}



int checkRoadNow(enum action firstAction, enum action secondAction,
		 struct gameState_t myGameState)
{
	/*Returns TRUE if  I should build road now, that is, if I have the resources needed, and it does not affect negatively my planned strategy, what is basically:
	 * firstAction involves building a Road (RSET or RRSET) OR
	 *firstAction is City or Development Card (that uses completely different resources than road)  and secondAction is not SET (that will need them).
	 *It does not take into account if there is actually enough roads (tokens)to do it */

	int roadsPossible = 0;
	if ((myGameState.resourcesAlreadyHave[0] >= 1)
	    && (myGameState.resourcesAlreadyHave[4] >= 1))
		roadsPossible = 1;
	if ((myGameState.resourcesAlreadyHave[0] >= 2)
	    && (myGameState.resourcesAlreadyHave[4] >= 2))
		roadsPossible = 2;
	if (!roadsPossible)
		return (0);

	switch (firstAction) {
	case SET:
		/*If I have enough resources for two roads and secondAction is not another SET, I can build one road without affecting my strategy */
		if ((roadsPossible == 2) && (secondAction != SET))
			return (1);
		else
			return (0);
		break;
	case CIT:
	case DEV:
		switch (secondAction) {
		case SET:
			/*If I have enough resources for two roads, I still can build one and have enough resources for the SET */
			if (roadsPossible == 2)
				return (1);
			else
				return (0);
			break;
		case RSET:
			/*Even if I can do two roads now, I should not use the resources I will need for the settlement of RSET */
			return (1);
			break;
		case CIT:
		case DEV:
		case RRSET:
			return (roadsPossible);
			break;
		}
	case RSET:
		/*Even if I can do two roads now, I should not use the resources I will need for the settlement of RSET */
		return (1);
		break;
	case RRSET:
		return (roadsPossible);
		break;
	default:
		return (0);
	}
}
