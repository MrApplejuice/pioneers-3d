#ifndef __GAMERESOURCES_H__
#define __GAMERESOURCES_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#define GAMERESOURCES_TYPE            (game_resources_get_type ())
#define GAMERESOURCES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMERESOURCES_TYPE, GameResources))
#define GAMERESOURCES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMERESOURCES_TYPE, GameResourcesClass))
#define IS_GAMERESOURCES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMERESOURCES_TYPE))
#define IS_GAMERESOURCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMERESOURCES_TYPE))
typedef struct _GameResources GameResources;
typedef struct _GameResourcesClass GameResourcesClass;

struct _GameResources {
	GtkGrid grid;

	GtkSpinButton *num_resources;
};

struct _GameResourcesClass {
	GtkGridClass parent_class;
};

GType game_resources_get_type(void);
GtkWidget *game_resources_new(void);

void game_resources_set_num_resources(GameResources * gr, gint num);
gint game_resources_get_num_resources(GameResources * gr);

G_END_DECLS
#endif				/* __GAMERESOURCES_H__ */
