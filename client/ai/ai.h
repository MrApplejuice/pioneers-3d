/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
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

#ifndef _ai_h
#define _ai_h

#include <glib.h>
#include "callback.h"

/** Filename for the chromosome of the genetic player */
extern char *chromosomeFile;

void ai_panic(const char *message);
void ai_wait(void);
void ai_chat(const char *message);
void genetic_init(void);
void greedy_init(void);
void lobbybot_init(void);

/** Chat when a player must discard resources */
void ai_chat_discard(gint player_num, gint discard_num);
/** Chat when the computer player moved the robber */
void ai_chat_self_moved_robber(void);
#endif
