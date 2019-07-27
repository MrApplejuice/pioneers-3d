/* A custom widget for selecting a game from a list of games.
 *
 * The code is based on the TICTACTOE example
 * www.gtk.oorg/tutorial/app-codeexamples.html#SEC-TICTACTOE
 *
 * Adaptation for Pioneers: 2004 Roland Clobus
 *
 */
#ifndef __SELECTGAME_H__
#define __SELECTGAME_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#define SELECTGAME_TYPE            (select_game_get_type ())
#define SELECTGAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SELECTGAME_TYPE, SelectGame))
#define SELECTGAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SELECTGAME_TYPE, SelectGameClass))
#define IS_SELECTGAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SELECTGAME_TYPE))
#define IS_SELECTGAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SELECTGAME_TYPE))
typedef struct _SelectGame SelectGame;
typedef struct _SelectGameClass SelectGameClass;

struct _SelectGame {
	GtkGrid grid;

	GtkWidget *combo_box;
	GtkListStore *data;
	GPtrArray *game_names;
	gchar *default_game;
};

struct _SelectGameClass {
	GtkGridClass parent_class;

	void (*activate) (SelectGame * sg);
};

GType select_game_get_type(void);
GtkWidget *select_game_new(void);
void select_game_set_default(SelectGame * sg, const gchar * game_title);
void select_game_add(SelectGame * sg, const gchar * game_title);
void select_game_add_details(SelectGame * sg, const GameParams * params);
const gchar *select_game_get_active_title(SelectGame * sg);
const GameParams *select_game_get_active_game(SelectGame * sg);

G_END_DECLS
#endif				/* __SELECTGAME_H__ */
