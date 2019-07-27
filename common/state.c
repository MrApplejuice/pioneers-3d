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

#include "config.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include "game.h"
#include "log.h"
#include "state.h"
#include "network.h"

struct StateMachine {
	gpointer user_data;	/* parameter for mode functions */
	/* FIXME RC 2004-11-13 in practice:
	 * it is NULL or a Player*
	 * the value is set by sm_new.
	 * Why? Can the player not be bound to a
	 * StateMachine otherwise? */

	StateFunc global;	/* global state - test after current state */
	StateFunc unhandled;	/* global state - process unhandled states */
	StateFunc stack[16];	/* handle sm_push() to save context */
	const gchar *stack_name[16];	/* state names used for a stack dump */
	gint stack_ptr;		/* stack index */
	const gchar *current_state;	/* name of current state */

	gchar *line;		/* line passed in from network event */
	size_t line_offset;	/* line prefix handling */

	Session *ses;		/* network session feeding state machine */
	gint use_count;		/* # functions is in use by */
	gboolean is_dead;	/* is this machine waiting to be killed? */

	gboolean use_cache;	/* cache the data that is sent */
	GList *cache;		/* cache for the delayed data */
};

static void route_event(StateMachine * sm, gint event);

void sm_inc_use_count(StateMachine * sm)
{
	sm->use_count++;
}

void sm_dec_use_count(StateMachine * sm)
{
	if (!--sm->use_count && sm->is_dead)
		sm_free(sm);
}

const gchar *sm_current_name(StateMachine * sm)
{
	return sm->current_state;
}

void sm_state_name(StateMachine * sm, const gchar * name)
{
	sm->current_state = name;
	sm->stack_name[sm->stack_ptr] = name;
}

gboolean sm_is_connected(StateMachine * sm)
{
	return sm->ses != NULL && net_connected(sm->ses);
}

static void route_event(StateMachine * sm, gint event)
{
	StateFunc curr_state;
	gpointer user_data;

	if (sm->stack_ptr >= 0)
		curr_state = sm_current(sm);
	else
		curr_state = NULL;

	user_data = sm->user_data;
	if (user_data == NULL)
		user_data = sm;

	/* send death notification even when dead */
	if (event == SM_FREE) {
		/* send death notifications only to global handler */
		if (sm->global !=NULL)
			sm->global (user_data, event);
		return;
	}

	if (sm->is_dead)
		return;

	switch (event) {
	case SM_ENTER:
		if (curr_state != NULL)
			curr_state(user_data, event);
		break;
	case SM_INIT:
		if (curr_state != NULL)
			curr_state(user_data, event);
		if (!sm->is_dead && sm->global !=NULL)
			sm->global (user_data, event);
		break;
	case SM_RECV:
		sm_cancel_prefix(sm);
		if (curr_state != NULL && curr_state(user_data, event))
			break;
		sm_cancel_prefix(sm);
		if (!sm->is_dead
		    && sm->global !=NULL && sm->global (user_data, event))
			break;

		sm_cancel_prefix(sm);
		if (!sm->is_dead && sm->unhandled != NULL)
			sm->unhandled(user_data, event);
		break;
	case SM_NET_CLOSE:
		sm_close(sm);
		/* fall through */
	default:
		if (curr_state != NULL)
			curr_state(user_data, event);
		if (!sm->is_dead && sm->global !=NULL)
			sm->global (user_data, event);
		break;
	}
}

void sm_cancel_prefix(StateMachine * sm)
{
	sm->line_offset = 0;
}

static void net_event(Session * ses, NetEvent event, const gchar * line,
		      gpointer user_data)
{
	StateMachine *sm = (StateMachine *) user_data;

	g_assert(ses == sm->ses);

	sm_inc_use_count(sm);

	switch (event) {
	case NET_CONNECT:
		route_event(sm, SM_NET_CONNECT);
		break;
	case NET_CONNECT_FAIL:
		route_event(sm, SM_NET_CONNECT_FAIL);
		break;
	case NET_CLOSE:
		route_event(sm, SM_NET_CLOSE);
		break;
	case NET_READ:
		if (sm->line)
			g_free(sm->line);
		sm->line = g_strdup(line);
		/* Only handle data if there is a context.  Fixes bug that
		 * clients starting to send data immediately crash the
		 * server */
		if (sm->stack_ptr != -1)
			route_event(sm, SM_RECV);
		else {
			sm_dec_use_count(sm);
			return;
		}
		break;
	}
	route_event(sm, SM_INIT);

	sm_dec_use_count(sm);
}

gboolean sm_connect(StateMachine * sm, const gchar * host,
		    const gchar * port)
{
	if (sm->ses != NULL)
		net_free(&(sm->ses));

	sm->ses = net_new(net_event, sm);
	log_message(MSG_INFO, _("Connecting to %s, port %s\n"), host,
		    port);
	if (net_connect(sm->ses, host, port))
		return TRUE;

	net_free(&(sm->ses));
	return FALSE;
}

void sm_set_session(StateMachine * sm, Session * ses)
{
	sm_inc_use_count(sm);
	if (sm->ses != NULL)
		net_free(&(sm->ses));
	sm->ses = ses;
	net_set_notify_func(sm->ses, net_event, sm);
	sm_dec_use_count(sm);
};

gboolean sm_recv(StateMachine * sm, const gchar * fmt, ...)
{
	va_list ap;
	ssize_t offset;

	va_start(ap, fmt);
	offset = game_vscanf(sm->line + sm->line_offset, fmt, ap);
	va_end(ap);

	return offset > 0
	    && sm->line[sm->line_offset + (size_t) offset] == '\0';
}

gboolean sm_recv_prefix(StateMachine * sm, const gchar * fmt, ...)
{
	va_list ap;
	ssize_t offset;

	va_start(ap, fmt);
	offset = game_vscanf(sm->line + sm->line_offset, fmt, ap);
	va_end(ap);

	if (offset < 0)
		return FALSE;
	sm->line_offset += (size_t) offset;
	return TRUE;
}

void sm_write(StateMachine * sm, const gchar * str)
{
	if (sm->use_cache) {
		/* Protect against strange/slow connects */
		if (g_list_length(sm->cache) > 1000) {
			net_write(sm->ses, "ERR connection too slow\n");
			net_close(sm->ses);
		} else {
			sm->cache =
			    g_list_append(sm->cache, g_strdup(str));
		}
	} else
		net_write(sm->ses, str);
}

void sm_write_uncached(StateMachine * sm, const gchar * str)
{
	g_assert(sm->ses);
	g_assert(sm->use_cache);

	net_write(sm->ses, str);
}

void sm_send(StateMachine * sm, const gchar * fmt, ...)
{
	va_list ap;
	gchar *buff;

	if (!sm->ses)
		return;

	va_start(ap, fmt);
	buff = game_vprintf(fmt, ap);
	va_end(ap);

	sm_write(sm, buff);
	g_free(buff);
}

void sm_set_use_cache(StateMachine * sm, gboolean use_cache)
{
	if (sm->use_cache == use_cache)
		return;

	if (!use_cache) {
		/* The cache is turned off, send the delayed data */
		GList *list = sm->cache;
		while (list) {
			gchar *data = list->data;
			net_write(sm->ses, data);
			list = g_list_remove(list, data);
			g_free(data);
		}
		sm->cache = NULL;
	} else {
		/* Be sure that the cache is empty */
		g_assert(!sm->cache);
	}
	sm->use_cache = use_cache;
}

gboolean sm_get_use_cache(const StateMachine * sm)
{
	return sm->use_cache;
}

void sm_global_set(StateMachine * sm, StateFunc state)
{
	sm->global = state;
}

void sm_unhandled_set(StateMachine * sm, StateFunc state)
{
	sm->unhandled = state;
}

static void push_new_state(StateMachine * sm)
{
	++sm->stack_ptr;
	/* check for stack overflows */
	if (sm->stack_ptr >= (gint) G_N_ELEMENTS(sm->stack)) {
		log_message(MSG_ERROR,
			    /* Error message */
			    _(""
			      "State stack overflow. Stack dump sent to standard error.\n"));
		sm_stack_dump(sm);
		g_error("State stack overflow");
	}
	sm->stack[sm->stack_ptr] = NULL;
	sm->stack_name[sm->stack_ptr] = NULL;
}

static void do_goto(StateMachine * sm, StateFunc new_state, gboolean enter)
{
	sm_inc_use_count(sm);

	if (sm->stack_ptr < 0) {
		/* Wait until the application window is fully
		 * displayed before starting state machine.
		 */
		if (driver != NULL && driver->event_queue != NULL)
			driver->event_queue();
		push_new_state(sm);
	}

	sm->stack[sm->stack_ptr] = new_state;
	if (enter)
		route_event(sm, SM_ENTER);
	route_event(sm, SM_INIT);

#ifdef STACK_DEBUG
	debug("sm_goto  -> %d:%s", sm->stack_ptr, sm->current_state);
#endif

	sm_dec_use_count(sm);
}

void sm_debug(G_GNUC_UNUSED const gchar * function,
	      G_GNUC_UNUSED const gchar * state)
{
#ifdef STACK_DEBUG
	debug("Call %s with %s\n", function, state);
#endif
}

void sm_goto_nomacro(StateMachine * sm, StateFunc new_state)
{
	do_goto(sm, new_state, TRUE);
}

void sm_goto_noenter_nomacro(StateMachine * sm, StateFunc new_state)
{
	do_goto(sm, new_state, FALSE);
}

static void do_push(StateMachine * sm, StateFunc new_state, gboolean enter)
{
	sm_inc_use_count(sm);

	push_new_state(sm);
	sm->stack[sm->stack_ptr] = new_state;
	if (enter)
		route_event(sm, SM_ENTER);
	route_event(sm, SM_INIT);
#ifdef STACK_DEBUG
	debug("sm_push -> %d:%s (enter=%d)", sm->stack_ptr,
	      sm->current_state, enter);
#endif
	sm_dec_use_count(sm);
}

void sm_push_nomacro(StateMachine * sm, StateFunc new_state)
{
	do_push(sm, new_state, TRUE);
}

void sm_push_noenter_nomacro(StateMachine * sm, StateFunc new_state)
{
	do_push(sm, new_state, FALSE);
}

void sm_pop(StateMachine * sm)
{
	sm_inc_use_count(sm);

	g_assert(sm->stack_ptr > 0);
	sm->stack_ptr--;
	route_event(sm, SM_ENTER);
#ifdef STACK_DEBUG
	debug("sm_pop  -> %d:%s", sm->stack_ptr, sm->current_state);
#endif
	route_event(sm, SM_INIT);
	sm_dec_use_count(sm);
}

void sm_multipop(StateMachine * sm, gint depth)
{
	sm_inc_use_count(sm);

	g_assert(sm->stack_ptr >= depth - 1);
	sm->stack_ptr -= depth;
	route_event(sm, SM_ENTER);
#ifdef STACK_DEBUG
	debug("sm_multipop  -> %d:%s", sm->stack_ptr, sm->current_state);
#endif
	route_event(sm, SM_INIT);

	sm_dec_use_count(sm);
}

void sm_pop_all_and_goto(StateMachine * sm, StateFunc new_state)
{
	sm_inc_use_count(sm);

	sm->stack_ptr = 0;
	sm->stack[sm->stack_ptr] = new_state;
	sm->stack_name[sm->stack_ptr] = NULL;
	route_event(sm, SM_ENTER);
	route_event(sm, SM_INIT);

	sm_dec_use_count(sm);
}

/** Return the state at offset from the top of the stack.
 *  @param sm     The StateMachine
 *  @param offset Offset from the top (0=top, 1=previous)
 *  @return The StateFunc, or NULL if the stack contains
 *          less than offset entries
 */
StateFunc sm_stack_inspect(const StateMachine * sm, guint offset)
{
	if (sm->stack_ptr >= (gint) offset)
		return sm->stack[(guint) sm->stack_ptr - offset];
	else
		return NULL;
}

StateFunc sm_current(StateMachine * sm)
{
	g_assert(sm->stack_ptr >= 0);

	return sm->stack[sm->stack_ptr];
}

/* Build a new state machine instance
 */
StateMachine *sm_new(gpointer user_data)
{
	StateMachine *sm = g_malloc0(sizeof(*sm));

	sm->user_data = user_data;
	sm->stack_ptr = -1;

	return sm;
}

/* Free a state machine
 */
void sm_free(StateMachine * sm)
{
	g_free(sm->line);
	sm->line = NULL;
	if (sm->ses != NULL) {
		net_free(&(sm->ses));
		return;
	}
	if (sm->use_count > 0)
		sm->is_dead = TRUE;
	else {
		route_event(sm, SM_FREE);
		g_free(sm);
	}
}

void sm_close(StateMachine * sm)
{
	net_free(&(sm->ses));
	if (sm->use_cache) {
		/* Purge the cache */
		GList *list = sm->cache;
		sm->cache = NULL;
		sm_set_use_cache(sm, FALSE);
		while (list) {
			gchar *data = list->data;
			list = g_list_remove(list, data);
			g_free(data);
		}
	}
}

void sm_copy_stack(StateMachine * dest, const StateMachine * src)
{
	memcpy(dest->stack, src->stack, sizeof(dest->stack));
	memcpy(dest->stack_name, src->stack_name,
	       sizeof(dest->stack_name));
	dest->stack_ptr = src->stack_ptr;
	dest->current_state = src->current_state;
}

void sm_stack_dump(const StateMachine * sm)
{
	gint sp;
	fprintf(stderr, "Stack dump for %p\n", (const void *) sm);
	for (sp = 0; sp <= sm->stack_ptr; ++sp) {
		fprintf(stderr, "Stack %2d: %s\n", sp, sm->stack_name[sp]);
	}
}
