/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
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

#ifndef __state_h
#define __state_h

#include <glib.h>
#include "network.h"

/* sm_ API:
 *
 * The server output is handled one line at a time.  For each line
 * received, the current state is called with the SM_RECV event.  The
 * [fmt] format string is modelled on the printf format string,
 * see game_printf and game_scanf for details.
 *
 * sm_recv(fmt, ...)
 *	Match the entire current line from the start position.
 *	Returns TRUE if there is a match
 *	
 * sm_recv_prefix(fmt, ...)
 *	Match a prefix of the current line from the start position.
 *	Returns TRUE if there is a match, and sets the start position
 *	to the character following the prefix.  If the prefix does not
 *	match, the function returns FALSE, and does not alter the
 *	start position.
 *
 * sm_cancel_prefix()
 *	Set start position in current line back to beginning.
 *
 * sm_send(fmt, ...)
 *	Send data back to the server.
 *
 * The sm_ API maintains a record of the current state, and a stack of
 * previous states.  Code can move to new states using the following
 * functions.
 *
 * sm_goto(new_state)
 *	Set the current state to [new_state]
 *
 * sm_push(new_state)
 *	Save the current state on the stack, then set the current
 *	state to [new_state]
 *
 * sm_pop()
 *	Pop the top state off the stack and make it the current state.
 *
 * sm_multipop()
 * 	Pop a number of states off the stack and set a new current state
 * 	accordingly.
 *
 * sm_pop_all_and_goto(new_state)
 *	Clear the state stack, set the current state to [new_state]
 *
 * sm_stack_inspect()
 *	Return the state at offset from the top of the stack.
 */

typedef enum {
	SM_NET_CONNECT = 10000,
	SM_NET_CONNECT_FAIL,
	SM_NET_CLOSE,

	SM_ENTER,
	SM_INIT,
	SM_RECV,
	SM_FREE
} EventType;

typedef struct StateMachine StateMachine;

/* All state functions look like this
 */
typedef gboolean(*StateFunc) (StateMachine * sm, gint event);

StateMachine *sm_new(gpointer user_data);
void sm_free(StateMachine * sm);
void sm_close(StateMachine * sm);
/** Copy the stack to another state machine.
 * @dest Destination
 * @src Source
*/
void sm_copy_stack(StateMachine * dest, const StateMachine * src);

const gchar *sm_current_name(StateMachine * sm);
void sm_state_name(StateMachine * sm, const gchar * name);
gboolean sm_recv(StateMachine * sm, const gchar * fmt, ...);
gboolean sm_recv_prefix(StateMachine * sm, const gchar * fmt, ...);
void sm_cancel_prefix(StateMachine * sm);
void sm_write(StateMachine * sm, const gchar * str);
/** Send the data, even when caching is turned on */
void sm_write_uncached(StateMachine * sm, const gchar * str);
void sm_send(StateMachine * sm, const gchar * fmt, ...);
/** Cache the messages that are sent.
 * When the caching is turned off, all cached data is sent.
 * @param sm The statemachine
 * @param use_cache Turn the caching on/off
 */
void sm_set_use_cache(StateMachine * sm, gboolean use_cache);
/** Check whether messages are cached.
 * @param sm The statemachine
 * @return TRUE when the caching of messages is active
 */
gboolean sm_get_use_cache(const StateMachine * sm);

void sm_debug(const gchar * function, const gchar * state);
#define sm_goto(a, b) do { sm_debug("sm_goto", #b); sm_goto_nomacro(a, b); } while (0)
void sm_goto_nomacro(StateMachine * sm, StateFunc new_state);
#define sm_goto_noenter(a, b) do { sm_debug("sm_goto_noenter", #b); sm_goto_noenter_nomacro(a, b); } while (0)
void sm_goto_noenter_nomacro(StateMachine * sm, StateFunc new_state);
#define sm_push(a, b) do { sm_debug("sm_push", #b); sm_push_nomacro(a, b); } while (0)
void sm_push_nomacro(StateMachine * sm, StateFunc new_state);
#define sm_push_noenter(a, b) do { sm_debug("sm_push_noenter", #b); sm_push_noenter_nomacro(a, b); } while (0)
void sm_push_noenter_nomacro(StateMachine * sm, StateFunc new_state);
void sm_pop(StateMachine * sm);
void sm_multipop(StateMachine * sm, gint depth);
void sm_pop_all_and_goto(StateMachine * sm, StateFunc new_state);
StateFunc sm_current(StateMachine * sm);
StateFunc sm_stack_inspect(const StateMachine * sm, guint offset);
void sm_global_set(StateMachine * sm, StateFunc state);
void sm_unhandled_set(StateMachine * sm, StateFunc state);

gboolean sm_is_connected(StateMachine * sm);
gboolean sm_connect(StateMachine * sm, const gchar * host,
		    const gchar * port);
void sm_set_session(StateMachine * sm, Session * ses);
void sm_dec_use_count(StateMachine * sm);
void sm_inc_use_count(StateMachine * sm);
/** Dump the stack */
void sm_stack_dump(const StateMachine * sm);
#endif
