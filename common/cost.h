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

#ifndef __cost_h
#define __cost_h

#include <glib.h>

const gint *cost_road(void);
const gint *cost_ship(void);
const gint *cost_bridge(void);
const gint *cost_settlement(void);
const gint *cost_upgrade_settlement(void);
const gint *cost_city(void);
const gint *cost_city_wall(void);
const gint *cost_development(void);

gboolean cost_buy(const gint * cost, gint * assets);
void cost_refund(const gint * cost, gint * assets);
gboolean cost_can_afford(const gint * cost, const gint * assets);

#endif
