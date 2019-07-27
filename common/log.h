/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2006 Roland Clobus <rclobus@bigfoot.com>
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

#ifndef __log_h
#define __log_h

#include <glib.h>
#include <glib/gi18n.h>

/** Type of logging functions */
typedef void (*LogFunc) (gint msg_type, const gchar * text);

/* Message Types */
#define MSG_ERROR	1
#define MSG_INFO	2
#define MSG_CHAT	3
#define MSG_RESOURCE	4
#define MSG_BUILD	5
#define MSG_DICE	6
#define MSG_STEAL	7
#define MSG_TRADE	8
#define MSG_DEVCARD	9
#define MSG_LARGESTARMY	10
#define MSG_LONGESTROAD	11
#define MSG_BEEP	12
#define MSG_TIMESTAMP	13
#define MSG_PLAYER1	101
#define MSG_PLAYER2	102
#define MSG_PLAYER3	103
#define MSG_PLAYER4	104
#define MSG_PLAYER5	105
#define MSG_PLAYER6	106
#define MSG_PLAYER7	107
#define MSG_PLAYER8	108
#define MSG_SPECTATOR_CHAT	199

/** Set the logging function to 'func'. */
void log_set_func(LogFunc func);

/** Set the logging function to the system default (stderr) */
void log_set_func_default(void);

/** Write a message string to the console, adding a prefix depending on
 *   its type.
 */
void log_message_string_console(gint msg_type, const gchar * text);

/** Log a message after turning the params into a single string. */
void log_message(gint msg_type, const gchar * fmt, ...);

/** Log a chat message.
 *  When the log function is not the default, only the chat is shown
 *  with msg_type, the other parts are shown with MSG_INFO.
 *  This means that player_name and joining_text are shown black,
 *  and the chat is in the colour of the player.
 */
void log_message_chat(const gchar * player_name,
		      const gchar * joining_text, gint msg_type,
		      const gchar * chat);

void set_enable_debug(gboolean enabled);
void debug(const gchar * fmt, ...);

#endif				/* __log_h */
