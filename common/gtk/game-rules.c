#include "config.h"
#include "game.h"
#include <gtk/gtk.h>
#include <string.h>
#include <glib.h>

#include "game-rules.h"
#include "game.h"

static void game_rules_init(GameRules * sg, gboolean show_all_rules);
static void verify_island_discovery_bonus(GtkButton * button,
					  gpointer user_data);

static void dice_deck_toggled_callback(GtkToggleButton * dice_deck,
				       gpointer user_data)
{
	GameRules *gr = user_data;
	gtk_widget_set_sensitive(GTK_WIDGET(gr->num_dice_decks),
				 gtk_toggle_button_get_active(dice_deck));
	gtk_widget_set_sensitive(GTK_WIDGET(gr->num_removed_dice_cards),
				 gtk_toggle_button_get_active(dice_deck));
}

/* Register the class */
GType game_rules_get_type(void)
{
	static GType gp_type = 0;

	if (!gp_type) {
		static const GTypeInfo gp_info = {
			sizeof(GameRulesClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			NULL,	/* class init */
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof(GameRules),
			0,
			NULL,
			NULL
		};
		gp_type =
		    g_type_register_static(GTK_TYPE_GRID,
					   "GameRules", &gp_info, 0);
	}
	return gp_type;
}

static void add_row(GameRules * gr, const gchar * name,
		    const gchar * tooltip, guint row,
		    GtkCheckButton ** check)
{
	GtkWidget *check_btn;

	check_btn = gtk_check_button_new_with_label(name);
	gtk_widget_show(check_btn);
	gtk_grid_attach(GTK_GRID(gr), check_btn, 0, row, 2, 1);
	*check = GTK_CHECK_BUTTON(check_btn);

	gtk_widget_set_tooltip_text(check_btn, tooltip);
}

/* Build the composite widget */
static void game_rules_init(GameRules * gr, gboolean show_all_rules)
{
	GtkWidget *label;
	GtkWidget *vbox_sevens;
	GtkWidget *hbox;
	GtkWidget *widget;
	gint idx;
	guint row;

	gtk_grid_set_row_spacing(GTK_GRID(gr), 3);
	gtk_grid_set_column_spacing(GTK_GRID(gr), 5);

	row = 0;

	/* Label */
	label = gtk_label_new(_("Sevens rule"));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(gr), label, 0, row, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.0);

	gr->radio_sevens[0] = gtk_radio_button_new_with_label(NULL,
							      /* Sevens rule: normal */
							      _("Normal"));
	gr->radio_sevens[1] =
	    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON
							(gr->radio_sevens
							 [0]),
							/* Sevens rule: reroll on 1st 2 turns */
							_(""
							  "Reroll on 1st 2 turns"));
	gr->radio_sevens[2] =
	    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON
							(gr->radio_sevens
							 [0]),
							/* Sevens rule: reroll all 7s */
							_(""
							  "Reroll all 7s"));

	vbox_sevens = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_set_homogeneous(GTK_BOX(vbox_sevens), TRUE);
	gtk_widget_show(vbox_sevens);
	gtk_widget_set_tooltip_text(gr->radio_sevens[0],
				    /* Tooltip for sevens rule normal */
				    _(""
				      "All sevens move the robber or pirate"));
	gtk_widget_set_tooltip_text(gr->radio_sevens[1],
				    /* Tooltip for sevens rule reroll on 1st 2 turns */
				    _(""
				      "In the first two turns all sevens are rerolled"));
	gtk_widget_set_tooltip_text(gr->radio_sevens[2],
				    /* Tooltip for sevens rule reroll all */
				    _("All sevens are rerolled"));

	for (idx = 0; idx < 3; ++idx) {
		gtk_widget_show(gr->radio_sevens[idx]);
		gtk_box_pack_start(GTK_BOX(vbox_sevens),
				   gr->radio_sevens[idx], TRUE, TRUE, 0);
	}
	gtk_grid_attach(GTK_GRID(gr), vbox_sevens, 1, row, 1, 1);
	gtk_widget_set_hexpand(vbox_sevens, TRUE);

	row++;
	add_row(gr, _("Randomize terrain"), _("Randomize the terrain"),
		row++, &gr->random_terrain);
	if (show_all_rules) {
		add_row(gr, _("Use pirate"),
			_("Use the pirate to block ships"), row++,
			&gr->use_pirate);
		add_row(gr, _("Strict trade"),
			_("Allow trade only before building or buying"),
			row++, &gr->strict_trade);
		add_row(gr, _("Domestic trade"),
			_("Allow trade between players"), row++,
			&gr->domestic_trade);
		add_row(gr, _("Victory at end of turn"),
			_("Check for victory only at end of turn"), row++,
			&gr->check_victory_at_end_of_turn);

		add_row(gr, _("Use dice deck"),
			_
			("Use a deck of 36 dice cards instead of real dice"),
			row++, &gr->use_dice_deck);
		g_signal_connect(G_OBJECT(gr->use_dice_deck), "toggled",
				 G_CALLBACK(dice_deck_toggled_callback),
				 gr);


		/* Label */
		label = gtk_label_new(_("Number of dice decks"));
		gtk_widget_show(label);
		gtk_grid_attach(GTK_GRID(gr), label, 0, row, 1, 1);
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		gr->num_dice_decks =
		    GTK_SPIN_BUTTON(gtk_spin_button_new_with_range
				    (1, 5, 1));
		gtk_entry_set_alignment(GTK_ENTRY(gr->num_dice_decks),
					1.0);
		gtk_widget_show(GTK_WIDGET(gr->num_dice_decks));
		gtk_widget_set_sensitive(GTK_WIDGET(gr->num_dice_decks),
					 gtk_toggle_button_get_active
					 (GTK_TOGGLE_BUTTON
					  (gr->use_dice_deck)));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gr->num_dice_decks),
					    _
					    ("The number of dice decks (of 36 cards each)"));
		gtk_grid_attach(GTK_GRID(gr),
				GTK_WIDGET(gr->num_dice_decks), 1, row, 1,
				1);

		row++;
		/* Label */
		label = gtk_label_new(_("Number of removed dice cards"));
		gtk_widget_show(label);
		gtk_grid_attach(GTK_GRID(gr), label, 0, row, 1, 1);
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		gr->num_removed_dice_cards =
		    GTK_SPIN_BUTTON(gtk_spin_button_new_with_range
				    (0, 30, 1));
		gtk_entry_set_alignment(GTK_ENTRY
					(gr->num_removed_dice_cards), 1.0);
		gtk_widget_show(GTK_WIDGET(gr->num_removed_dice_cards));
		gtk_widget_set_sensitive(GTK_WIDGET
					 (gr->num_removed_dice_cards),
					 gtk_toggle_button_get_active
					 (GTK_TOGGLE_BUTTON
					  (gr->use_dice_deck)));
		gtk_widget_set_tooltip_text(GTK_WIDGET
					    (gr->num_removed_dice_cards),
					    _
					    ("The number of dice cards that are removed after shuffling the deck"));
		gtk_grid_attach(GTK_GRID(gr),
				GTK_WIDGET(gr->num_removed_dice_cards), 1,
				row, 1, 1);

		row++;
		/* Label */
		label = gtk_label_new(_("Island discovery bonuses"));
		gtk_widget_show(label);
		gtk_grid_attach(GTK_GRID(gr), label, 0, row, 1, 1);
		gtk_label_set_xalign(GTK_LABEL(label), 0.0);

		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);

		gr->island_bonus = GTK_ENTRY(gtk_entry_new());
		gtk_widget_show(GTK_WIDGET(gr->island_bonus));
		gtk_widget_set_tooltip_text(GTK_WIDGET(gr->island_bonus),
					    /* Tooltip for island bonus */
					    _(""
					      "A comma separated list of "
					      "bonus points for discovering "
					      "islands"));
		gtk_box_pack_start(GTK_BOX(hbox),
				   GTK_WIDGET(gr->island_bonus), TRUE,
				   TRUE, 0);

		widget = gtk_button_new();
		gtk_button_set_image(GTK_BUTTON(widget),
				     gtk_image_new_from_icon_name
				     ("pioneers-checkmark",
				      GTK_ICON_SIZE_BUTTON));
		g_signal_connect(G_OBJECT(widget), "clicked",
				 G_CALLBACK(verify_island_discovery_bonus),
				 (gpointer) gr);
		gtk_widget_set_tooltip_text(widget,
					    /* Tooltip for the check button */
					    _("Check and correct island "
					      "discovery bonuses"));
		gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, FALSE, 0);

		gtk_grid_attach(GTK_GRID(gr), GTK_WIDGET(hbox), 1, row, 1,
				1);
		row++;
	} else {
		gr->use_pirate = NULL;
		gr->strict_trade = NULL;
		gr->domestic_trade = NULL;
		gr->check_victory_at_end_of_turn = NULL;
		gr->island_bonus = NULL;
	}
}

/* Create a new instance of the widget */
GtkWidget *game_rules_new(void)
{
	GameRules *widget =
	    GAMERULES(g_object_new(game_rules_get_type(), NULL));
	game_rules_init(widget, TRUE);
	return GTK_WIDGET(widget);
}


/* Create a new instance with only the changes that can be applied by the metaserver */
GtkWidget *game_rules_new_metaserver(void)
{
	GameRules *widget =
	    GAMERULES(g_object_new(game_rules_get_type(), NULL));
	game_rules_init(widget, FALSE);
	return GTK_WIDGET(widget);
}

void game_rules_set_random_terrain(GameRules * gr, gboolean val)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gr->random_terrain),
				     val);
}

gboolean game_rules_get_random_terrain(GameRules * gr)
{
	return
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (gr->random_terrain));
}

/* Set the sevens rule
 * 0 = Normal
 * 1 = Reroll first two turns
 * 2 = Reroll all
 */
void game_rules_set_sevens_rule(GameRules * gr, guint sevens_rule)
{
	g_return_if_fail(sevens_rule < G_N_ELEMENTS(gr->radio_sevens));

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				     (gr->radio_sevens[sevens_rule]),
				     TRUE);

}

/* Get the sevens rule */
guint game_rules_get_sevens_rule(GameRules * gr)
{
	guint idx;

	for (idx = 0; idx < G_N_ELEMENTS(gr->radio_sevens); idx++)
		if (gtk_toggle_button_get_active
		    (GTK_TOGGLE_BUTTON(gr->radio_sevens[idx])))
			return idx;
	return 0;
}

void game_rules_set_use_dice_deck(GameRules * gr, gboolean val)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gr->use_dice_deck),
				     val);
}

gboolean game_rules_get_use_dice_deck(GameRules * gr)
{
	return
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (gr->use_dice_deck));
}

void game_rules_set_num_dice_decks(GameRules * gr, guint val)
{
	gtk_spin_button_set_value(gr->num_dice_decks, val);
}

guint game_rules_get_num_dice_decks(GameRules * gr)
{
	return gtk_spin_button_get_value_as_int(gr->num_dice_decks);
}

void game_rules_set_num_removed_dice_cards(GameRules * gr, guint val)
{
	gtk_spin_button_set_value(gr->num_removed_dice_cards, val);
}

guint game_rules_get_num_removed_dice_cards(GameRules * gr)
{
	return
	    gtk_spin_button_get_value_as_int(gr->num_removed_dice_cards);
}

void game_rules_set_use_pirate(GameRules * gr, gboolean val,
			       gint num_ships)
{
	if (num_ships == 0) {
		gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON
						   (gr->use_pirate), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gr->use_pirate),
					 FALSE);
	} else {
		gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON
						   (gr->use_pirate),
						   FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
					     (gr->use_pirate), val);
		gtk_widget_set_sensitive(GTK_WIDGET(gr->use_pirate), TRUE);
	}
}

gboolean game_rules_get_use_pirate(GameRules * gr)
{
	return
	    gtk_toggle_button_get_inconsistent(GTK_TOGGLE_BUTTON
					       (gr->use_pirate)) ? FALSE :
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (gr->use_pirate));
}

void game_rules_set_strict_trade(GameRules * gr, gboolean val)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gr->strict_trade),
				     val);
}

gboolean game_rules_get_strict_trade(GameRules * gr)
{
	return
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (gr->strict_trade));
}

void game_rules_set_domestic_trade(GameRules * gr, gboolean val)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gr->domestic_trade),
				     val);
}

gboolean game_rules_get_domestic_trade(GameRules * gr)
{
	return
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (gr->domestic_trade));
}

void game_rules_set_victory_at_end_of_turn(GameRules * gr, gboolean val)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				     (gr->check_victory_at_end_of_turn),
				     val);
}

gboolean game_rules_get_victory_at_end_of_turn(GameRules * gr)
{
	return
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (gr->
					  check_victory_at_end_of_turn));
}

void game_rules_set_island_discovery_bonus(GameRules * gr, GArray * val)
{
	gchar *text;

	text = format_int_list(NULL, val);
	if (text != NULL) {
		gtk_entry_set_text(gr->island_bonus, text);
	} else {
		gtk_entry_set_text(gr->island_bonus, "");
	}
	g_free(text);
}

GArray *game_rules_get_island_discovery_bonus(GameRules * gr)
{
	return build_int_list(gtk_entry_get_text(gr->island_bonus));
}

static void verify_island_discovery_bonus(G_GNUC_UNUSED GtkButton * button,
					  gpointer user_data)
{
	GameRules *gr = GAMERULES(user_data);
	GArray *bonuses;

	bonuses = game_rules_get_island_discovery_bonus(gr);
	game_rules_set_island_discovery_bonus(gr, bonuses);
	if (bonuses != NULL)
		g_array_free(bonuses, TRUE);
}
