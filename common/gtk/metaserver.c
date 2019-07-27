/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2008 Roland Clobus <rclobus@bigfoot.com>
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>
#include "network.h"
#include "config-gnome.h"

#include "metaserver.h"

static void metaserver_class_init(MetaServerClass * klass);
static void metaserver_init(MetaServer * ms);

/* Register the class */
GType metaserver_get_type(void)
{
	static GType sg_type = 0;

	if (!sg_type) {
		static const GTypeInfo sg_info = {
			sizeof(MetaServerClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) metaserver_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(MetaServer),
			0,
			(GInstanceInitFunc) metaserver_init,
			NULL
		};
		sg_type =
		    g_type_register_static(GTK_TYPE_GRID, "MetaServer",
					   &sg_info, 0);
	}
	return sg_type;
}

static void metaserver_class_init(G_GNUC_UNUSED MetaServerClass * klass)
{
}

/* Build the composite widget */
static void metaserver_init(MetaServer * ms)
{
	GtkTreeIter iter;
	GtkCellRenderer *cell;
	gchar *default_metaserver_name;
	gchar *custom_metaserver_name;
	gboolean novar;

	/* Create model */
	ms->data = gtk_list_store_new(1, G_TYPE_STRING);
	ms->combo_box =
	    gtk_combo_box_new_with_model(GTK_TREE_MODEL(ms->data));

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ms->combo_box),
				   cell, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ms->combo_box),
				       cell, "text", 0, NULL);

	gtk_widget_show(ms->combo_box);
	gtk_widget_set_hexpand(ms->combo_box, TRUE);
	gtk_widget_set_tooltip_text(ms->combo_box,
				    /* Tooltip for the list of metaservers */
				    _("Select a metaserver"));
	gtk_grid_attach(GTK_GRID(ms), ms->combo_box, 0, 0, 1, 1);

	/* Default metaserver */
	default_metaserver_name = get_metaserver_name(TRUE);
	metaserver_add(ms, default_metaserver_name);
	g_free(default_metaserver_name);

	/* Custom metaserver */
	custom_metaserver_name =
	    config_get_string
	    ("server/custom-metaserver=pioneers.game-host.org", &novar);
	metaserver_add(ms, custom_metaserver_name);
	g_free(custom_metaserver_name);

	/* Select the first item.
	 * When later metaserver_add is called, it will set the current metaserver */
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ms->data), &iter);
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ms->combo_box), &iter);
}

/* Create a new instance of the widget */
GtkWidget *metaserver_new(void)
{
	return GTK_WIDGET(g_object_new(metaserver_get_type(), NULL));
}

void metaserver_add(MetaServer * ms, const gchar * text)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ms->data), &iter)) {
		gchar *old;
		gboolean found = FALSE;
		gboolean atend = FALSE;
		do {
			gtk_tree_model_get(GTK_TREE_MODEL(ms->data), &iter,
					   0, &old, -1);
			if (!strcmp(text, old))
				found = TRUE;
			else
				atend =
				    !gtk_tree_model_iter_next
				    (GTK_TREE_MODEL(ms->data), &iter);
			g_free(old);
		} while (!found && !atend);
		if (!found)
			gtk_list_store_insert_with_values(ms->data, &iter,
							  999, 0, text,
							  -1);
	} else {
		/* Was empty */
		gtk_list_store_insert_with_values(ms->data, &iter, 999,
						  0, text, -1);
	}
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ms->combo_box), &iter);
}

gchar *metaserver_get(MetaServer * ms)
{
	GtkTreeIter iter;
	gchar *text;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(ms->combo_box), &iter);

	gtk_tree_model_get(GTK_TREE_MODEL(ms->data), &iter, 0, &text, -1);
	return text;
}
