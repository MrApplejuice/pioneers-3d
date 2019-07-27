#ifndef __GAMERULES_H__
#define __GAMERULES_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#define GAMERULES_TYPE            (game_rules_get_type ())
#define GAMERULES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMERULES_TYPE, GameRules))
#define GAMERULES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMERULES_TYPE, GameRulesClass))
#define IS_GAMERULES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMERULES_TYPE))
#define IS_GAMERULES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMERULES_TYPE))
typedef struct _GameRules GameRules;
typedef struct _GameRulesClass GameRulesClass;

struct _GameRules {
	GtkGrid grid;

	GtkCheckButton *random_terrain;
	GtkWidget *radio_sevens[3];	/* radio buttons for sevens rules */
	GtkCheckButton *use_dice_deck;
	GtkSpinButton *num_dice_decks;
	GtkSpinButton *num_removed_dice_cards;
	GtkCheckButton *use_pirate;
	GtkCheckButton *strict_trade;
	GtkCheckButton *domestic_trade;
	GtkCheckButton *check_victory_at_end_of_turn;
	GtkEntry *island_bonus;
};

struct _GameRulesClass {
	GtkGridClass parent_class;
};

GType game_rules_get_type(void);
GtkWidget *game_rules_new(void);
GtkWidget *game_rules_new_metaserver(void);

void game_rules_set_random_terrain(GameRules * gr, gboolean val);
gboolean game_rules_get_random_terrain(GameRules * gr);
void game_rules_set_sevens_rule(GameRules * gr, guint sevens_rule);
guint game_rules_get_sevens_rule(GameRules * gr);
void game_rules_set_use_dice_deck(GameRules * gr, gboolean val);
gboolean game_rules_get_use_dice_deck(GameRules * gr);
void game_rules_set_num_dice_decks(GameRules * gr, guint val);
guint game_rules_get_num_dice_decks(GameRules * gr);
void game_rules_set_num_removed_dice_cards(GameRules * gr, guint val);
guint game_rules_get_num_removed_dice_cards(GameRules * gr);
void game_rules_set_use_pirate(GameRules * gr, gboolean val,
			       gint num_ships);
gboolean game_rules_get_use_pirate(GameRules * gr);
void game_rules_set_strict_trade(GameRules * gr, gboolean val);
gboolean game_rules_get_strict_trade(GameRules * gr);
void game_rules_set_domestic_trade(GameRules * gr, gboolean val);
gboolean game_rules_get_domestic_trade(GameRules * gr);
void game_rules_set_victory_at_end_of_turn(GameRules * gr, gboolean val);
gboolean game_rules_get_victory_at_end_of_turn(GameRules * gr);
void game_rules_set_island_discovery_bonus(GameRules * gr, GArray * val);
GArray *game_rules_get_island_discovery_bonus(GameRules * gr);

G_END_DECLS
#endif				/* __GAMERULES_H__ */
