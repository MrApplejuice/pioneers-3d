/* A custom widget for adjusting the game settings.
 *
 * The code is based on the TICTACTOE example
 * www.gtk.oorg/tutorial/app-codeexamples.html#SEC-TICTACTOE
 *
 * Adaptation for Pioneers: 2004 Roland Clobus
 *
 */
#ifndef __GAMESETTINGS_H__
#define __GAMESETTINGS_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#define GAMESETTINGS_TYPE            (game_settings_get_type ())
#define GAMESETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMESETTINGS_TYPE, GameSettings))
#define GAMESETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMESETTINGS_TYPE, GameSettingsClass))
#define IS_GAMESETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMESETTINGS_TYPE))
#define IS_GAMESETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMESETTINGS_TYPE))
typedef struct _GameSettings GameSettings;
typedef struct _GameSettingsClass GameSettingsClass;

struct _GameSettings {
	GtkGrid grid;

	GtkWidget *victory_spin;	/* victory point target */
	GtkWidget *players_spin;	/* number of players */
	GtkWidget *check_button;	/* check whether the game can be won */

	guint players;		/* The number of players */
	guint victory_points;	/* The points needed to win */
};

struct _GameSettingsClass {
	GtkGridClass parent_class;

	void (*change) (GameSettings * gs);
	void (*change_players) (GameSettings * gs);
	void (*check) (GameSettings * gs);
};

GType game_settings_get_type(void);
GtkWidget *game_settings_new(gboolean with_check_button);

void game_settings_set_players(GameSettings * gs, guint players);
guint game_settings_get_players(GameSettings * gs);
void game_settings_set_victory_points(GameSettings * gs,
				      guint victory_points);
guint game_settings_get_victory_points(GameSettings * gs);

G_END_DECLS
#endif				/* __GAMESETTINGS_H__ */
