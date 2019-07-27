/* A custom widget for selecting a game from a list of games.
 *
 * The code is based on the TICTACTOE example
 * www.gtk.oorg/tutorial/app-codeexamples.html#SEC-TICTACTOE
 *
 * Adaptation for Pioneers: 2004 Roland Clobus
 *
 */

#include "config.h"
#include "game.h"
#include <gtk/gtk.h>
#include <string.h>
#include "guimap.h"
#include "common_gtk.h"

#include "select-game.h"

/* The signals */
enum {
	ACTIVATE,
	LAST_SIGNAL
};

static const gint MAP_WIDTH = 64;
static const gint MAP_HEIGHT = 48;

static void select_game_class_init(SelectGameClass * klass);
static void select_game_init(SelectGame * sg);
static void select_game_finalize(GObject * object);
static void select_game_item_changed(GtkWidget * widget, SelectGame * sg);

/* All signals */
static guint select_game_signals[LAST_SIGNAL] = { 0 };

/* Register the class */
GType select_game_get_type(void)
{
	static GType sg_type = 0;

	if (!sg_type) {
		static const GTypeInfo sg_info = {
			sizeof(SelectGameClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) select_game_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(SelectGame),
			0,
			(GInstanceInitFunc) select_game_init,
			NULL
		};
		sg_type =
		    g_type_register_static(GTK_TYPE_GRID, "SelectGame",
					   &sg_info, 0);
	}
	return sg_type;
}

/* Clean up after the widget is no longer in use */
static void select_game_finalize(GObject * object)
{
	SelectGame *sg = SELECTGAME(object);

	/* sg->combo_box is already handled */

	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(sg->data), &iter);
	do {
		GameParams *params;
		GdkPixbuf *pixbuf;
		gtk_tree_model_get(GTK_TREE_MODEL(sg->data), &iter, 2,
				   &params, 1, &pixbuf, -1);
		g_object_unref(pixbuf);
		params_free(params);
	} while (gtk_list_store_remove(sg->data, &iter));

	gtk_list_store_clear(sg->data);
	g_ptr_array_unref(sg->game_names);
	g_free(sg->default_game);
}

/* Register the signals.
 * SelectGame will emit one signal: 'activate' with the text of the
 *    active game in user_data
 */
static void select_game_class_init(SelectGameClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	select_game_signals[ACTIVATE] = g_signal_new("activate",
						     G_TYPE_FROM_CLASS
						     (klass),
						     G_SIGNAL_RUN_FIRST |
						     G_SIGNAL_ACTION,
						     G_STRUCT_OFFSET
						     (SelectGameClass,
						      activate), NULL,
						     NULL,
						     g_cclosure_marshal_VOID__VOID,
						     G_TYPE_NONE, 0);
	object_class->finalize = select_game_finalize;
}

/* Build the composite widget */
static void select_game_init(SelectGame * sg)
{
	GtkCellRenderer *cell;

	/* Create model */
	sg->data =
	    gtk_list_store_new(3, G_TYPE_STRING, GDK_TYPE_PIXBUF,
			       G_TYPE_POINTER);
	sg->combo_box =
	    gtk_combo_box_new_with_model(GTK_TREE_MODEL(sg->data));

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(sg->combo_box),
				   cell, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(sg->combo_box),
				       cell, "text", 0, NULL);

	cell = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(sg->combo_box),
				   cell, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(sg->combo_box),
				       cell, "pixbuf", 1, NULL);

	sg->game_names = g_ptr_array_new_with_free_func(g_free);

	gtk_widget_show(sg->combo_box);
	gtk_widget_set_tooltip_text(sg->combo_box,
				    /* Tooltip for the list of games */
				    _("Select a game"));
	gtk_grid_attach(GTK_GRID(sg), sg->combo_box, 0, 0, 1, 1);
	sg->default_game = g_strdup("Default");
	g_signal_connect(G_OBJECT(sg->combo_box), "changed",
			 G_CALLBACK(select_game_item_changed), sg);
}

/* Create a new instance of the widget */
GtkWidget *select_game_new(void)
{
	return GTK_WIDGET(g_object_new(select_game_get_type(), NULL));
}

/* Set the default game */
void select_game_set_default(SelectGame * sg, const gchar * game_title)
{
	if (sg->default_game)
		g_free(sg->default_game);
	sg->default_game = g_strdup(game_title);
}

/** Add a game title to the list. The default game will be the active item.
 * @param sg The SelectGame
 * @param game_title Title
 * @param[out] iter The iter for the new entry
 */
static void select_game_add_internal(SelectGame * sg,
				     const gchar * game_title,
				     GtkTreeIter * iter)
{
	g_ptr_array_add(sg->game_names, g_strdup(game_title));
	gtk_list_store_append(sg->data, iter);
	gtk_list_store_set(sg->data, iter, 0, game_title, 1, NULL, 2,
			   NULL, -1);

	if (!strcmp(game_title, sg->default_game)) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(sg->combo_box),
					 sg->game_names->len - 1);
	} else if (sg->game_names->len == 1) {
		/* Activate the first item */
		gtk_combo_box_set_active(GTK_COMBO_BOX(sg->combo_box), 0);
	}
}

/** Add a game title to the list. The default game will be the active item.
 * @param sg The SelectGame
 * @param game_title Title
 */
void select_game_add(SelectGame * sg, const gchar * game_title)
{
	GtkTreeIter iter;

	select_game_add_internal(sg, game_title, &iter);
}

/** Render the map.
 * @param base_widget Base widget
 * @param map The map
 * @return A GdkPixbuf with ref count = 1
 */
static GdkPixbuf *render_map(GtkWidget * base_widget, const Map * map)
{
	GdkPixbuf *pixbuf;
	GuiMap *gmap;

	gmap = guimap_new();
	cairo_rectangle_t extent = { 0.0, 0.0, MAP_WIDTH, MAP_HEIGHT };
	gmap->surface =
	    cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA,
					   &extent);
	gmap->width = MAP_WIDTH;
	gmap->height = MAP_HEIGHT;
	gmap->area = base_widget;
	g_object_ref(gmap->area);
	gmap->map = map_copy(map);
	guimap_scale_to_size(gmap, MAP_WIDTH, MAP_HEIGHT);
	guimap_display(gmap);

	pixbuf =
	    gdk_pixbuf_get_from_surface(gmap->surface, 0, 0, MAP_WIDTH,
					MAP_HEIGHT);
	guimap_delete(gmap);

	return pixbuf;
}

static void select_game_item_changed(G_GNUC_UNUSED GtkWidget * widget,
				     SelectGame * sg)
{
	g_signal_emit(G_OBJECT(sg), select_game_signals[ACTIVATE], 0);
}

/** Locate a title.
 * @param sg The SelectGame
 * @param title The title to locate
 * @param[out] iter The iter (when found), only valid when found
 * @return TRUE if found
 */
static gboolean select_game_locate_title(SelectGame * sg,
					 const gchar * locate_title,
					 GtkTreeIter * iter)
{
	gboolean valid;
	gboolean found;

	valid =
	    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(sg->data), iter);
	found = FALSE;
	while (valid && !found) {
		gchar *title;

		gtk_tree_model_get(GTK_TREE_MODEL(sg->data), iter, 0,
				   &title, -1);

		if (!strcmp(title, locate_title)) {
			found = TRUE;
		} else {
			valid =
			    gtk_tree_model_iter_next(GTK_TREE_MODEL
						     (sg->data), iter);
		}
		g_free(title);
	}
	return found;
}

struct TMapRenderer {
	SelectGame *sg;
	GameParams *params;
};

/** Render a map during the idle loop.
 * @param user_data Information about the map to render
 * @return FALSE to automatically remove the GSource.
 */
static gboolean select_game_render_map(gpointer user_data)
{
	GtkTreeIter iter;
	struct TMapRenderer *map_render_data;
	SelectGame *sg;
	GdkPixbuf *pixbuf;

	map_render_data = user_data;
	sg = map_render_data->sg;
	g_return_val_if_fail(select_game_locate_title
			     (sg, map_render_data->params->title, &iter),
			     FALSE);

	pixbuf = render_map(sg->combo_box, map_render_data->params->map);
	gtk_list_store_set(sg->data, &iter, 1, pixbuf, -1);
	g_object_unref(pixbuf);

	g_free(map_render_data);

	return FALSE;
}

/** Add a detailed entry.
 * @param sg SelectGame
 * @param params The full parameters of the game
 */
void select_game_add_details(SelectGame * sg, const GameParams * params)
{
	GtkTreeIter iter;
	gboolean found;
	struct TMapRenderer *map_render_data;

	found = select_game_locate_title(sg, params->title, &iter);
	if (!found) {
		select_game_add_internal(sg, params->title, &iter);
	}

	map_render_data = g_malloc(sizeof(*map_render_data));
	map_render_data->sg = sg;
	map_render_data->params = params_copy(params);

	if (strcmp(select_game_get_active_title(sg), params->title)) {
		/* Inactive item */
		GdkPixbuf *pixbuf;

		/* Create a placeholder of the right size */
		pixbuf =
		    gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, MAP_WIDTH,
				   MAP_HEIGHT);
		gtk_list_store_set(sg->data, &iter, 1, pixbuf, 2,
				   map_render_data->params, -1);
		g_object_unref(pixbuf);

		/* Render later */
		g_idle_add(select_game_render_map, map_render_data);
	} else {
		/* Send an update when the details are known for
		 * the active item */
		gtk_list_store_set(sg->data, &iter, 2,
				   map_render_data->params, -1);
		select_game_render_map(map_render_data);
		select_game_item_changed(NULL, sg);
	}
}

/** Return the selected title.
 * @param sg SelectGame
 * @return The title, or NULL when nothing is selected
 */
const gchar *select_game_get_active_title(SelectGame * sg)
{
	gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(sg->combo_box));
	return g_ptr_array_index(sg->game_names, idx);
}

/** Return the details of the game.
 * @param sg SelectGame
 * @return The details game description, or NULL when nothing is selected
 */
const GameParams *select_game_get_active_game(SelectGame * sg)
{
	GtkTreeIter iter;
	GameParams *params;

	if (gtk_combo_box_get_active_iter
	    (GTK_COMBO_BOX(sg->combo_box), &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(sg->data), &iter, 2,
				   &params, -1);
		return params;
	} else {
		return NULL;
	}
}
