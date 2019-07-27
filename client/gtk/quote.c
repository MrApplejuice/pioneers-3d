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

#include "config.h"
#include "frontend.h"
#include "quoteinfo.h"
#include "resource-table.h"
#include "quote-view.h"
#include "notification.h"

static gint trade_player;

static GtkWidget *player_icon;
static GtkWidget *desc_lbl;
static GtkWidget *submit_btn;
static GtkWidget *delete_btn;
static GtkWidget *reject_btn;
static GtkWidget *quoteview;

static GtkWidget *want_table;
static GtkWidget *give_table;

static gint next_quote_num;

static gint we_supply[NO_RESOURCE];
static gint we_receive[NO_RESOURCE];

static gboolean can_delete_this_quote(const QuoteInfo * quote)
{
	g_assert(quote->is_domestic);
	return quote->var.d.player_num == my_player_num();;
}

gboolean can_submit_quote(void)
{
	gint want_quote[NO_RESOURCE];
	gint give_quote[NO_RESOURCE];

	resource_table_get_amount(RESOURCETABLE(want_table), want_quote);
	resource_table_get_amount(RESOURCETABLE(give_table), give_quote);

	if (resource_count(want_quote) == 0
	    && resource_count(give_quote) == 0)
		return FALSE;

	return !quote_view_trade_exists(QUOTEVIEW(quoteview), give_quote,
					want_quote)
	    && !player_is_spectator(my_player_num());
}

gboolean can_delete_quote(void)
{
	const QuoteInfo *selected_quote = quote_current_quote();
	return selected_quote != NULL
	    && can_delete_this_quote(selected_quote);
}

gboolean can_reject_quote(void)
{
	return !player_is_spectator(my_player_num()) &&
	    !quote_view_has_reject(QUOTEVIEW(quoteview), my_player_num());
}

const QuoteInfo *quote_current_quote(void)
{
	return quote_view_get_selected_quote(QUOTEVIEW(quoteview));
}

const gint *quote_we_supply(void)
{
	static gint we_supply[NO_RESOURCE];

	resource_table_get_amount(RESOURCETABLE(give_table), we_supply);
	return we_supply;
}

const gint *quote_we_receive(void)
{
	static gint we_receive[NO_RESOURCE];

	resource_table_get_amount(RESOURCETABLE(want_table), we_receive);
	return we_receive;
}

gint quote_next_num(void)
{
	return next_quote_num;
}

static void quote_update(void)
{
	resource_table_update_hand(RESOURCETABLE(want_table));
	resource_table_update_hand(RESOURCETABLE(give_table));
}

static void lock_resource_tables(void)
{
	gint idx;
	gint filter[NO_RESOURCE];

	/* Lock the UI */
	for (idx = 0; idx < NO_RESOURCE; idx++)
		filter[idx] = 0;
	resource_table_set_filter(RESOURCETABLE(want_table), filter);
	resource_table_set_filter(RESOURCETABLE(give_table), filter);
	resource_table_clear(RESOURCETABLE(want_table));
	resource_table_clear(RESOURCETABLE(give_table));
}

static void set_resource_tables_filter(const gint * we_receive, const gint
				       * we_supply)
{
	if (player_is_spectator(my_player_num())) {
		lock_resource_tables();
	} else {
		resource_table_set_filter(RESOURCETABLE(want_table),
					  we_receive);
		resource_table_set_filter(RESOURCETABLE(give_table),
					  we_supply);
		resource_table_clear(RESOURCETABLE(want_table));
		resource_table_clear(RESOURCETABLE(give_table));
	}
}

void quote_add_quote(gint player_num,
		     gint quote_num, const gint * we_supply,
		     const gint * we_receive)
{
	quote_view_add_quote(QUOTEVIEW(quoteview), player_num, quote_num,
			     we_supply, we_receive);
	next_quote_num++;
}

void quote_delete_quote(gint player_num, gint quote_num)
{
	quote_view_remove_quote(QUOTEVIEW(quoteview), player_num,
				quote_num);
}

void quote_player_finish(gint player_num)
{
	quote_view_reject(QUOTEVIEW(quoteview), player_num);
	if (player_num == my_player_num()) {
		/* Lock the UI */
		lock_resource_tables();
	}
}

void quote_finish(void)
{
	quote_view_finish(QUOTEVIEW(quoteview));
	gui_show_quote_page(FALSE);
}

static void show_quote_params(gint player_num,
			      const gint * they_supply,
			      const gint * they_receive)
{
	gchar we_supply_desc[512];
	gchar we_receive_desc[512];
	gchar desc[512];
	GdkPixbuf *icon;

	trade_player = player_num;
	resource_format_type(we_supply_desc, they_receive);
	resource_format_type(we_receive_desc, they_supply);
	g_snprintf(desc, sizeof(desc),
		   _("%s has %s, and is looking for %s"),
		   player_name(player_num, TRUE), we_receive_desc,
		   we_supply_desc);
	gtk_label_set_text(GTK_LABEL(desc_lbl), desc);

	icon = player_create_icon(player_num, TRUE);
	gtk_image_set_from_pixbuf(GTK_IMAGE(player_icon), icon);
	g_object_unref(icon);

	memcpy(we_supply, they_receive, sizeof(we_supply));
	memcpy(we_receive, they_supply, sizeof(we_receive));
}

void quote_begin_again(gint player_num, const gint * we_receive,
		       const gint * we_supply)
{
	gchar *msg;

	/* show the new parameters */
	show_quote_params(player_num, we_receive, we_supply);
	/* throw out reject rows: everyone can quote again */
	quote_view_remove_rejected_quotes(QUOTEVIEW(quoteview));
	/* check if existing quotes are still valid */
	quote_view_check_validity_of_trades(QUOTEVIEW(quoteview));
	/* update everything */
	quote_update();
	set_resource_tables_filter(we_receive, we_supply);
	frontend_gui_update();
	msg = g_strdup_printf(
				     /* Notification */
				     _("New offer from %s."),
				     player_name(player_num, FALSE));
	notification_send(msg, PIONEERS_PIXMAP_TRADE);
	g_free(msg);
}

void quote_begin(gint player_num, const gint * we_receive,
		 const gint * we_supply)
{
	gchar *msg;

	/* show what is asked */
	show_quote_params(player_num, we_receive, we_supply);
	/* reset variables */
	next_quote_num = 0;
	/* clear the gui list */
	quote_view_begin(QUOTEVIEW(quoteview));
	/* initialize our offer */
	quote_update();
	set_resource_tables_filter(we_receive, we_supply);
	frontend_gui_update();
	/* finally, show the page so the user can see it */
	gui_show_quote_page(TRUE);
	msg = g_strdup_printf(
				     /* Notification */
				     _("Offer from %s."),
				     player_name(player_num, FALSE));
	notification_send(msg, PIONEERS_PIXMAP_TRADE);
	g_free(msg);
}

static void quote_selected_cb(G_GNUC_UNUSED QuoteView * quoteview,
			      G_GNUC_UNUSED gpointer user_data)
{
	/** @todo RC 2006-05-27 Update the resource tables,
	 *  to show the effect of the selected quote
	 */
	frontend_gui_update();
}

static void quote_dblclick_cb(G_GNUC_UNUSED QuoteView * quoteview,
			      gpointer delete_btn)
{
	if (can_delete_quote())
		gtk_button_clicked(GTK_BUTTON(delete_btn));
}

static void amount_changed_cb(G_GNUC_UNUSED ResourceTable * rt,
			      G_GNUC_UNUSED gpointer user_data)
{
	quote_view_clear_selected_quote(QUOTEVIEW(quoteview));
	frontend_gui_update();
}

GtkWidget *quote_build_page(void)
{
	GtkWidget *scroll_win;
	GtkWidget *panel_vbox;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *bbox;

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (scroll_win), GTK_SHADOW_NONE);
	gtk_widget_show(scroll_win);

	panel_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_widget_show(panel_vbox);
	gtk_container_set_border_width(GTK_CONTAINER(panel_vbox), 6);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW
					      (scroll_win), panel_vbox);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(panel_vbox), hbox, FALSE, TRUE, 0);

	player_icon = gtk_image_new();
	gtk_widget_show(player_icon);
	gtk_box_pack_start(GTK_BOX(hbox), player_icon, FALSE, FALSE, 0);

	desc_lbl = gtk_label_new("");
	gtk_widget_show(desc_lbl);
	gtk_box_pack_start(GTK_BOX(hbox), desc_lbl, TRUE, TRUE, 0);
	gtk_label_set_xalign(GTK_LABEL(desc_lbl), 0.0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(panel_vbox), hbox, TRUE, TRUE, 0);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);

	want_table = resource_table_new(
					       /* Label */
					       _("I want"),
					       RESOURCE_TABLE_MORE_IN_HAND,
					       FALSE);
	gtk_widget_show(want_table);
	gtk_box_pack_start(GTK_BOX(vbox), want_table, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(want_table), "change",
			 G_CALLBACK(amount_changed_cb), NULL);

	give_table = resource_table_new(
					       /* Label */
					       _("Give them"),
					       RESOURCE_TABLE_LESS_IN_HAND,
					       FALSE);
	gtk_widget_show(give_table);
	gtk_box_pack_start(GTK_BOX(vbox), give_table, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(give_table), "change",
			 G_CALLBACK(amount_changed_cb), NULL);

	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);

	/* Button text */
	submit_btn = gtk_button_new_with_label(_("Quote"));
	frontend_gui_register(submit_btn, GUI_QUOTE_SUBMIT, "clicked");
	gtk_widget_show(submit_btn);
	gtk_container_add(GTK_CONTAINER(bbox), submit_btn);

	/* Button text */
	delete_btn = gtk_button_new_with_label(_("Delete"));
	frontend_gui_register(delete_btn, GUI_QUOTE_DELETE, "clicked");
	gtk_widget_show(delete_btn);
	gtk_container_add(GTK_CONTAINER(bbox), delete_btn);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	quoteview = quote_view_new(FALSE, can_delete_this_quote,
				   "edit-delete", NULL);
	gtk_widget_show(quoteview);
	gtk_box_pack_start(GTK_BOX(vbox), quoteview, TRUE, TRUE, 0);
	g_signal_connect(QUOTEVIEW(quoteview), "selection-changed",
			 G_CALLBACK(quote_selected_cb), NULL);
	g_signal_connect(G_OBJECT(quoteview), "selection-activated",
			 G_CALLBACK(quote_dblclick_cb), delete_btn);

	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(bbox);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox),
				  GTK_BUTTONBOX_SPREAD);

	/* Button text */
	reject_btn = gtk_button_new_with_label(_("Reject Domestic Trade"));
	frontend_gui_register(reject_btn, GUI_QUOTE_REJECT, "clicked");
	gtk_widget_show(reject_btn);
	gtk_container_add(GTK_CONTAINER(bbox), reject_btn);

	return scroll_win;
}

void frontend_quote_trade(G_GNUC_UNUSED gint player_num, gint partner_num,
			  gint quote_num,
			  G_GNUC_UNUSED const gint * they_supply,
			  G_GNUC_UNUSED const gint * they_receive)
{
	/* a quote has been accepted, remove it from the list. */
	quote_view_remove_quote(QUOTEVIEW(quoteview), partner_num,
				quote_num);
	quote_update();
	frontend_gui_update();
}
