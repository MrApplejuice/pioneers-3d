/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2004-2006 Roland Clobus <rclobus@bigfoot.com>
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

#include "config.h"
#include "quote-view.h"
#include "map.h"		/* For NO_RESOURCE */
#include "game.h"
#include "quoteinfo.h"
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "colors.h"
#include "frontend.h"
#include "theme.h"
#include "common_gtk.h"

enum {
	TRADE_COLUMN_PLAYER, /**< Player icon */
	TRADE_COLUMN_POSSIBLE, /**< Good/bad trade icon */
	TRADE_COLUMN_DESCRIPTION, /**< Trade description */
	TRADE_COLUMN_QUOTE, /**< Internal data: contains the quotes. Not for display */
	TRADE_COLUMN_REJECT, /**< Internal data: contains the rejected players. Not for display */
	TRADE_COLUMN_PLAYER_NUM, /**< The player number, or -1 for maritime trade */
	TRADE_COLUMN_LAST
};

/** The quote is found here */
static GtkTreeIter quote_found_iter;
/** Has the quote been found ? */
static gboolean quote_found_flag;

/** Icon for rejected trade */
static GdkPixbuf *cross_pixbuf;
/** Icon for the maritime trade */
static GdkPixbuf *maritime_pixbuf;

/* The signals */
enum {
	SELECTION_CHANGED,
	SELECTION_ACTIVATED,
	LAST_SIGNAL
};

static void quote_view_class_init(QuoteViewClass * klass);
static void quote_view_init(QuoteView * qv);

static gint quote_click_cb(GtkWidget * widget,
			   GdkEventButton * event, gpointer user_data);
static void quote_select_cb(GtkTreeSelection * selection,
			    gpointer user_data);
static void load_pixmaps(QuoteView * qv);
static void set_selected_quote(QuoteView * qv, const QuoteInfo * quote);
static gboolean trade_locate_quote(GtkTreeModel * model,
				   G_GNUC_UNUSED GtkTreePath * path,
				   GtkTreeIter * iter, gpointer user_data);

/* All signals */
static guint quote_view_signals[LAST_SIGNAL] = { 0, 0 };

/* Register the class */
GType quote_view_get_type(void)
{
	static GType rt_type = 0;

	if (!rt_type) {
		static const GTypeInfo rt_info = {
			sizeof(QuoteViewClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) quote_view_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(QuoteView),
			0,
			(GInstanceInitFunc) quote_view_init,
			NULL
		};
		rt_type =
		    g_type_register_static(GTK_TYPE_SCROLLED_WINDOW,
					   "QuoteView", &rt_info, 0);
	}
	return rt_type;
}

/* Register the signals.
 * QuoteView will emit these signals:
 * 'selection-changed'   when the selection changes.
 * 'selection-activated' when the selection is double-clicked
 */
static void quote_view_class_init(QuoteViewClass * klass)
{
	quote_view_signals[SELECTION_CHANGED] =
	    g_signal_new("selection-changed",
			 G_TYPE_FROM_CLASS
			 (klass),
			 G_SIGNAL_RUN_FIRST |
			 G_SIGNAL_ACTION,
			 G_STRUCT_OFFSET
			 (QuoteViewClass,
			  selection_changed), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	quote_view_signals[SELECTION_ACTIVATED] =
	    g_signal_new("selection-activated",
			 G_TYPE_FROM_CLASS
			 (klass),
			 G_SIGNAL_RUN_FIRST |
			 G_SIGNAL_ACTION,
			 G_STRUCT_OFFSET
			 (QuoteViewClass,
			  selection_activated), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

/* Initialise the composite widget */
static void quote_view_init(QuoteView * qv)
{
	GtkTreeViewColumn *column;

	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (qv), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(qv),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(qv), NULL);
	gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(qv), NULL);

	/* Create model */
	qv->store = gtk_list_store_new(TRADE_COLUMN_LAST,
				       GDK_TYPE_PIXBUF,
				       GDK_TYPE_PIXBUF,
				       G_TYPE_STRING,
				       G_TYPE_POINTER,
				       G_TYPE_POINTER, G_TYPE_INT);

	/* Create graphical representation of the model */
	qv->quotes =
	    gtk_tree_view_new_with_model(GTK_TREE_MODEL(qv->store));
	gtk_container_add(GTK_CONTAINER(qv), qv->quotes);

	/* Register double-click */
	g_signal_connect(G_OBJECT(qv->quotes), "button_press_event",
			 G_CALLBACK(quote_click_cb), qv);

	g_signal_connect(G_OBJECT
			 (gtk_tree_view_get_selection
			  (GTK_TREE_VIEW(qv->quotes))), "changed",
			 G_CALLBACK(quote_select_cb), qv);

	/* Now create columns */

	/* Table header: Player who trades */
	column = gtk_tree_view_column_new_with_attributes(_("Player"),
							  gtk_cell_renderer_pixbuf_new
							  (), "pixbuf",
							  TRADE_COLUMN_PLAYER,
							  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(qv->quotes), column);

	column = gtk_tree_view_column_new_with_attributes("",
							  gtk_cell_renderer_pixbuf_new
							  (), "pixbuf",
							  TRADE_COLUMN_POSSIBLE,
							  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(qv->quotes), column);

	/* Table header: Quote */
	column = gtk_tree_view_column_new_with_attributes(_("Quotes"),
							  gtk_cell_renderer_text_new
							  (), "text",
							  TRADE_COLUMN_DESCRIPTION,
							  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(qv->quotes), column);
	gtk_widget_show(qv->quotes);

	load_pixmaps(qv);

	qv->with_maritime = FALSE;
	qv->quote_list = NULL;
}

/* Create a new QuoteView */
GtkWidget *quote_view_new(gboolean with_maritime,
			  CheckQuoteFunc check_quote_func,
			  const gchar * true_pixbuf_id,
			  const gchar * false_pixbuf_id)
{
	QuoteView *qv;
	GtkIconTheme *theme;

	qv = g_object_new(quote_view_get_type(), NULL);
	qv->with_maritime = with_maritime;
	qv->check_quote_func = check_quote_func;

	theme = gtk_icon_theme_get_default();
	if (true_pixbuf_id)
		qv->true_pixbuf =
		    gtk_icon_theme_load_icon(theme, true_pixbuf_id, 16, 0,
					     NULL);
	else
		qv->true_pixbuf = NULL;

	if (false_pixbuf_id)
		qv->false_pixbuf =
		    gtk_icon_theme_load_icon(theme, false_pixbuf_id, 16, 0,
					     NULL);
	else
		qv->false_pixbuf = NULL;

	return GTK_WIDGET(qv);
}

static gint quote_click_cb(G_GNUC_UNUSED GtkWidget * widget,
			   GdkEventButton * event, gpointer quoteview)
{
	if (event->type == GDK_2BUTTON_PRESS) {
		g_signal_emit(G_OBJECT(quoteview),
			      quote_view_signals[SELECTION_ACTIVATED], 0);
	};
	return FALSE;
}

static void quote_select_cb(GtkTreeSelection * selection,
			    gpointer quoteview)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	QuoteInfo *quote;

	g_assert(selection != NULL);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
		gtk_tree_model_get(model, &iter, TRADE_COLUMN_QUOTE,
				   &quote, -1);
	else
		quote = NULL;
	set_selected_quote(QUOTEVIEW(quoteview), quote);
}

/** Load/construct the images */
static void load_pixmaps(QuoteView * qv)
{
	static gboolean init = FALSE;
	int width, height;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_rectangle_t extent;

	if (init)
		return;

	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);
	extent.width = width;
	extent.height = height;

	surface =
	    cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA,
					   &extent);
	cr = cairo_create(surface);
	gdk_cairo_set_source_pixbuf(cr,
				    theme_get_terrain_pixbuf(SEA_TERRAIN),
				    0.0, 0.0);
	cairo_pattern_set_extend(cairo_get_source(cr),
				 CAIRO_EXTEND_REPEAT);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	gdk_cairo_set_source_rgba(cr, &black);
	cairo_set_line_width(cr, 1.0);
	cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
	cairo_stroke(cr);
	maritime_pixbuf =
	    gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	cross_pixbuf =
	    gtk_widget_render_icon_pixbuf(qv->quotes, "pioneers-cross",
					  GTK_ICON_SIZE_MENU);

	init = TRUE;
}

static void trade_format_maritime(const QuoteInfo * quote, gchar * desc)
{
	/* trade: maritime quote: %1 resources of type %2 for
	 * one resource of type %3 */
	sprintf(desc, _("%d:1 %s for %s"),
		quote->var.m.ratio,
		resource_name(quote->var.m.supply, FALSE),
		resource_name(quote->var.m.receive, FALSE));
}

/** Add a maritime trade */
static void add_maritime_trade(QuoteView * qv, G_GNUC_UNUSED gint ratio,
			       G_GNUC_UNUSED Resource receive,
			       G_GNUC_UNUSED Resource supply)
{
	QuoteInfo *quote;
	QuoteInfo *prev;
	gchar quote_desc[128];
	GtkTreeIter iter;

	for (quote = quotelist_first(qv->quote_list);
	     quote != NULL; quote = quotelist_next(quote))
		if (quote->is_domestic)
			break;
		else if (quote->var.m.ratio == ratio
			 && quote->var.m.supply == supply
			 && quote->var.m.receive == receive)
			return;

	quote =
	    quotelist_add_maritime(qv->quote_list, ratio, supply, receive);

	trade_format_maritime(quote, quote_desc);
	prev = quotelist_prev(quote);

	quote_found_flag = FALSE;
	if (prev != NULL)
		gtk_tree_model_foreach(GTK_TREE_MODEL(qv->store),
				       trade_locate_quote, prev);
	if (quote_found_flag)
		gtk_list_store_insert_after(qv->store, &iter,
					    &quote_found_iter);
	else
		gtk_list_store_prepend(qv->store, &iter);
	gtk_list_store_set(qv->store, &iter, TRADE_COLUMN_PLAYER, maritime_pixbuf, TRADE_COLUMN_POSSIBLE, NULL, TRADE_COLUMN_DESCRIPTION, quote_desc, TRADE_COLUMN_QUOTE, quote, TRADE_COLUMN_PLAYER_NUM, -1,	/*
																										   Maritime trade */
			   -1);
}

/** Locate the QuoteInfo* in user_data. Return TRUE if is found. The iter
 *  is set in quote_found_iter. The flag quote_found_flag is set to TRUE
 */
static gboolean trade_locate_quote(GtkTreeModel * model,
				   G_GNUC_UNUSED GtkTreePath * path,
				   GtkTreeIter * iter, gpointer user_data)
{
	QuoteInfo *wanted = user_data;
	QuoteInfo *current;
	gtk_tree_model_get(model, iter, TRADE_COLUMN_QUOTE, &current, -1);
	if (current == wanted) {
		quote_found_iter = *iter;
		quote_found_flag = TRUE;
		return TRUE;
	}
	return FALSE;
}

/** Remove a quote from the list */
static void remove_quote(QuoteView * qv, QuoteInfo * quote)
{
	if (quote == qv->selected_quote)
		set_selected_quote(qv, NULL);

	quote_found_flag = FALSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(qv->store),
			       trade_locate_quote, quote);
	if (quote_found_flag)
		gtk_list_store_remove(qv->store, &quote_found_iter);
	quotelist_delete(qv->quote_list, quote);
}

/** Locate the Player* in user_data. Return TRUE if is found. The iter
 *  is set in quote_found_iter. The flag quote_found_flag is set to TRUE
 */
static gboolean trade_locate_reject(GtkTreeModel * model,
				    G_GNUC_UNUSED GtkTreePath * path,
				    GtkTreeIter * iter, gpointer user_data)
{
	Player *wanted = user_data;
	Player *current;
	gtk_tree_model_get(model, iter, TRADE_COLUMN_REJECT, &current, -1);
	if (current == wanted) {
		quote_found_iter = *iter;
		quote_found_flag = TRUE;
		return TRUE;
	}
	return FALSE;
}

/** Player <I>player_num</I> has rejected trade */
void quote_view_reject(QuoteView * qv, gint player_num)
{
	Player *player = player_get(player_num);
	QuoteInfo *quote;
	GtkTreeIter iter;
	enum TFindResult found;
	GdkPixbuf *pixbuf;

	if (qv->quote_list == NULL)
		return;

	while ((quote =
		quotelist_find_domestic(qv->quote_list, player_num,
					-1)) != NULL) {
		remove_quote(qv, quote);
	}

	quote_found_flag = FALSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(qv->store),
			       trade_locate_reject, player);
	if (quote_found_flag)	/* Already removed */
		return;

	/* work out where to put the reject row
	 */
	for (quote = quotelist_first(qv->quote_list);
	     quote != NULL; quote = quotelist_next(quote))
		if (!quote->is_domestic)
			continue;
		else if (quote->var.d.player_num >= player_num)
			break;

	found =
	    find_integer_in_tree(GTK_TREE_MODEL(qv->store), &iter,
				 TRADE_COLUMN_PLAYER_NUM, player_num);
	if (found != FIND_NO_MATCH)
		gtk_list_store_insert_before(qv->store, &iter, &iter);
	else
		gtk_list_store_append(qv->store, &iter);
	pixbuf = player_create_icon(player_num, TRUE);
	gtk_list_store_set(qv->store, &iter, TRADE_COLUMN_PLAYER, pixbuf,
			   TRADE_COLUMN_POSSIBLE, cross_pixbuf,
			   /* Trade: a player has rejected trade */
			   TRADE_COLUMN_DESCRIPTION, _("Rejected trade"),
			   TRADE_COLUMN_QUOTE, NULL,
			   TRADE_COLUMN_REJECT, player,
			   TRADE_COLUMN_PLAYER_NUM, player_num, -1);
	g_object_unref(pixbuf);
}

/** How many of this resource do we need for a maritime trade?  If the trade is
 *  not possible, return 0.
 */
static gint maritime_amount(QuoteView * qv, gint resource)
{
	if (qv->maritime_info.specific_resource[resource]) {
		if (resource_asset(resource) >= 2)
			return 2;
	} else if (qv->maritime_info.any_resource) {
		if (resource_asset(resource) >= 3)
			return 3;
	} else if (resource_asset(resource) >= 4)
		return 4;
	return 0;
}

/** Check if all existing maritime trades are valid.
 *  Add and remove maritime trades as needed
 */
static void check_maritime_trades(QuoteView * qv)
{
	QuoteInfo *quote;
	gint idx;
	gboolean check_supply = FALSE;
	gint maritime_supply[NO_RESOURCE];

	if (!qv->with_maritime)
		return;

	/* Check supply whenever any supply box is selected.  */
	for (idx = 0; idx < NO_RESOURCE; ++idx) {
		if (qv->maritime_filter_supply[idx])
			check_supply = TRUE;
	}

	/* Check how many of which resources can be used for maritime supply.
	 */
	for (idx = 0; idx < NO_RESOURCE; ++idx) {
		if (check_supply && !qv->maritime_filter_supply[idx])
			maritime_supply[idx] = 0;
		else
			maritime_supply[idx] = maritime_amount(qv, idx);
	}

	/* Remove invalid quotes.  */
	quote = quotelist_first(qv->quote_list);
	while (quote != NULL) {
		QuoteInfo *curr = quote;

		quote = quotelist_next(quote);
		if (curr->is_domestic)
			break;

		/* Is the current quote valid?  */
		if (qv->maritime_filter_receive[curr->var.m.receive] == 0
		    || maritime_supply[curr->var.m.supply] == 0)
			remove_quote(qv, curr);
	}

	/* Add all of the maritime trades that can be performed
	 */
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gint supply_idx;

		if (!qv->maritime_filter_receive[idx])
			continue;

		for (supply_idx = 0; supply_idx < NO_RESOURCE;
		     supply_idx++) {
			if (supply_idx == idx)
				continue;

			if (!maritime_supply[supply_idx])
				continue;

			if (resource_asset(supply_idx)
			    >= maritime_supply[supply_idx])
				add_maritime_trade(qv,
						   maritime_supply
						   [supply_idx], idx,
						   supply_idx);
		}
	}
}

/** Check if the quote still is valid. Update the icon.
 */
static gboolean check_valid_trade(GtkTreeModel * model,
				  G_GNUC_UNUSED GtkTreePath * path,
				  GtkTreeIter * iter, gpointer user_data)
{
	QuoteView *quoteview = QUOTEVIEW(user_data);
	QuoteInfo *quote;
	gtk_tree_model_get(model, iter, TRADE_COLUMN_QUOTE, &quote, -1);
	if (quote != NULL)
		if (quote->is_domestic) {
			gtk_list_store_set(quoteview->store, iter,
					   TRADE_COLUMN_POSSIBLE,
					   quoteview->check_quote_func
					   (quote) ? quoteview->true_pixbuf
					   : quoteview->false_pixbuf, -1);
		}
	return FALSE;
}

/** Add a quote from a player */
void quote_view_add_quote(QuoteView * qv, gint player_num,
			  gint quote_num, const gint * supply,
			  const gint * receive)
{
	GtkTreeIter iter;
	enum TFindResult found;
	QuoteInfo *quote;
	gchar quote_desc[128];
	GdkPixbuf *pixbuf;

	if (qv->quote_list == NULL)
		quotelist_new(&qv->quote_list);

	/* If the trade is already listed, don't duplicate */
	if (quotelist_find_domestic(qv->quote_list, player_num, quote_num)
	    != NULL)
		return;

	quote = quotelist_add_domestic(qv->quote_list,
				       player_num, quote_num, supply,
				       receive);
	trade_format_quote(quote, quote_desc);

	found =
	    find_integer_in_tree(GTK_TREE_MODEL(qv->store), &iter,
				 TRADE_COLUMN_PLAYER_NUM, player_num + 1);

	if (found != FIND_NO_MATCH)
		gtk_list_store_insert_before(qv->store, &iter, &iter);
	else
		gtk_list_store_append(qv->store, &iter);
	pixbuf = player_create_icon(player_num, TRUE);
	gtk_list_store_set(qv->store, &iter, TRADE_COLUMN_PLAYER, pixbuf,
			   TRADE_COLUMN_POSSIBLE,
			   qv->check_quote_func(quote) ? qv->
			   true_pixbuf : qv->false_pixbuf,
			   TRADE_COLUMN_DESCRIPTION, quote_desc,
			   TRADE_COLUMN_QUOTE, quote,
			   TRADE_COLUMN_PLAYER_NUM, player_num, -1);
	g_object_unref(pixbuf);
}

void quote_view_remove_quote(QuoteView * qv, gint partner_num,
			     gint quote_num)
{
	QuoteInfo *quote;

	if (qv->quote_list == NULL)
		return;

	g_assert(qv->quote_list != NULL);
	quote =
	    quotelist_find_domestic(qv->quote_list, partner_num,
				    quote_num);

	if (quote == NULL)
		return;
	g_assert(quote != NULL);
	remove_quote(qv, quote);
}

void quote_view_begin(QuoteView * qv)
{
	quotelist_new(&qv->quote_list);
	if (qv->with_maritime) {
		map_maritime_info(callbacks.get_map(), &qv->maritime_info,
				  my_player_num());
	}

	gtk_list_store_clear(qv->store);
}

void quote_view_finish(QuoteView * qv)
{
	if (qv->quote_list != NULL)
		quotelist_free(&qv->quote_list);
}

void quote_view_check_validity_of_trades(QuoteView * qv)
{
	check_maritime_trades(qv);

	/* Check if all quotes are still valid */
	gtk_tree_model_foreach(GTK_TREE_MODEL(qv->store),
			       check_valid_trade, qv);
}

/** Activate a new quote.
 * If the quote == NULL, clear the selection in the listview too */
static void set_selected_quote(QuoteView * qv, const QuoteInfo * quote)
{
	if (qv->selected_quote == quote)
		return;		/* Don't do the same thing again */

	qv->selected_quote = quote;
	if (quote == NULL)
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection
						(GTK_TREE_VIEW
						 (qv->quotes)));

	g_signal_emit(G_OBJECT(qv), quote_view_signals[SELECTION_CHANGED],
		      0);
}

void quote_view_clear_selected_quote(QuoteView * qv)
{
	set_selected_quote(qv, NULL);
}

const QuoteInfo *quote_view_get_selected_quote(QuoteView * qv)
{
	return qv->selected_quote;
}

void quote_view_remove_rejected_quotes(QuoteView * qv)
{
	gint idx;

	for (idx = 0; idx < num_players(); idx++) {
		Player *player = player_get(idx);
		quote_found_flag = FALSE;
		gtk_tree_model_foreach(GTK_TREE_MODEL(qv->store),
				       trade_locate_reject, player);
		if (quote_found_flag)
			gtk_list_store_remove(qv->store,
					      &quote_found_iter);
	}
}

void quote_view_set_maritime_filters(QuoteView * qv,
				     const gboolean * filter_supply,
				     const gboolean * filter_receive)
{
	gint idx;

	for (idx = 0; idx < NO_RESOURCE; idx++) {
		qv->maritime_filter_supply[idx] = filter_supply[idx];
		qv->maritime_filter_receive[idx] = filter_receive[idx];
	}
	check_maritime_trades(qv);
}

void quote_view_theme_changed(QuoteView * qv)
{
	int width, height;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_rectangle_t extent;
	QuoteInfo *quote;

	if (!qv->with_maritime)
		return;

	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);
	extent.width = width;
	extent.height = height;

	surface =
	    cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA,
					   &extent);
	cr = cairo_create(surface);
	gdk_cairo_set_source_pixbuf(cr,
				    theme_get_terrain_pixbuf(SEA_TERRAIN),
				    0.0, 0.0);
	cairo_pattern_set_extend(cairo_get_source(cr),
				 CAIRO_EXTEND_REPEAT);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	gdk_cairo_set_source_rgba(cr, &black);
	cairo_set_line_width(cr, 1.0);
	cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
	cairo_stroke(cr);
	if (maritime_pixbuf)
		g_object_unref(maritime_pixbuf);
	maritime_pixbuf =
	    gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	/* Remove all maritime quotes */
	quote = quotelist_first(qv->quote_list);
	while (quote != NULL) {
		QuoteInfo *curr = quote;
		quote = quotelist_next(quote);
		if (curr->is_domestic)
			break;
		remove_quote(qv, curr);
	}

	/* Add all of the maritime trades that can be performed */
	check_maritime_trades(qv);
}

gboolean quote_view_trade_exists(QuoteView * qv, const gint * supply,
				 const gint * receive)
{
	const QuoteInfo *quote;
	gboolean match;
	gint idx;

	/* Find the quote which equals the parameters
	 */
	for (quote = quotelist_first(qv->quote_list);
	     quote != NULL; quote = quotelist_next(quote)) {
		if (quote->var.d.player_num != my_player_num())
			continue;
		/* Does this quote equal the parameters?
		 */
		match = TRUE;
		for (idx = 0; idx < NO_RESOURCE && match; idx++)
			if (quote->var.d.supply[idx] != supply[idx]
			    || quote->var.d.receive[idx] != receive[idx])
				match = FALSE;
		if (match)
			return TRUE;
	}
	return FALSE;
}

gboolean quote_view_has_reject(QuoteView * qv, gint player_num)
{
	Player *player = player_get(player_num);

	if (qv->quote_list == NULL)
		return FALSE;

	quote_found_flag = FALSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(qv->store),
			       trade_locate_reject, player);
	return quote_found_flag;
}
