/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2009 Andreas Steinel <lnxbil@users.sourceforge.net>
 * Copyright (C) 2010 Roland Clobus <rclobus@rclobus.nl>
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "avahi-browser.h"

static void avahibrowser_class_init(AvahiBrowserClass * klass);
static void avahibrowser_init(AvahiBrowser * ab);

/* Register the class */
GType avahibrowser_get_type(void)
{
	static GType sg_type = 0;

	if (!sg_type) {
		static const GTypeInfo sg_info = {
			sizeof(AvahiBrowserClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) avahibrowser_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(AvahiBrowser),
			0,
			(GInstanceInitFunc) avahibrowser_init,
			NULL
		};
		sg_type =
		    g_type_register_static(GTK_TYPE_GRID,
					   "AvahiBrowser", &sg_info, 0);
	}
	return sg_type;
}

static void avahibrowser_class_init(G_GNUC_UNUSED AvahiBrowserClass *
				    klass)
{
}

/* Build the composite widget */
static void avahibrowser_init(AvahiBrowser * ab)
{
	GtkCellRenderer *cell;

	/* Create model */
	ab->data =
	    gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_STRING,
			       G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			       G_TYPE_STRING, G_TYPE_STRING);
	ab->combo_box =
	    gtk_combo_box_new_with_model(GTK_TREE_MODEL(ab->data));

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ab->combo_box), cell,
				   TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ab->combo_box),
				       cell, "text", 6, NULL);

	gtk_widget_show(ab->combo_box);
	gtk_widget_set_tooltip_text(ab->combo_box,
				    _
				    ("Select an automatically discovered game"));
	gtk_grid_attach(GTK_GRID(ab), ab->combo_box, 0, 1, 1, 1);
	gtk_widget_set_hexpand(ab->combo_box, TRUE);
}

/* Create a new instance of the widget */
GtkWidget *avahibrowser_new(GtkWidget * connect_button)
{
	AvahiBrowser *ab =
	    AVAHIBROWSER(g_object_new(avahibrowser_get_type(), NULL));
	ab->connect_button = connect_button;
	gtk_widget_set_sensitive(ab->connect_button, FALSE);
	return GTK_WIDGET(ab);
}

void avahibrowser_add(AvahiBrowser * ab, const char *service_name,
		      const char *resolved_hostname, const char *host_name,
		      const gchar * port, const char *version,
		      const char *title)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ab->data), &iter)) {
		gchar *old;
		gboolean found = FALSE;
		gboolean atend = FALSE;
		do {
			gtk_tree_model_get(GTK_TREE_MODEL(ab->data), &iter,
					   0, &old, -1);
			if (!strcmp(service_name, old))
				found = TRUE;
			else
				atend =
				    !gtk_tree_model_iter_next
				    (GTK_TREE_MODEL(ab->data), &iter);
			g_free(old);
		} while (!found && !atend);
		if (!found) {
			gtk_list_store_append(ab->data, &iter);
		}
	} else {
		/* Was empty */
		gtk_list_store_append(ab->data, &iter);
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ab->combo_box),
					      &iter);
	}

	gchar *nice_text = g_strdup_printf(
						  /* $1=Game title, $2=version, $3=host_name, $4=port */
						  _("%s (%s) on %s:%s"),
						  title, version,
						  host_name, port);
	gtk_list_store_set(ab->data, &iter, 0, service_name, 1,
			   resolved_hostname, 2, host_name, 3, port, 4,
			   version, 5, title, 6, nice_text, -1);
	g_free(nice_text);
	gtk_widget_set_sensitive(GTK_WIDGET(ab->connect_button), TRUE);
}

void avahibrowser_del(AvahiBrowser * ab, const char *service_name)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ab->data), &iter)) {
		gchar *old;
		gboolean found = FALSE;
		gboolean atend = FALSE;
		do {
			gtk_tree_model_get(GTK_TREE_MODEL(ab->data), &iter,
					   0, &old, -1);

			if (!strcmp(service_name, old))
				found = TRUE;
			else
				atend =
				    !gtk_tree_model_iter_next
				    (GTK_TREE_MODEL(ab->data), &iter);
			g_free(old);
		} while (!found && !atend);
		if (found)
			gtk_list_store_remove(ab->data, &iter);
	}
	/* If there is more than one server available, select the first one */
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ab->data), &iter)) {
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ab->combo_box),
					      &iter);
	} else {
		/* if no server is available, disable the join button */
		gtk_widget_set_sensitive(GTK_WIDGET
					 (ab->connect_button), FALSE);
	}
}

gchar *avahibrowser_get_server(AvahiBrowser * ab)
{
	GtkTreeIter iter;
	gchar *text;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(ab->combo_box), &iter);

	gtk_tree_model_get(GTK_TREE_MODEL(ab->data), &iter, 1, &text, -1);
	return text;
}

gchar *avahibrowser_get_port(AvahiBrowser * ab)
{
	GtkTreeIter iter;
	gchar *text;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(ab->combo_box), &iter);

	gtk_tree_model_get(GTK_TREE_MODEL(ab->data), &iter, 3, &text, -1);
	return text;
}
