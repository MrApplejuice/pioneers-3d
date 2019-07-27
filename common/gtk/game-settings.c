/* A custom widget for adjusting the game settings.
 *
 * The code is based on the TICTACTOE example
 * www.gtk.org/tutorial/app-codeexamples.html#SEC-TICTACTOE
 *
 * Adaptation for Pioneers: 2004 Roland Clobus
 *
 */

#include "config.h"
#include <gtk/gtk.h>
#include <glib.h>

#include "game-settings.h"
#include "game.h"

/* The signals */
enum {
	CHANGE,
	CHANGE_PLAYERS,
	CHECK,
	LAST_SIGNAL
};

static void game_settings_class_init(GameSettingsClass * klass);
static void game_settings_init(GameSettings * sg);
static void game_settings_change_players(GtkSpinButton * widget,
					 GameSettings * gs);
static void game_settings_change_victory_points(GtkSpinButton * widget,
						GameSettings * gs);
static void game_settings_check(GtkButton * widget, GameSettings * gs);
static void game_settings_update(GameSettings * gs);

/* All signals */
static guint game_settings_signals[LAST_SIGNAL] = { 0, 0 };

/* Register the class */
GType game_settings_get_type(void)
{
	static GType gs_type = 0;

	if (!gs_type) {
		static const GTypeInfo gs_info = {
			sizeof(GameSettingsClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) game_settings_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(GameSettings),
			0,
			(GInstanceInitFunc) game_settings_init,
			NULL
		};
		gs_type =
		    g_type_register_static(GTK_TYPE_GRID, "GameSettings",
					   &gs_info, 0);
	}
	return gs_type;
}

/* Register the signals.
 * GameSettings will emit two signals:
 * 'change'         when any change to one of the controls occurs.
 * 'change-players' when the amount of player has changed.
 */
static void game_settings_class_init(GameSettingsClass * klass)
{
	game_settings_signals[CHANGE] = g_signal_new("change",
						     G_TYPE_FROM_CLASS
						     (klass),
						     G_SIGNAL_RUN_FIRST |
						     G_SIGNAL_ACTION,
						     G_STRUCT_OFFSET
						     (GameSettingsClass,
						      change), NULL, NULL,
						     g_cclosure_marshal_VOID__VOID,
						     G_TYPE_NONE, 0);
	game_settings_signals[CHANGE_PLAYERS] =
	    g_signal_new("change-players", G_TYPE_FROM_CLASS(klass),
			 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			 G_STRUCT_OFFSET(GameSettingsClass,
					 change_players), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	game_settings_signals[CHECK] =
	    g_signal_new("check", G_TYPE_FROM_CLASS(klass),
			 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			 G_STRUCT_OFFSET(GameSettingsClass, check), NULL,
			 NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE,
			 0);
}

/* Build the composite widget */
static void game_settings_init(GameSettings * gs)
{
	GtkWidget *label;
	GtkWidget *hbox;
	GtkAdjustment *adj;

	gtk_grid_set_row_spacing(GTK_GRID(gs), 3);
	gtk_grid_set_column_spacing(GTK_GRID(gs), 5);
	gtk_widget_set_hexpand(GTK_WIDGET(gs), FALSE);

	/* Label text for customising a game */
	label = gtk_label_new(_("Number of players"));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(gs), label, 0, 1, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);

	adj =
	    GTK_ADJUSTMENT(gtk_adjustment_new(0, 2, MAX_PLAYERS, 1, 4, 0));
	gs->players_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_entry_set_alignment(GTK_ENTRY(gs->players_spin), 1.0);
	gtk_widget_show(gs->players_spin);
	gtk_widget_set_hexpand(gs->players_spin, TRUE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(gs->players_spin),
				    TRUE);
	gtk_grid_attach(GTK_GRID(gs), gs->players_spin, 1, 1, 1, 1);
	g_signal_connect(G_OBJECT(gs->players_spin), "value-changed",
			 G_CALLBACK(game_settings_change_players), gs);
	gtk_widget_set_tooltip_text(gs->players_spin,
				    /* Tooltip for 'Number of Players' */
				    _("The number of players"));

	/* Label for customising a game */
	label = gtk_label_new(_("Victory point target"));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(gs), label, 0, 2, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);

	adj = GTK_ADJUSTMENT(gtk_adjustment_new(10, 3, 99, 1, 5, 0));
	gs->victory_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_entry_set_alignment(GTK_ENTRY(gs->victory_spin), 1.0);
	gtk_widget_show(gs->victory_spin);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(gs->victory_spin),
				    TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), gs->victory_spin, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(gs->victory_spin), "value-changed",
			 G_CALLBACK(game_settings_change_victory_points),
			 gs);
	gtk_widget_set_tooltip_text(gs->victory_spin,
				    /* Tooltip for Victory Point Target */
				    _(""
				      "The points needed to win the game"));

	gs->check_button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(gs->check_button),
			     gtk_image_new_from_icon_name
			     ("pioneers-checkmark", GTK_ICON_SIZE_BUTTON));
	gtk_widget_show(gs->check_button);
	gtk_box_pack_start(GTK_BOX(hbox), gs->check_button, FALSE, FALSE,
			   0);
	g_signal_connect(G_OBJECT(gs->check_button), "clicked",
			 G_CALLBACK(game_settings_check), gs);
	gtk_widget_set_tooltip_text(gs->check_button,
				    /* Tooltip for the check button */
				    _("Is it possible to win this game?"));

	gtk_grid_attach(GTK_GRID(gs), hbox, 1, 2, 1, 1);
	gtk_widget_show(hbox);

	gs->players = 4;
	gs->victory_points = 10;
	game_settings_update(gs);
}

/* Create a new instance of the widget */
GtkWidget *game_settings_new(gboolean with_check_button)
{
	GtkWidget *widget =
	    GTK_WIDGET(g_object_new(game_settings_get_type(), NULL));
	if (with_check_button)
		gtk_widget_show(GAMESETTINGS(widget)->check_button);
	else
		gtk_widget_hide(GAMESETTINGS(widget)->check_button);
	return widget;
}

/* Emits 'change-players' when the number of players has changed */
static void game_settings_change_players(GtkSpinButton * widget,
					 GameSettings * gs)
{
	gs->players = (guint) gtk_spin_button_get_value_as_int(widget);
	game_settings_update(gs);
	g_signal_emit(G_OBJECT(gs), game_settings_signals[CHANGE_PLAYERS],
		      0);
	g_signal_emit(G_OBJECT(gs), game_settings_signals[CHANGE], 0);
}

/* Callback when the points needed to win have changed */
static void game_settings_change_victory_points(GtkSpinButton * widget,
						GameSettings * gs)
{
	gs->victory_points =
	    (guint) gtk_spin_button_get_value_as_int(widget);
	game_settings_update(gs);
	g_signal_emit(G_OBJECT(gs), game_settings_signals[CHANGE], 0);
}

/* Set the number of players */
void game_settings_set_players(GameSettings * gs, guint players)
{
	gs->players = players;
	game_settings_update(gs);
}

/* Get the number of players */
guint game_settings_get_players(GameSettings * gs)
{
	return gs->players;
}

/* Set the points needed to win */
void game_settings_set_victory_points(GameSettings * gs,
				      guint victory_points)
{
	gs->victory_points = victory_points;
	game_settings_update(gs);
}

/* Get the points needed to win */
guint game_settings_get_victory_points(GameSettings * gs)
{
	return gs->victory_points;
}

static void game_settings_check(G_GNUC_UNUSED GtkButton * widget,
				GameSettings * gs)
{
	g_signal_emit(G_OBJECT(gs), game_settings_signals[CHECK], 0);
}

/* Update the display to the current state */
static void game_settings_update(GameSettings * gs)
{
	/* Disable signals, to avoid recursive updates */
	g_signal_handlers_block_matched(G_OBJECT(gs->players_spin),
					G_SIGNAL_MATCH_DATA,
					0, 0, NULL, NULL, gs);
	g_signal_handlers_block_matched(G_OBJECT(gs->victory_spin),
					G_SIGNAL_MATCH_DATA,
					0, 0, NULL, NULL, gs);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gs->players_spin),
				  gs->players);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gs->victory_spin),
				  gs->victory_points);

	/* Reenable the signals */
	g_signal_handlers_unblock_matched(G_OBJECT(gs->players_spin),
					  G_SIGNAL_MATCH_DATA,
					  0, 0, NULL, NULL, gs);
	g_signal_handlers_unblock_matched(G_OBJECT(gs->victory_spin),
					  G_SIGNAL_MATCH_DATA,
					  0, 0, NULL, NULL, gs);
}
