#ifndef __GAMEBUILDINGS_H__
#define __GAMEBUILDINGS_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#define GAMEBUILDINGS_TYPE            (game_buildings_get_type ())
#define GAMEBUILDINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMEBUILDINGS_TYPE, GameBuildings))
#define GAMEBUILDINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMEBUILDINGS_TYPE, GameBuildingsClass))
#define IS_GAMEBUILDINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMEBUILDINGS_TYPE))
#define IS_GAMEBUILDINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMEBUILDINGS_TYPE))
typedef struct _GameBuildings GameBuildings;
typedef struct _GameBuildingsClass GameBuildingsClass;

struct _GameBuildings {
	GtkGrid grid;

	GtkSpinButton *num_buildings[NUM_BUILD_TYPES];
};

struct _GameBuildingsClass {
	GtkGridClass parent_class;
};

GType game_buildings_get_type(void);
GtkWidget *game_buildings_new(void);

void game_buildings_set_num_buildings(GameBuildings * gb, gint type,
				      gint num);
gint game_buildings_get_num_buildings(GameBuildings * gb, gint type);

G_END_DECLS
#endif				/* __GAMEBUILDINGS_H__ */
