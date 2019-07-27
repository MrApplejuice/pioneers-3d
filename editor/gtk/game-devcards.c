#include "config.h"
#include "game.h"
#include <gtk/gtk.h>
#include <string.h>
#include <glib.h>

#include "game-devcards.h"

static void game_devcards_init(GameDevCards * gd);

/* Register the class */
GType game_devcards_get_type(void)
{
	static GType gd_type = 0;

	if (!gd_type) {
		static const GTypeInfo gd_info = {
			sizeof(GameDevCardsClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			NULL,	/* class init */
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(GameDevCards),
			0,
			(GInstanceInitFunc) game_devcards_init,
			NULL
		};
		gd_type =
		    g_type_register_static(GTK_TYPE_GRID, "GameDevCards",
					   &gd_info, 0);
	}
	return gd_type;
}

/* Build the composite widget */
static void game_devcards_init(GameDevCards * gd)
{
	GtkWidget *label;
	GtkWidget *spin;
	GtkAdjustment *adjustment;
	guint row;

	gtk_grid_set_row_spacing(GTK_GRID(gd), 3);
	gtk_grid_set_column_spacing(GTK_GRID(gd), 5);
	gtk_grid_set_column_homogeneous(GTK_GRID(gd), TRUE);

	for (row = 0; row < NUM_DEVEL_TYPES; row++) {
		label = gtk_label_new(get_devel_name(row));
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		gtk_grid_attach(GTK_GRID(gd), label, 0, row, 1, 1);

		adjustment =
		    GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 100, 1, 5, 0));
		spin =
		    gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
		gtk_widget_set_tooltip_text(spin,
					    get_devel_description(row));
		gtk_entry_set_alignment(GTK_ENTRY(spin), 1.0);
		gtk_grid_attach(GTK_GRID(gd), spin, 1, row, 1, 1);
		gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
		gd->num_cards[row] = GTK_SPIN_BUTTON(spin);
	}
}

/* Create a new instance of the widget */
GtkWidget *game_devcards_new(void)
{
	return GTK_WIDGET(g_object_new(game_devcards_get_type(), NULL));
}

void game_devcards_set_num_cards(GameDevCards * gd, DevelType type,
				 gint num)
{
	gtk_spin_button_set_value(gd->num_cards[type], num);
}

gint game_devcards_get_num_cards(GameDevCards * gd, DevelType type)
{
	return gtk_spin_button_get_value_as_int(gd->num_cards[type]);
}
