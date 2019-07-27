/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2013 Micah Bunting <Amnykon@gmail.com>
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

/** @file random.c
 * Contains the functions to randomly generate numbers.
 *
 * The random number generator need to be initialized with random_init() before
 * any other functions are called.
 *
 * To obtain a random number, use random_guint().
 */

#include "random.h"

/** The seed for the random number generator. */
GRand *g_rand_ctx = NULL;

/** Initializes the seed to the random number generator.
 * @return The seed to the random number generator.
 */
guint32 random_init(void)
{
	guint32 randomseed;
	g_rand_ctx = g_rand_new();
	randomseed = g_rand_int(g_rand_ctx);
	g_rand_set_seed(g_rand_ctx, randomseed);
	return randomseed;
}

/**
 * Returns a random number from 0 to range - 1.
 * @param range The range of the random number generator.
 * @return The random number.
 */
guint random_guint(guint range)
{
	return g_rand_int_range(g_rand_ctx, 0, range);
}
