/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2000 Steve Langasek
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

#ifndef __driver_h
#define __driver_h

#include "log.h"

typedef struct {
	/* function for clearing the event queue */
	void (*event_queue) (void);

	/* Function to write logs and data to the system display */
	LogFunc log_write;	/* ==> void log_write( gint msg_type, gchar *text ); */

	/* callbacks for the server */
	void (*player_added) (void *player);	/* these really should be ... */
	void (*player_renamed) (void *player);	/* ... `Player *player', but */
	void (*player_removed) (void *player);	/* that requires more headers */

	void (*player_change) (void *game);	/* Should be Game *game */

} UIDriver;

extern UIDriver *driver;

void set_ui_driver(UIDriver * d);

#endif				/* __driver_h */
