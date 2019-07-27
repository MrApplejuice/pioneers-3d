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

#ifndef __quoteinfo_h
#define __quoteinfo_h

typedef struct {
	GList *list;		/* list entry which owns the quote */
	gboolean is_domestic;	/* is this a maritime trade? */
	union {
		struct {
			gint player_num;	/* player who make the quote */
			gint quote_num;	/* quote identifier */
			gint supply[NO_RESOURCE];	/* resources supplied in the quote */
			gint receive[NO_RESOURCE];	/* resources received in the quote */
		} d;
		struct {
			gint ratio;
			Resource supply;
			Resource receive;
		} m;
	} var;
} QuoteInfo;

typedef struct {
	GList *quotes;
} QuoteList;

/** Create a new quote list, and remove the old list if needed */
void quotelist_new(QuoteList ** list);
/** Free the QuoteList (if needed), and set it to NULL */
void quotelist_free(QuoteList ** list);
QuoteInfo *quotelist_add_domestic(QuoteList * list, gint player_num,
				  gint quote_num, const gint * supply,
				  const gint * receive);
QuoteInfo *quotelist_add_maritime(QuoteList * list, gint ratio,
				  Resource supply, Resource receive);
QuoteInfo *quotelist_first(QuoteList * list);
QuoteInfo *quotelist_prev(const QuoteInfo * quote);
QuoteInfo *quotelist_next(const QuoteInfo * quote);
gboolean quotelist_is_player_first(const QuoteInfo * quote);
QuoteInfo *quotelist_find_domestic(QuoteList * list, gint player_num,
				   gint quote_num);
void quotelist_delete(QuoteList * list, QuoteInfo * quote);

#endif
