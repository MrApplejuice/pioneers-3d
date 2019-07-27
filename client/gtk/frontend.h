/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2003,2006 Bas Wijnen <shevek@fmf.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _frontend_h
#define _frontend_h

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "callback.h"
#include "colors.h"
#include "quoteinfo.h"

/* All graphics events. */
typedef enum {
	GUI_UPDATE,
	GUI_CONNECT,
	GUI_CONNECT_TRY,
	GUI_CONNECT_CANCEL,
	GUI_DISCONNECT,
	GUI_CHANGE_NAME,
	GUI_QUIT,
	GUI_ROLL,
	GUI_TRADE,
	GUI_UNDO,
	GUI_FINISH,
	GUI_ROAD,
	GUI_SHIP,
	GUI_MOVE_SHIP,
	GUI_BRIDGE,
	GUI_SETTLEMENT,
	GUI_CITY,
	GUI_CITY_WALL,
	GUI_BUY_DEVELOP,
	GUI_PLAY_DEVELOP,
	GUI_MONOPOLY,
	GUI_PLENTY,
	GUI_DISCARD,
	GUI_CHOOSE_GOLD,
	GUI_TRADE_CALL,
	GUI_TRADE_ACCEPT,
	GUI_TRADE_FINISH,
	GUI_QUOTE_SUBMIT,
	GUI_QUOTE_DELETE,
	GUI_QUOTE_REJECT
} GuiEvent;

#include "gui.h"

/* Information about a GUI component */
typedef struct {
	GtkWidget *widget;	/* the GTK widget */
	GuiEvent id;		/* widget id */
	gboolean destroy_only;	/* react to destroy signal */
	const gchar *signal;	/* signal attached */
	gboolean current;	/* is widget currently sensitive? */
	gboolean next;		/* should widget be sensitive? */
} GuiWidgetState;

/** all widgets are inactive while waiting for network. */
extern gboolean frontend_waiting_for_network;

/** set all widgets to their programmed state. */
void frontend_gui_update(void);

/** program the state of a widget for when frontend_gui_update is called. */
void frontend_gui_check(GuiEvent event, gboolean sensitive);

/** are the widgets sensitive?
 * @param event The GuiEvent.
 * @return TRUE when the GuiEvent has widgets associated that are sensitive.
 */
gboolean frontend_gui_get_sensitive(GuiEvent event);

/** initialise the frontend_gui_register_* functions */
void frontend_gui_register_init(void);

/** register a new destroy-only widget. */
void frontend_gui_register_destroy(GtkWidget * widget, GuiEvent id);

/** register an action. */
void frontend_gui_register_action(GtkAction * action, GuiEvent id);

/** register a new "normal" widget. */
void frontend_gui_register(GtkWidget * widget, GuiEvent id,
			   const gchar * signal);

/** route an event to the gui event function */
void frontend_gui_route_event(GuiEvent event);

/* callbacks */
void frontend_init_gtk_et_al(int argc, char **argv);
void frontend_init(void);
void frontend_init_game(void);
void frontend_new_statistics(gint player_num, StatisticType type,
			     gint num);
void frontend_new_points(gint player_num, Points * points, gboolean added);
void frontend_spectator_name(gint spectator_num, const gchar * name);
void frontend_player_name(gint player_num, const gchar * name);
void frontend_player_style(gint player_num, const gchar * style);
void frontend_player_quit(gint player_num);
void frontend_spectator_quit(gint player_num);
void frontend_disconnect(void);
void frontend_offline(void);
void frontend_discard(void);
void frontend_discard_add(gint player_num, gint discard_num);
void frontend_discard_remove(gint player_num);
void frontend_discard_done(void);
void frontend_gold(void);
void frontend_gold_add(gint player_num, gint gold_num);
void frontend_gold_remove(gint player_num, gint * resources);
void frontend_gold_choose(gint gold_num, const gint * bank);
void frontend_gold_done(void);
void frontend_setup(gint num_settlements, gint num_roads);
void frontend_quote(gint player_num, gint * they_supply,
		    gint * they_receive);
void frontend_roadbuilding(gint num_roads);
void frontend_monopoly(void);
void frontend_plenty(const gint * bank);
void frontend_turn(void);
void frontend_trade_player_end(gint player_num);
void frontend_trade_add_quote(gint player_num, gint quote_num,
			      const gint * they_supply,
			      const gint * they_receive);
void frontend_trade_remove_quote(int player_num, int quote_num);
void frontend_quote_player_end(gint player_num);
void frontend_quote_add(gint player_num, gint quote_num,
			const gint * they_supply,
			const gint * they_receive);
void frontend_quote_remove(gint player_num, gint quote_num);
void frontend_quote_start(void);
void frontend_quote_end(void);
void frontend_quote_monitor(void);
void frontend_rolled_dice(gint die1, gint die2, gint player_num);
void frontend_bought_develop(DevelType type);
void frontend_played_develop(gint player_num, guint card_idx,
			     DevelType type);
void frontend_resource_change(Resource type, gint new_amount);
void frontend_robber(void);
void frontend_steal_building(void);
void frontend_steal_ship(void);
void frontend_robber_done(void);
void frontend_game_over(gint player, gint points);
Map *frontend_get_map(void);
void frontend_set_map(Map * map);
void frontend_quit(void);

/* connect.c */
const gchar *connect_get_server(void);
const gchar *connect_get_port(void);
gboolean connect_get_spectator(void);
void connect_set_server(const gchar * server);
void connect_set_port(const gchar * port);
void connect_set_spectator(gboolean spectator);
void connect_set_metaserver(const gchar * metaserver);
void connect_create_dlg(void);

/* trade.c */
GtkWidget *trade_build_page(void);
gboolean can_call_for_quotes(void);
gboolean trade_valid_selection(void);
const gint *trade_we_supply(void);
const gint *trade_we_receive(void);
const QuoteInfo *trade_current_quote(void);
void trade_finish(void);
void trade_add_quote(int player_num, int quote_num,
		     const gint * they_supply, const gint * they_receive);
void trade_delete_quote(int player_num, int quote_num);
void trade_player_finish(gint player_num);
void trade_begin(void);
void trade_format_quote(const QuoteInfo * quote, gchar * buffer);
void trade_new_trade(void);
void trade_perform_maritime(gint ratio, Resource supply, Resource receive);
void trade_perform_domestic(gint player_num, gint partner_num,
			    gint quote_num, const gint * they_supply,
			    const gint * they_receive);
void frontend_trade_domestic(gint partner_num, gint quote_num,
			     const gint * we_supply,
			     const gint * we_receive);
void frontend_trade_maritime(gint ratio, Resource we_supply,
			     Resource we_receive);

/* quote.c */
GtkWidget *quote_build_page(void);
gboolean can_submit_quote(void);
gboolean can_delete_quote(void);
gboolean can_reject_quote(void);
gint quote_next_num(void);
const gint *quote_we_supply(void);
const gint *quote_we_receive(void);
const QuoteInfo *quote_current_quote(void);
void quote_begin_again(gint player_num, const gint * they_supply,
		       const gint * they_receive);
void quote_begin(gint player_num, const gint * they_supply,
		 const gint * they_receive);
void quote_add_quote(gint player_num, gint quote_num,
		     const gint * they_supply, const gint * they_receive);
void quote_delete_quote(gint player_num, gint quote_num);
void quote_player_finish(gint player_num);
void quote_finish(void);
void frontend_quote_trade(gint player_num, gint partner_num,
			  gint quote_num, const gint * they_supply,
			  const gint * they_receive);

/* legend.c */
GtkWidget *legend_create_dlg(void);
GtkWidget *legend_create_content(void);

/* gui_develop.c */
GtkWidget *develop_build_page(void);
gint develop_current_idx(void);
void develop_reset(void);

/* discard.c */
GtkWidget *discard_build_page(void);
gboolean can_discard(void);
void discard_get_list(gint * discards);
void discard_begin(void);
void discard_player_must(gint player_num, gint discard_num);
void discard_player_did(gint player_num);
void discard_end(void);

/* gold.c */
GtkWidget *gold_build_page(void);
gboolean can_choose_gold(void);
void choose_gold_get_list(gint * choice);
void gold_choose_begin(void);
void gold_choose_player_prepare(gint player_num, gint gold_num);
void gold_choose_player_must(gint gold_num, const gint * bank);
void gold_choose_player_did(gint player_num, gint * resource_list);
void gold_choose_end(void);

/* identity.c */
GtkWidget *identity_build_panel(void);
void identity_draw(void);
void identity_set_dice(gint die1, gint die2);
void identity_reset(void);

/* resource.c */
GtkWidget *resource_build_panel(void);

/* player.c */
GtkWidget *player_build_summary(void);
GtkWidget *player_build_turn_area(void);
void player_clear_summary(void);
void player_init(void);
/** The colour of the player, or spectator */
GdkRGBA *player_or_spectator_color(gint player_num);
/** The colour of the player */
GdkRGBA *player_color(gint player_num);
/** Create an icon of the player, for player_num, who is connected.
 *  You should unref the pixbuf when it is no longer needed
 */
GdkPixbuf *player_create_icon(gint player_num, gboolean connected);
void player_show_current(gint player_num);
void set_num_players(gint num);

/* chat.c */
/** Create the chat widget */
GtkWidget *chat_build_panel(void);
/** Determine if the focus should be moved to the chat widget */
void chat_set_grab_focus_on_update(gboolean grab);
/** Set the focus to the chat widget */
void chat_set_focus(void);
/** A player/spectator has changed his name */
void chat_player_name(gint player_num, const gchar * name);
/** A player/spectator has changed his style */
void chat_player_style(gint player_num);
/** A player has quit */
void chat_player_quit(gint player_num);
/** A spectator has quit */
void chat_spectator_quit(gint spectator_num);
/** Clear all names */
void chat_clear_names(void);
/** Parse the chat for commands */
void chat_parser(gint player_num, const gchar * chat_str);

/* name.c */
/** Create a dialog to change the name */
void name_create_dlg(void);

/* settingscreen.c */
void settings_init(void);
GtkWidget *settings_create_dlg(void);

/* monopoly.c */
Resource monopoly_type(void);
void monopoly_destroy_dlg(void);
void monopoly_create_dlg(void);

/* plenty.c */
void plenty_resources(gint * plenty);
void plenty_destroy_dlg(void);
void plenty_create_dlg(const gint * bank);
gboolean plenty_can_activate(void);

/* gameover.c */
GtkWidget *gameover_create_dlg(gint player_num, gint num_points);

#define PIONEERS_PIXMAP_DICE "pioneers/dice.svg"
#define PIONEERS_PIXMAP_TRADE "pioneers/trade.svg"
#define PIONEERS_PIXMAP_ROAD "pioneers/road.svg"
#define PIONEERS_PIXMAP_SHIP "pioneers/ship.svg"
#define PIONEERS_PIXMAP_SHIP_MOVEMENT "pioneers/ship_move.svg"
#define PIONEERS_PIXMAP_BRIDGE "pioneers/bridge.svg"
#define PIONEERS_PIXMAP_SETTLEMENT "pioneers/settlement.svg"
#define PIONEERS_PIXMAP_CITY "pioneers/city.svg"
#define PIONEERS_PIXMAP_CITY_WALL "pioneers/city_wall.svg"
#define PIONEERS_PIXMAP_DEVELOP "pioneers/develop.svg"
#define PIONEERS_PIXMAP_FINISH "pioneers/finish.svg"

#endif
