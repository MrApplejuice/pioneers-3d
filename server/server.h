/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003-2007 Bas Wijnen <shevek@fmf.nl>
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

#ifndef __server_h
#define __server_h

#include "game.h"
#include "cards.h"
#include "map.h"
#include "quoteinfo.h"
#include "state.h"
#include "network.h"

#define TERRAIN_DEFAULT	0
#define TERRAIN_RANDOM	1

typedef struct Game Game;
typedef struct {
	StateMachine *sm;	/* state machine for this player */
	Game *game;		/* game that player belongs to */

	gchar *location;	/* reverse lookup player hostname */
	gint num;		/* number each player */
	char *name;		/* give each player a name */
	gchar *style;		/* description of the player icon */
	ClientVersionType version;	/* version, so adapted messages can be sent */

	GList *build_list;	/* list of building that can be undone */
	gint prev_assets[NO_RESOURCE];	/* remember previous resources */
	gint assets[NO_RESOURCE];	/* our resources */
	gint gold;		/* how much gold will we receive? */
	Deck *devel;		/* development cards we own */
	GList *special_points;	/* points from special actions */
	gint special_points_next_id;	/* Next id for the special points */
	gint discard_num;	/* number of resources we must discard */

	gint num_roads;		/* number of roads available */
	gint num_bridges;	/* number of bridges available */
	gint num_ships;		/* number of ships available */
	gint num_settlements;	/* settlements available */
	gint num_cities;	/* cities available */
	gint num_city_walls;	/* city walls available */

	gint num_soldiers;	/* number of soldiers played */
	gint develop_points;	/* number of development card victory points */
	gint chapel_played;	/* number of Chapel cards played */
	gint univ_played;	/* number of University cards played */
	gint gov_played;	/* number of Governors cards played */
	gint libr_played;	/* number of Library cards played */
	gint market_played;	/* number of Market cards played */
	guint islands_discovered;	/* number of islands discovered */
	gboolean disconnected;
} Player;

struct Game {
	GameParams *params;	/* game parameters */
	gchar *hostname;	/* reported hostname */

	Service *service;	/* network service */

	GList *player_list;	/* all players in the game */
	GList *dead_players;	/* all players that should be removed when player_list_use_count == 0 */
	gint player_list_use_count;	/* # functions is in use by */
	guint num_players;	/* current number of players in the game */

	guint tournament_countdown;	/* number of remaining minutes before AIs are added */
	guint tournament_timer;	/* timer id */

	gboolean double_setup;
	gboolean reverse_setup;
	GList *setup_player;

	gboolean is_game_over;	/* is the game over? */
	Player *longest_road;	/* who holds longest road */
	Player *largest_army;	/* who has largest army */

	QuoteList *quotes;	/* domestic trade quotes */
	gint quote_supply[NO_RESOURCE];	/* only valid when trading */
	gint quote_receive[NO_RESOURCE];	/* only valid when trading */

	gint curr_player;	/* whose turn is it? */
	gint curr_turn;		/* current turn number */
	gboolean rolled_dice;	/* has dice been rolled in turn yet? */
	gint die1, die2;	/* latest dice values */
	guint dice_cards[36];	/* remaining dice cards */
	guint num_dice_cards;	/* total remaining dice cards */
	gboolean bought_develop;	/* has devel. card been bought in turn? */
	guint num_playable_cards;	/* number of playable development cards */

	gint bank_deck[NO_RESOURCE];	/* resource card bank */
	gint num_develop;	/* number of development cards */
	gint *develop_deck;	/* development cards */
	gint develop_next;	/* next development card to deal */

	gboolean is_running;	/* is the server currently running? */
	gchar *server_port;	/* port to run game on */
	gboolean random_order;	/* is turn order randomized? */
	gboolean is_manipulated;	/* has the game been manipulated by the admin? */

	guint no_player_timeout;	/* time to wait for players */
	guint no_player_timer;	/* glib timer identifier */

	guint no_humans_timer;	/* timer id: no human players are present */
};

/**** global variables ****/
/* buildutil.c */
void check_longest_road(Game * game);
void node_add(Player * player,
	      BuildType type, int x, int y, int pos, gboolean paid_for,
	      Points * special_points);
void edge_add(Player * player, BuildType type, int x, int y, int pos,
	      gboolean paid_for);
gboolean perform_undo(Player * player);

/* develop.c */
void develop_shuffle(Game * game);
void develop_buy(Player * player);
void develop_play(Player * player, guint idx);
gboolean mode_road_building(Player * player, gint event);
gboolean mode_plenty_resources(Player * player, gint event);
gboolean mode_monopoly(Player * player, gint event);

/* discard.c */
void discard_resources(Game * player);
gboolean mode_discard_resources(Player * player, gint event);
gboolean mode_wait_others_place_robber(Player * player, gint event);
gboolean mode_discard_resources_place_robber(Player * player, gint event);

/* meta.c */
gchar *get_server_name(void);
void meta_register(const gchar * server, Game * game);
void meta_unregister(void);
void meta_start_game(void);
void meta_report_num_players(guint num_players);

/* player.c */
typedef enum {
	PB_ALL,
	PB_RESPOND,
	PB_SILENT,
	PB_OTHERS
} BroadcastType;
gchar *player_new_computer_player(Game * game);
Player *player_new(Game * game, const gchar * name);
Player *player_new_connection(Game * game, Session * ses);
Player *player_by_num(Game * game, gint num);
void player_set_name(Player * player, gchar * name);
Player *player_none(Game * game);
void player_broadcast(Player * player, BroadcastType type,
		      ClientVersionType first_supported_version,
		      ClientVersionType last_supported_version,
		      const char *fmt, ...);
void player_broadcast_extension(Player * player, BroadcastType type,
				ClientVersionType first_supported_version,
				ClientVersionType last_supported_version,
				const char *fmt, ...);
void player_send(Player * player,
		 ClientVersionType first_supported_version,
		 ClientVersionType last_supported_version, const char *fmt,
		 ...);
void player_send_uncached(Player * player,
			  ClientVersionType first_supported_version,
			  ClientVersionType last_supported_version,
			  const char *fmt, ...);
void player_remove(Player * player);
void player_free(Player * player);
void player_archive(Player * player);
void player_revive(Player * newp, char *name);
GList *player_first_real(Game * game);
GList *player_next_real(GList * last);
GList *list_from_player(Player * player);
GList *next_player_loop(GList * current, Player * first);
gboolean mode_spectator(Player * player, gint event);
void playerlist_inc_use_count(Game * game);
void playerlist_dec_use_count(Game * game);
gboolean player_is_spectator(Game * game, gint player_num);

/* pregame.c */
gboolean mode_pre_game(Player * player, gint event);
gboolean mode_setup(Player * player, gint event);
gboolean send_gameinfo_uncached(const Hex * hex, void *player);
void next_setup_player(Game * game);

/* resource.c */
gboolean resource_available(Player * player,
			    gint * resources, gint * num_in_bank);
void resource_maritime_trade(Player * player,
			     Resource supply, Resource receive,
			     gint ratio);
void resource_start(Game * game);
void resource_end(Game * game, const gchar * action, gint mult);
void resource_spend(Player * player, const gint * cost);
void resource_refund(Player * player, const gint * cost);

/* robber.c */
void robber_place(Player * player);
gboolean mode_place_robber(Player * player, gint event);
gboolean mode_select_pirated(Player * player, gint event);
gboolean mode_select_robbed(Player * player, gint event);
void robber_undo(Player * player);

/* server.c */
void start_timeout(Game * game);
void stop_timeout(Game * game);
Game *game_new(const GameParams * params);
void game_free(Game * game);
gint add_computer_player(Game * game, gboolean want_chat);
Game *server_start(const GameParams * params, const gchar * hostname,
		   const gchar * port, gboolean register_server,
		   const gchar * metaserver_name, gboolean random_order);
gboolean server_stop(Game * game);
gboolean server_is_running(Game * game);
gint accept_connection(gint in_fd, gchar ** location);

/**** callbacks to set parameters ****/
GameParams *cfg_set_game(const gchar * game);
GameParams *cfg_set_game_file(const gchar * game_filename);
void cfg_set_num_players(GameParams * params, gint num_players);
void cfg_set_sevens_rule(GameParams * params, gint sevens_rule);
void cfg_set_use_dice_deck(GameParams * params, gboolean use_dice_deck);
void cfg_set_num_dice_decks(GameParams * params, gint num_dice_decks);
void cfg_set_num_removed_dice_cards(GameParams * params,
				    gint num_removed_dice_cards);
void cfg_set_victory_points(GameParams * params, gint victory_points);
void cfg_set_terrain_type(GameParams * params, gint terrain_type);
void cfg_set_tournament_time(GameParams * params, gint tournament_time);
void cfg_set_quit(GameParams * params, gboolean quitdone);
void admin_broadcast(Game * game, const gchar * message);

/* initialize the server */
void server_init(void);
void game_is_over(Game * game);
void request_server_stop(Game * game);

/* trade.c */
void trade_perform_maritime(Player * player,
			    gint ratio, Resource supply, Resource receive);
gboolean mode_domestic_quote_rejected(Player * player, gint event);
gboolean mode_domestic_quote(Player * player, gint event);
void trade_finish_domestic(Player * player);
void trade_accept_domestic(Player * player,
			   gint partner_num, gint quote_num,
			   gint * supply, gint * receive);
gboolean mode_domestic_initiate(Player * player, gint event);
void trade_begin_domestic(Player * player, gint * supply, gint * receive);

/* turn.c */
gboolean mode_idle(Player * player, gint event);
gboolean mode_turn(Player * player, gint event);
void turn_next_player(Game * game);
/** Check whether this player has won the game.
 *  If so, return TRUE and set all state machines to idle
 *  @param player Has this player won?
 *  @return TRUE if the given player has won
 */
gboolean check_victory(Player * player);

/* gold.c */
gboolean gold_limited_bank(const Game * game, int limit,
			   gint * limited_bank);
void distribute_first(GList * list);
gboolean mode_choose_gold(Player * player, gint event);
gboolean mode_wait_for_gold_choosing_players(Player * player, gint event);

/* discard.c */
gboolean mode_wait_for_other_discarding_players(Player * player,
						gint event);
#endif
