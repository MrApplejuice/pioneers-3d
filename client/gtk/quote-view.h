/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2006 Roland Clobus <rclobus@bigfoot.com>
 * Copyright (C) 2006 Bas Wijnen <shevek@fmf.nl>
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

#ifndef __quoteview_h
#define __quoteview_h

#include <gtk/gtk.h>
#include "map.h"		/* For NO_RESOURCE */
#include "quoteinfo.h"

G_BEGIN_DECLS
#define QUOTEVIEW_TYPE            (quote_view_get_type ())
#define QUOTEVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QUOTEVIEW_TYPE, QuoteView))
#define QUOTEVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), QUOTEVIEW_TYPE, QuoteViewClass))
#define IS_QUOTEVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QUOTEVIEW_TYPE))
#define IS_QUOTEVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), QUOTEVIEW_TYPE))
typedef struct _QuoteView QuoteView;
typedef struct _QuoteViewClass QuoteViewClass;

typedef gboolean(*CheckQuoteFunc) (const QuoteInfo * quote);

struct _QuoteView {
	GtkScrolledWindow scrolled_window;

	/** The data */
	GtkListStore *store;

	/** The tree view widget */
	GtkWidget *quotes;

	/** All quotes */
	QuoteList *quote_list;

	/** Show maritime quotes? */
	gboolean with_maritime;

	/** Information about available maritime trades */
	MaritimeInfo maritime_info;

	gboolean maritime_filter_supply[NO_RESOURCE];
	gboolean maritime_filter_receive[NO_RESOURCE];

	CheckQuoteFunc check_quote_func;

	/** The currently selected quote, or NULL */
	const QuoteInfo *selected_quote;

	/** CheckQuoteFunc returns true */
	GdkPixbuf *true_pixbuf;

	/** CheckQuoteFunc returns false */
	GdkPixbuf *false_pixbuf;
};

struct _QuoteViewClass {
	GtkScrolledWindowClass parent_class;

	void (*selection_changed) (QuoteView * qv);
	void (*selection_activated) (QuoteView * qv);
};

GType quote_view_get_type(void);
GtkWidget *quote_view_new(gboolean with_maritime,
			  CheckQuoteFunc check_quote_func,
			  const gchar * true_pixbuf_id,
			  const gchar * false_pixbuf_id);
void quote_view_begin(QuoteView * qv);
void quote_view_add_quote(QuoteView * qv, gint player_num,
			  gint quote_num, const gint * supply,
			  const gint * receive);
void quote_view_remove_quote(QuoteView * qv, gint partner_num,
			     gint quote_num);
void quote_view_reject(QuoteView * qv, gint player_num);
void quote_view_finish(QuoteView * qv);
void quote_view_check_validity_of_trades(QuoteView * qv);
void quote_view_clear_selected_quote(QuoteView * qv);
const QuoteInfo *quote_view_get_selected_quote(QuoteView * qv);
void quote_view_remove_rejected_quotes(QuoteView * qv);
void quote_view_set_maritime_filters(QuoteView * qv,
				     const gboolean * filter_supply,
				     const gboolean * filter_receive);
void quote_view_theme_changed(QuoteView * qv);
gboolean quote_view_trade_exists(QuoteView * qv, const gint * supply,
				 const gint * receive);
gboolean quote_view_has_reject(QuoteView * qv, gint player_num);
G_END_DECLS
#endif
