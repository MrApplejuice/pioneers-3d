/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2004,2006 Roland Clobus <rclobus@bigfoot.com>
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
#include "frontend.h"
#include "cost.h"
#include "theme.h"
#include "common_gtk.h"
#include "quote-view.h"
#include "notification.h"

static void trade_update(void);

typedef struct {
	GtkWidget *chk;	/**< Checkbox to activate trade in this resource */
	GtkWidget *curr; /**< Amount in possession of this resource */
	Resource resource; /**< The resource */
	gboolean enabled; /**< Trading enabled */
} TradeRow;

static GtkWidget *quoteview;

static TradeRow we_supply_rows[NO_RESOURCE];
static TradeRow we_receive_rows[NO_RESOURCE];

static gint active_supply_request[NO_RESOURCE];
static gint active_receive_request[NO_RESOURCE];
static gboolean trade_since_selection_changed;

/** This button can be hidden in games without interplayer trade */
static GtkWidget *call_btn;
/** This frame can be hidden in games without interplayer trade */
static GtkWidget *we_receive_frame;
/** The last quote that is called */
static GtkWidget *active_quote_label;

/** @return TRUE is we can accept this domestic quote */
static gboolean is_good_quote(const QuoteInfo * quote)
{
	gint idx;
	gboolean interested;

	g_assert(quote != NULL);
	g_assert(quote->is_domestic);

	interested = FALSE;
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		gint we_supply = quote->var.d.receive[idx];
		gint we_receive = quote->var.d.supply[idx];

		/* Asked for too many, or we don't want to give it anymore */
		if (we_supply > resource_asset(idx)
		    || (we_supply > 0 && !we_supply_rows[idx].enabled))
			return FALSE;

		/* We want one of the offered resources */
		if (we_receive > 0 && we_receive_rows[idx].enabled) {
			interested = TRUE;
		}
	}
	return interested;
}

/** @return TRUE if at least one resource is asked/offered */
gboolean can_call_for_quotes(void)
{
	gint idx;
	gboolean have_we_receive;
	gboolean have_we_supply;
	gboolean different_call;

	different_call = FALSE;
	have_we_receive = have_we_supply = FALSE;
	for (idx = 0; idx < NO_RESOURCE; idx++) {
		if (we_receive_rows[idx].enabled !=
		    active_receive_request[idx])
			different_call = TRUE;
		if (we_supply_rows[idx].enabled !=
		    active_supply_request[idx])
			different_call = TRUE;
		if (we_receive_rows[idx].enabled)
			have_we_receive = TRUE;
		if (we_supply_rows[idx].enabled)
			have_we_supply = TRUE;
	}
	/* don't require both supply and receive, for resources may be
	 * given away for free */
	return (have_we_receive || have_we_supply)
	    && can_trade_domestic()
	    && (different_call || trade_since_selection_changed);
}

/** @return the current quote */
const QuoteInfo *trade_current_quote(void)
{
	return quote_view_get_selected_quote(QUOTEVIEW(quoteview));
}

/** Show what the resources will be if the quote is accepted */
static void update_rows(void)
{
	guint idx;
	gint amount;
	gchar str[16];
	const QuoteInfo *quote = trade_current_quote();

	for (idx = 0; idx < G_N_ELEMENTS(we_supply_rows); idx++) {
		Resource resource = we_receive_rows[idx].resource;
		if (!trade_valid_selection())
			amount = 0;
		else if (quote->is_domestic)
			amount =
			    quote->var.d.receive[idx] -
			    quote->var.d.supply[idx];
		else
			amount =
			    (quote->var.m.supply ==
			     resource ? quote->var.m.ratio : 0)
			    - (quote->var.m.receive == resource ? 1 : 0);
		sprintf(str, "%d", resource_asset(resource) - amount);
		gtk_entry_set_text(GTK_ENTRY(we_receive_rows[idx].curr),
				   str);
		gtk_entry_set_text(GTK_ENTRY(we_supply_rows[idx].curr),
				   str);
	}
}

/** @return all resources we supply */
const gint *trade_we_supply(void)
{
	return active_supply_request;
}

/** @return all resources we want to have */
const gint *trade_we_receive(void)
{
	return active_receive_request;
}

/** @return TRUE if a selection is made, and it is valid */
gboolean trade_valid_selection(void)
{
	const QuoteInfo *quote;

	quote = quote_view_get_selected_quote(QUOTEVIEW(quoteview));
	if (quote == NULL)
		return FALSE;
	if (!quote->is_domestic)
		return TRUE;
	return is_good_quote(quote);
}

static void trade_theme_changed(void)
{
	quote_view_theme_changed(QUOTEVIEW(quoteview));
}

static void format_list(gchar * desc, const gint * resources)
{
	gint idx;
	gboolean is_first;

	is_first = TRUE;
	for (idx = 0; idx < NO_RESOURCE; idx++)
		if (resources[idx] > 0) {
			if (!is_first)
				*desc++ = '+';
			if (resources[idx] > 1) {
				sprintf(desc, "%d ", resources[idx]);
				desc += strlen(desc);
			}
			strcpy(desc, resource_name(idx, FALSE));
			desc += strlen(desc);
			is_first = FALSE;
		}
}

void trade_format_quote(const QuoteInfo * quote, gchar * desc)
{
	const gchar *format = NULL;
	gchar buf1[128];
	gchar buf2[128];

	if (resource_count(quote->var.d.supply) == 0) {
		/* trade: you ask for something for free */
		format = _("ask for %s for free");
		format_list(buf1, quote->var.d.receive);
		sprintf(desc, format, buf1);
	} else if (resource_count(quote->var.d.receive) == 0) {
		/* trade: you give something away for free */
		format = _("give %s for free");
		format_list(buf1, quote->var.d.supply);
		sprintf(desc, format, buf1);
	} else {
		/* trade: you trade something for something else */
		format = _("give %s for %s");
		format_list(buf1, quote->var.d.supply);
		format_list(buf2, quote->var.d.receive);
		sprintf(desc, format, buf1, buf2);
	}
}

/** A new trade is started. Keep old quotes, and remove rejection messages.
 */
void trade_new_trade(void)
{
	guint idx;
	gchar we_supply_desc[512];
	gchar we_receive_desc[512];
	gchar desc[512];

	quote_view_remove_rejected_quotes(QUOTEVIEW(quoteview));

	for (idx = 0; idx < G_N_ELEMENTS(active_supply_request); idx++) {
		active_supply_request[idx] = we_supply_rows[idx].enabled;
		active_receive_request[idx] = we_receive_rows[idx].enabled;
	}
	trade_since_selection_changed = FALSE;

	resource_format_type(we_supply_desc, active_supply_request);
	resource_format_type(we_receive_desc, active_receive_request);
	/* I want some resources, and give them some resources */
	g_snprintf(desc, sizeof(desc), _("I want %s, and give them %s"),
		   we_receive_desc, we_supply_desc);
	gtk_label_set_text(GTK_LABEL(active_quote_label), desc);
}

/** A resource checkbox is toggled */
static void toggled_cb(GtkWidget * widget, TradeRow * row)
{
	gint idx;
	gboolean filter[2][NO_RESOURCE];

	row->enabled =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	for (idx = 0; idx < NO_RESOURCE; idx++) {
		filter[0][idx] = we_supply_rows[idx].enabled;
		filter[1][idx] = we_receive_rows[idx].enabled;
	}
	quote_view_clear_selected_quote(QUOTEVIEW(quoteview));
	quote_view_set_maritime_filters(QUOTEVIEW(quoteview), filter[0],
					filter[1]);
	trade_update();
	frontend_gui_update();
}

/** Add a row with widgets for a resource */
static void add_trade_row(GtkWidget * grid, TradeRow * row,
			  Resource resource)
{
	gint col;

	col = 0;
	row->resource = resource;
	row->chk =
	    gtk_check_button_new_with_label(resource_name(resource, TRUE));
	g_signal_connect(G_OBJECT(row->chk), "toggled",
			 G_CALLBACK(toggled_cb), row);
	gtk_widget_show(row->chk);
	gtk_grid_attach(GTK_GRID(grid), row->chk, col, resource, 1, 1);
	col++;

	row->curr = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(row->curr), 3);
	gtk_entry_set_alignment(GTK_ENTRY(row->curr), 1.0);
	gtk_widget_set_sensitive(row->curr, FALSE);
	gtk_widget_show(row->curr);
	gtk_grid_attach(GTK_GRID(grid), row->curr, col, resource, 1, 1);
}

/** Set the sensitivity of the row, and update the assets when applicable */
static void set_row_sensitive(TradeRow * row)
{
	gtk_widget_set_sensitive(row->chk,
				 resource_asset(row->resource) > 0);
}

/** Actions before a domestic trade is performed */
void trade_perform_domestic(G_GNUC_UNUSED gint player_num,
			    gint partner_num, gint quote_num,
			    const gint * they_supply,
			    const gint * they_receive)
{
	cb_trade(partner_num, quote_num, they_supply, they_receive);
}

/** Actions after a domestic trade is performed */
void frontend_trade_domestic(gint partner_num, gint quote_num,
			     G_GNUC_UNUSED const gint * we_supply,
			     G_GNUC_UNUSED const gint * we_receive)
{
	quote_view_remove_quote(QUOTEVIEW(quoteview), partner_num,
				quote_num);
	trade_update();
}

/** Actions before a maritime trade is performed */
void trade_perform_maritime(gint ratio, Resource supply, Resource receive)
{
	cb_maritime(ratio, supply, receive);
}

/** Actions after a maritime trade is performed */
void frontend_trade_maritime(G_GNUC_UNUSED gint ratio,
			     G_GNUC_UNUSED Resource we_supply,
			     G_GNUC_UNUSED Resource we_receive)
{
	quote_view_clear_selected_quote(QUOTEVIEW(quoteview));
	trade_update();
}

/** Add a quote from a player */
void trade_add_quote(gint player_num,
		     gint quote_num, const gint * supply,
		     const gint * receive)
{
	gchar *msg;

	quote_view_add_quote(QUOTEVIEW(quoteview), player_num, quote_num,
			     supply, receive);

	msg = g_strdup_printf(
				     /* Notification */
				     _("Quote received from %s."),
				     player_name(player_num, FALSE));
	notification_send(msg, PIONEERS_PIXMAP_TRADE);
	g_free(msg);
}

void trade_delete_quote(gint player_num, gint quote_num)
{
	quote_view_remove_quote(QUOTEVIEW(quoteview), player_num,
				quote_num);
}

/** A player has rejected the trade. Removes all quotes, and adds a reject
 *  notification.
 */
void trade_player_finish(gint player_num)
{
	quote_view_reject(QUOTEVIEW(quoteview), player_num);
}

/** The trade is finished, hide the page */
void trade_finish(void)
{
	quote_view_finish(QUOTEVIEW(quoteview));
	gui_show_trade_page(FALSE);
}

/** Start a new trade */
void trade_begin(void)
{
	guint idx;

	quote_view_begin(QUOTEVIEW(quoteview));

	for (idx = 0; idx < G_N_ELEMENTS(we_supply_rows); idx++) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
					     (we_supply_rows[idx].chk),
					     FALSE);
		we_supply_rows[idx].enabled = FALSE;
		set_row_sensitive(we_supply_rows + idx);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
					     (we_receive_rows[idx].chk),
					     FALSE);
		we_receive_rows[idx].enabled = FALSE;
		active_receive_request[idx] = 0;
		active_supply_request[idx] = 0;
	}

	if (!can_trade_domestic()) {
		gtk_widget_hide(we_receive_frame);
		gtk_widget_hide(call_btn);
		gtk_widget_hide(active_quote_label);
	} else {
		gtk_widget_show(we_receive_frame);
		gtk_widget_show(call_btn);
		gtk_widget_show(active_quote_label);
		gtk_label_set_text(GTK_LABEL(active_quote_label), "");
	}
	quote_view_clear_selected_quote(QUOTEVIEW(quoteview));
	update_rows();		/* Always update */
	gui_show_trade_page(TRUE);
}

static void quote_dblclick_cb(G_GNUC_UNUSED QuoteView * quoteview,
			      gpointer accept_btn)
{
	if (trade_valid_selection())
		gtk_button_clicked(GTK_BUTTON(accept_btn));
}

static void quote_selected_cb(G_GNUC_UNUSED QuoteView * quoteview,
			      G_GNUC_UNUSED gpointer user_data)
{
	update_rows();
	frontend_gui_update();
}

/** Build a trade resources composite widget.
 *  @param title The title for the composite widget.
 *  @param trade_rows Pointer to TradeRow to store each row's data.
 *  @return Returns the composite widget.
 */
static GtkWidget *build_trade_resources_frame(const gchar * title,
					      TradeRow * trade_rows)
{
	/* vbox */
	/*       label */
	/*       alignment */
	/*               grid */
	/*                       trade rows */

	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *alignment;
	GtkWidget *grid;

	gchar *title_with_markup;
	gint idx;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show(vbox);

	label = gtk_label_new(NULL);
	title_with_markup = g_strdup_printf("<b>%s</b>", title);
	gtk_label_set_markup(GTK_LABEL(label), title_with_markup);
	g_free(title_with_markup);
	gtk_widget_show(label);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

	alignment = gtk_alignment_new(0.0, 0.0, 0.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 3, 0 * 12,
				  0);
	gtk_widget_show(alignment);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_container_add(GTK_CONTAINER(alignment), grid);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 0);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 3);

	/* trade rows, one for each resource */
	for (idx = 0; idx < NO_RESOURCE; ++idx)
		add_trade_row(grid, trade_rows + idx, idx);

	return vbox;
}

/** Build the trading page.
 *  @return Returns the composite widget.
 */
GtkWidget *trade_build_page(void)
{
	/* scroll window */
	/*       hbox - panel_mainbox */
	/*               vbox */
	/*                       trade_resources_frame */
	/*                       trade_resources_frame */
	/*                       hbox - bbox */
	/*                               call_btn */
	/*               vbox */
	/*                       active_quote_label */
	/*                       quoteview */
	/*                       hbox - bbox */
	/*                               accept_btn */
	/*                               finish_btn */

	GtkWidget *scroll_win;
	GtkWidget *panel_mainbox;
	GtkWidget *vbox;
	GtkWidget *bbox;
	GtkWidget *finish_btn;
	GtkWidget *accept_btn;

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (scroll_win), GTK_SHADOW_NONE);
	gtk_widget_show(scroll_win);

	panel_mainbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show(panel_mainbox);
	gtk_container_set_border_width(GTK_CONTAINER(panel_mainbox), 6);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW
					      (scroll_win), panel_mainbox);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(panel_mainbox), vbox, FALSE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox),
			   /* Frame title, trade: I want to trade these resources */
			   build_trade_resources_frame(_("I want"),
						       we_receive_rows),
			   FALSE, TRUE, 0);

	we_receive_frame =
	    /* Frame title, trade: I want these resources in return */
	    build_trade_resources_frame(_("Give them"), we_supply_rows);
	gtk_box_pack_start(GTK_BOX(vbox), we_receive_frame, FALSE, TRUE,
			   0);

	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);

	/* Button text, trade: call for quotes from other players */
	call_btn = gtk_button_new_with_mnemonic(_("_Call for Quotes"));
	frontend_gui_register(call_btn, GUI_TRADE_CALL, "clicked");
	gtk_widget_show(call_btn);
	gtk_container_add(GTK_CONTAINER(bbox), call_btn);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(panel_mainbox), vbox, TRUE, TRUE, 0);

	active_quote_label = gtk_label_new("");
	gtk_widget_show(active_quote_label);
	gtk_label_set_xalign(GTK_LABEL(active_quote_label), 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), active_quote_label,
			   FALSE, FALSE, 0);

	quoteview =
	    quote_view_new(TRUE, is_good_quote, "pioneers-checkmark",
			   "pioneers-cross");
	gtk_widget_show(quoteview);
	gtk_box_pack_start(GTK_BOX(vbox), quoteview, TRUE, TRUE, 0);
	g_signal_connect(QUOTEVIEW(quoteview), "selection-changed",
			 G_CALLBACK(quote_selected_cb), NULL);

	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);

	/* Button text: Trade page, accept selected quote */
	accept_btn = gtk_button_new_with_mnemonic(_("_Accept Quote"));
	frontend_gui_register(accept_btn, GUI_TRADE_ACCEPT, "clicked");
	gtk_widget_show(accept_btn);
	gtk_container_add(GTK_CONTAINER(bbox), accept_btn);

	/* Button text: Trade page, finish trading */
	finish_btn = gtk_button_new_with_mnemonic(_("_Finish Trading"));
	frontend_gui_register(finish_btn, GUI_TRADE_FINISH, "clicked");
	gtk_widget_show(finish_btn);
	gtk_container_add(GTK_CONTAINER(bbox), finish_btn);

	g_signal_connect(G_OBJECT(quoteview), "selection-activated",
			 G_CALLBACK(quote_dblclick_cb), accept_btn);

	theme_register_callback(G_CALLBACK(trade_theme_changed));

	return scroll_win;
}

/** A trade is performed/a new trade is possible */
static void trade_update(void)
{
	guint idx;

	for (idx = 0; idx < G_N_ELEMENTS(we_supply_rows); idx++) {
		if (resource_asset(idx) == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
						     (we_supply_rows
						      [idx].chk), FALSE);
			we_supply_rows[idx].enabled = FALSE;
		}
		set_row_sensitive(we_supply_rows + idx);
	}
	quote_view_check_validity_of_trades(QUOTEVIEW(quoteview));
	trade_since_selection_changed = TRUE;
}
