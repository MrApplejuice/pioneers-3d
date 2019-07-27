#include "config.h"
#include "game.h"
#include <gtk/gtk.h>
#include <string.h>
#include <glib.h>

#include "game-resources.h"

static void game_resources_init(GameResources * gr);

/* Register the class */
GType game_resources_get_type(void)
{
	static GType gr_type = 0;

	if (!gr_type) {
		static const GTypeInfo gr_info = {
			sizeof(GameResourcesClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			NULL,	/* class init */
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(GameResources),
			0,
			(GInstanceInitFunc) game_resources_init,
			NULL
		};
		gr_type =
		    g_type_register_static(GTK_TYPE_GRID, "GameResources",
					   &gr_info, 0);
	}
	return gr_type;
}

/* Build the composite widget */
static void game_resources_init(GameResources * gr)
{
	GtkWidget *label;
	GtkWidget *spin;
	GtkAdjustment *adjustment;

	gtk_grid_set_row_spacing(GTK_GRID(gr), 3);
	gtk_grid_set_column_spacing(GTK_GRID(gr), 5);
	gtk_grid_set_column_homogeneous(GTK_GRID(gr), TRUE);

	label = gtk_label_new(_("Resource count"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_grid_attach(GTK_GRID(gr), label, 0, 0, 1, 1);

	adjustment =
	    GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 100, 1, 5, 0));
	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
	gtk_entry_set_alignment(GTK_ENTRY(spin), 1.0);
	gtk_grid_attach(GTK_GRID(gr), spin, 1, 0, 1, 1);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
	gr->num_resources = GTK_SPIN_BUTTON(spin);
}

/* Create a new instance of the widget */
GtkWidget *game_resources_new(void)
{
	return GTK_WIDGET(g_object_new(game_resources_get_type(), NULL));
}

void game_resources_set_num_resources(GameResources * gr, gint num)
{
	gtk_spin_button_set_value(gr->num_resources, num);
}

gint game_resources_get_num_resources(GameResources * gr)
{
	return gtk_spin_button_get_value_as_int(gr->num_resources);
}
