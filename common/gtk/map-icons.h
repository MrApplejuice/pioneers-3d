/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2005, 2009 Roland Clobus <rclobus@bigfoot.com>
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

#ifndef __map_icons_h
#define __map_icons_h

#include <gtk/gtk.h>

#include "map.h"

/** Creates a widget with a view of a terrain.
 * @param Terrain The terrain in the view.
 * @return The GtkWidget containing the view of the terrain.
 */
GtkWidget *terrain_icon_new(Terrain terrain);

#endif
