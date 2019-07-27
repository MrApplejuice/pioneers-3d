#include "config.h"
#include "game.h"
#include <gtk/gtk.h>
#include <string.h>
#include <glib.h>

#include "game-buildings.h"

static const gchar *building_names[NUM_BUILD_TYPES] = {
	NULL, N_("Road"), N_("Bridge"), N_("Ship"), N_("Settlement"),
	N_("City"), N_("City wall")
};

static void game_buildings_init(GameBuildings * gb);

/* Register the class */
GType game_buildings_get_type(void)
{
	static GType gb_type = 0;

	if (!gb_type) {
		static const GTypeInfo gb_info = {
			sizeof(GameBuildingsClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			NULL,	/* class init */
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(GameBuildings),
			0,
			(GInstanceInitFunc) game_buildings_init,
			NULL
		};
		gb_type =
		    g_type_register_static(GTK_TYPE_GRID, "GameBuildings",
					   &gb_info, 0);
	}
	return gb_type;
}

/* Build the composite widget */
static void game_buildings_init(GameBuildings * gb)
{
	GtkWidget *label;
	GtkWidget *spin;
	GtkAdjustment *adjustment;
	guint row;

	gtk_grid_set_row_spacing(GTK_GRID(gb), 3);
	gtk_grid_set_column_spacing(GTK_GRID(gb), 5);
	gtk_grid_set_column_homogeneous(GTK_GRID(gb), TRUE);

	for (row = 1; row < NUM_BUILD_TYPES; row++) {
		label = gtk_label_new(gettext(building_names[row]));
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		gtk_grid_attach(GTK_GRID(gb), label, 0, row - 1, 1, 1);

		adjustment =
		    GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 100, 1, 5, 0));
		spin =
		    gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
		gtk_entry_set_alignment(GTK_ENTRY(spin), 1.0);
		gtk_grid_attach(GTK_GRID(gb), spin, 1, row - 1, 1, 1);
		gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
		gb->num_buildings[row] = GTK_SPIN_BUTTON(spin);
	}
}

/* Create a new instance of the widget */
GtkWidget *game_buildings_new(void)
{
	return GTK_WIDGET(g_object_new(game_buildings_get_type(), NULL));
}

void game_buildings_set_num_buildings(GameBuildings * gb, gint type,
				      gint num)
{
	gtk_spin_button_set_value(gb->num_buildings[type], num);
}

gint game_buildings_get_num_buildings(GameBuildings * gb, gint type)
{
	return gtk_spin_button_get_value_as_int(gb->num_buildings[type]);
}
