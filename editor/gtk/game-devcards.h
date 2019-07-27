#ifndef __GAMEDEVCARDS_H__
#define __GAMEDEVCARDS_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "cards.h"

G_BEGIN_DECLS
#define GAMEDEVCARDS_TYPE            (game_devcards_get_type ())
#define GAMEDEVCARDS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMEDEVCARDS_TYPE, GameDevCards))
#define GAMEDEVCARDS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMEDEVCARDS_TYPE, GameDevCardsClass))
#define IS_GAMEDEVCARDS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMEDEVCARDS_TYPE))
#define IS_GAMEDEVCARDS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMEDEVCARDS_TYPE))
typedef struct _GameDevCards GameDevCards;
typedef struct _GameDevCardsClass GameDevCardsClass;

struct _GameDevCards {
	GtkGrid grid;

	GtkSpinButton *num_cards[NUM_DEVEL_TYPES];
};

struct _GameDevCardsClass {
	GtkGridClass parent_class;
};

GType game_devcards_get_type(void);
GtkWidget *game_devcards_new(void);

void game_devcards_set_num_cards(GameDevCards * gd, DevelType type,
				 gint num);
gint game_devcards_get_num_cards(GameDevCards * gd, DevelType type);

G_END_DECLS
#endif				/* __GAMEDEVCARDS_H__ */
