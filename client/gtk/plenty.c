/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
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

#include "config.h"
#include "frontend.h"
#include "resource-table.h"

static struct {
	GtkWidget *dlg;
	GtkWidget *resource_widget;
	gint bank[NO_RESOURCE];
} plenty;

static void amount_changed_cb(G_GNUC_UNUSED ResourceTable * rt,
			      G_GNUC_UNUSED gpointer user_data)
{
	frontend_gui_update();
}

void plenty_resources(gint * resources)
{
	resource_table_get_amount(RESOURCETABLE(plenty.resource_widget),
				  resources);
}

static void plenty_destroyed(G_GNUC_UNUSED GtkWidget * widget,
			     G_GNUC_UNUSED gpointer data)
{
	if (callback_mode == MODE_PLENTY)
		plenty_create_dlg(NULL);
}

void plenty_create_dlg(const gint * bank)
{
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	const char *str;
	gint r;
	gint total;

	plenty.dlg = gtk_dialog_new_with_buttons(
							/* Dialog caption */
							_(""
							  "Year of Plenty"),
							GTK_WINDOW
							(app_window),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							/* Button text */
							_("_OK"),
							GTK_RESPONSE_OK,
							NULL);
	g_signal_connect(G_OBJECT(plenty.dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &plenty.dlg);
	gtk_widget_realize(plenty.dlg);
	/* Disable close */
	gdk_window_set_functions(gtk_widget_get_window(plenty.dlg),
				 GDK_FUNC_ALL | GDK_FUNC_CLOSE);

	dlg_vbox = gtk_dialog_get_content_area(GTK_DIALOG(plenty.dlg));
	gtk_widget_show(dlg_vbox);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	total = 0;
	for (r = 0; r < NO_RESOURCE; ++r) {
		if (bank != NULL)
			plenty.bank[r] = bank[r];
		total += plenty.bank[r];
	}
	if (total == 1)
		str = _("Please choose one resource from the bank");
	else if (total >= 2) {
		total = 2;
		str = _("Please choose two resources from the bank");
	} else
		str = _("The bank is empty");
	plenty.resource_widget =
	    resource_table_new(str, RESOURCE_TABLE_MORE_IN_HAND, TRUE);
	resource_table_set_total(RESOURCETABLE(plenty.resource_widget),
				 /* Text for total in year of plenty dialog */
				 _("Total resources"), total);
	resource_table_limit_bank(RESOURCETABLE(plenty.resource_widget),
				  TRUE);
	resource_table_set_bank(RESOURCETABLE(plenty.resource_widget),
				plenty.bank);

	gtk_widget_show(plenty.resource_widget);
	gtk_box_pack_start(GTK_BOX(vbox), plenty.resource_widget, FALSE,
			   TRUE, 0);
	g_signal_connect(G_OBJECT(plenty.resource_widget), "change",
			 G_CALLBACK(amount_changed_cb), NULL);

	frontend_gui_register(gui_get_dialog_button
			      (GTK_DIALOG(plenty.dlg), 0), GUI_PLENTY,
			      "clicked");
	/* This _must_ be after frontend_gui_register, otherwise the
	 * regeneration of the button happens before the destruction, which
	 * results in an incorrectly sensitive OK button. */
	g_signal_connect(gui_get_dialog_button(GTK_DIALOG(plenty.dlg), 0),
			 "destroy", G_CALLBACK(plenty_destroyed), NULL);
	gtk_widget_show(plenty.dlg);
	frontend_gui_update();
}

void plenty_destroy_dlg(void)
{
	if (plenty.dlg == NULL)
		return;
	gtk_widget_destroy(plenty.dlg);
	plenty.dlg = NULL;
}

gboolean plenty_can_activate(void)
{
	return
	    resource_table_is_total_reached(RESOURCETABLE
					    (plenty.resource_widget));
}
