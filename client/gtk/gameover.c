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

#include "config.h"
#include "frontend.h"

GtkWidget *gameover_create_dlg(gint player_num, gint num_points)
{
	GtkWidget *dlg;
	GtkWidget *dlg_vbox;
	GtkWidget *vbox;
	GtkWidget *lbl;
	char buff[512];

	dlg = gtk_dialog_new_with_buttons(
						 /* Dialog caption */
						 _("Game Over"),
						 GTK_WINDOW(app_window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 /* Button text */
						 _("_OK"),
						 GTK_RESPONSE_OK, NULL);
	gtk_widget_realize(dlg);
	gdk_window_set_functions(gtk_widget_get_window(dlg),
				 GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	dlg_vbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	gtk_widget_show(dlg_vbox);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 50);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);

	sprintf(buff, _("%s has won the game with %d victory points!"),
		player_name(player_num, TRUE), num_points);
	lbl = gtk_label_new(buff);
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, TRUE, 0);

	sprintf(buff, _("All praise %s, Lord of the known world!"),
		player_name(player_num, TRUE));
	lbl = gtk_label_new(buff);
	gtk_widget_show(lbl);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, TRUE, 0);

	gtk_widget_show(dlg);
	g_signal_connect(dlg, "response",
			 G_CALLBACK(gtk_widget_destroy), NULL);

	return dlg;
}
