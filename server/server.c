/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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

#include "config.h"
#include <stdlib.h>

#include "server.h"
#include "network.h"
#include "avahi.h"
#include "game-list.h"
#include "random.h"

#define TERRAIN_DEFAULT	0
#define TERRAIN_RANDOM	1

static gboolean timed_out(gpointer data)
{
	Game *game = data;
	log_message(MSG_INFO,
		    _(""
		      "Was hanging around for too long without players... bye.\n"));
	request_server_stop(game);
	return FALSE;
}

void start_timeout(Game * game)
{
	if (!game->no_player_timeout)
		return;
	game->no_player_timer =
	    g_timeout_add(game->no_player_timeout * 1000, timed_out, game);
}

void stop_timeout(Game * game)
{
	if (game->no_player_timer != 0) {
		g_source_remove(game->no_player_timer);
		game->no_player_timer = 0;
	}
}

Game *game_new(const GameParams * params)
{
	Game *game;
	guint idx;

	game = g_malloc0(sizeof(*game));

	game->service = NULL;
	game->is_running = FALSE;
	game->is_game_over = FALSE;
	game->is_manipulated = FALSE;
	game->params = params_copy(params);
	game->curr_player = -1;

	for (idx = 0; idx < G_N_ELEMENTS(game->bank_deck); idx++)
		game->bank_deck[idx] = game->params->resource_count;
	develop_shuffle(game);
	if (params->random_terrain)
		map_shuffle_terrain(game->params->map);

	return game;
}

void game_free(Game * game)
{
	if (game == NULL)
		return;

	server_stop(game);

	g_assert(game->player_list_use_count == 0);
	if (game->server_port != NULL)
		g_free(game->server_port);
	params_free(game->params);
	net_service_free(game->service);
	game->service = NULL;
	g_free(game);
}

gint add_computer_player(Game * game, gboolean want_chat)
{
	gchar *child_argv[10];
	GError *error = NULL;
	gint ret = 0;
	gint n = 0;
	gint i;

	child_argv[n++] = g_strdup(PIONEERS_AI_PROGRAM_NAME);
	child_argv[n++] = g_strdup(PIONEERS_AI_PROGRAM_NAME);
	child_argv[n++] = g_strdup("-s");
	child_argv[n++] = g_strdup(PIONEERS_DEFAULT_GAME_HOST);
	child_argv[n++] = g_strdup("-p");
	child_argv[n++] = g_strdup(game->server_port);
	child_argv[n++] = g_strdup("-n");
	child_argv[n++] = player_new_computer_player(game);
	if (!want_chat)
		child_argv[n++] = g_strdup("-c");
	child_argv[n] = NULL;
	g_assert(n < 10);

	if (!g_spawn_async
	    (NULL, child_argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL,
	     &error)) {
		log_message(MSG_ERROR, _("Error starting %s: %s\n"),
			    child_argv[0], error->message);
		g_error_free(error);
		ret = -1;
	}
	for (i = 0; child_argv[i] != NULL; i++)
		g_free(child_argv[i]);
	return ret;
}

static void player_connect(Session * ses, NetEvent event,
			   G_GNUC_UNUSED const gchar * line,
			   gpointer user_data)
{
	Game *game = (Game *) user_data;

	switch (event) {
	case NET_READ:
		/* there is data to be read */
		break;
	case NET_CLOSE:
		/* connection has been closed */
		net_free(&ses);
		break;
	case NET_CONNECT:
		/* new connection was made */
		if (player_new_connection(game, ses) != NULL) {
			stop_timeout(game);
		} else {
			net_close(ses);
		}
		break;
	case NET_CONNECT_FAIL:
		/* connect failed */
		net_free(&ses);
		break;
	}
}

static gboolean game_server_start(Game * game, gboolean register_server,
				  const gchar * metaserver_name)
{
	gchar *error_message;

	game->service =
	    net_service_new(atoi(game->server_port), player_connect, game,
			    &error_message);

	if (game->service == NULL) {
		log_message(MSG_ERROR, "%s\n", error_message);
		g_free(error_message);
		return FALSE;
	}
	game->is_running = TRUE;

	start_timeout(game);

	if (register_server) {
		g_assert(metaserver_name != NULL);
		meta_register(metaserver_name, game);
	}
	avahi_register_game(game);
	return TRUE;
}

/** Try to start a new server.
 * @param params The parameters of the game
 * @param hostname The hostname that will be visible in the metaserver
 * @param port The port to listen to
 * @param register_server Register at the metaserver
 * @param metaserver_name The hostname of the metaserver
 * @param random_order Randomize the player number
 * @return A pointer to the new game, or NULL
*/
Game *server_start(const GameParams * params, const gchar * hostname,
		   const gchar * port, gboolean register_server,
		   const gchar * metaserver_name, gboolean random_order)
{
	Game *game;
	guint32 randomseed;

	g_return_val_if_fail(params != NULL, NULL);
	g_return_val_if_fail(port != NULL, NULL);

#ifdef PRINT_INFO
	g_print("game type: %s\n", params->title);
	g_print("num players: %u\n", params->num_players);
	g_print("victory points: %u\n", params->victory_points);
	g_print("terrain type: %s\n",
		(params->random_terrain) ? "random" : "default");
	g_print("Tournament time: %u\n", params->tournament_time);
	g_print("Quit when done: %d\n", params->quit_when_done);
#endif

	/* create new random seed, to be able to reproduce games */
	randomseed = random_init();
	log_message(MSG_INFO, "%s #%" G_GUINT32_FORMAT ".%s.%03u\n",
		    /* Server: preparing game #..... */
		    _("Preparing game"), randomseed, "G",
		    random_guint(1000));

	game = game_new(params);
	g_assert(game->server_port == NULL);
	game->server_port = g_strdup(port);
	g_assert(game->hostname == NULL);
	if (hostname && strlen(hostname) > 0) {
		game->hostname = g_strdup(hostname);
	}
	game->random_order = random_order;
	if (!game_server_start(game, register_server, metaserver_name)) {
		game_free(game);
		game = NULL;
	}
	return game;
}

/** Stop the server.
 * @param game A game
 * @return TRUE if the game changed from running to stopped
*/
gboolean server_stop(Game * game)
{
	GList *current;

	if (!server_is_running(game))
		return FALSE;

	meta_unregister();
	avahi_unregister_game();

	game->is_running = FALSE;
	net_service_free(game->service);
	game->service = NULL;

	playerlist_inc_use_count(game);
	current = game->player_list;
	while (current != NULL) {
		Player *player = current->data;
		if (!player->disconnected) {
			player_remove(player);
		}
		player_free(player);
		current = g_list_next(current);
	}
	playerlist_dec_use_count(game);

	return TRUE;
}

/** Return true if a game is running */
gboolean server_is_running(Game * game)
{
	if (game != NULL)
		return game->is_running;
	return FALSE;
}

/* game configuration functions / callbacks */
void cfg_set_num_players(GameParams * params, gint num_players)
{
#ifdef PRINT_INFO
	g_print("cfg_set_num_players: %d\n", num_players);
#endif
	g_return_if_fail(params != NULL);
	params->num_players = (guint) CLAMP(num_players, 2, MAX_PLAYERS);
}

void cfg_set_sevens_rule(GameParams * params, gint sevens_rule)
{
#ifdef PRINT_INFO
	g_print("cfg_set_sevens_rule: %d\n", sevens_rule);
#endif
	g_return_if_fail(params != NULL);
	params->sevens_rule = CLAMP(sevens_rule, 0, 2);
}

void cfg_set_use_dice_deck(GameParams * params, gboolean use_dice_deck)
{
#ifdef PRINT_INFO
	g_print("cfg_set_dice_deck: %d\n", use_dice_deck);
#endif
	g_return_if_fail(params != NULL);
	params->use_dice_deck = use_dice_deck;
}

void cfg_set_num_dice_decks(GameParams * params, gint num_dice_decks)
{
#ifdef PRINT_INFO
	g_print("cfg_set_num-dice-decks: %d\n", num_dice_decks);
#endif
	g_return_if_fail(params != NULL);
	params->num_dice_decks = CLAMP(num_dice_decks, 1, 5);
}

void cfg_set_num_removed_dice_cards(GameParams * params,
				    gint num_removed_dice_cards)
{
#ifdef PRINT_INFO
	g_print("cfg_set_dice_deck: %d\n", num_removed_dice_cards);
#endif
	g_return_if_fail(params != NULL);
	params->num_removed_dice_cards =
	    CLAMP(num_removed_dice_cards, 0, 30);
}

void cfg_set_victory_points(GameParams * params, gint victory_points)
{
#ifdef PRINT_INFO
	g_print("cfg_set_victory_points: %d\n", victory_points);
#endif
	g_return_if_fail(params != NULL);
	params->victory_points = (guint) MAX(3, victory_points);
}

GameParams *cfg_set_game(const gchar * game)
{
#ifdef PRINT_INFO
	g_print("cfg_set_game: %s\n", game);
#endif
	if (game_list_is_empty()) {
		game_list_prepare();
		GameParams *param = params_copy(game_list_find_item(game));
		game_list_cleanup();
		return param;
	} else {
		return params_copy(game_list_find_item(game));
	}
}

GameParams *cfg_set_game_file(const gchar * game_filename)
{
#ifdef PRINT_INFO
	g_print("cfg_set_game_file: %s\n", game_filename);
#endif
	return params_load_file(game_filename);
}

void cfg_set_terrain_type(GameParams * params, gint terrain_type)
{
#ifdef PRINT_INFO
	g_print("cfg_set_terrain_type: %d\n", terrain_type);
#endif
	g_return_if_fail(params != NULL);
	params->random_terrain = (terrain_type == TERRAIN_RANDOM) ? 1 : 0;
}

void cfg_set_tournament_time(GameParams * params, gint tournament_time)
{
#ifdef PRINT_INFO
	g_print("cfg_set_tournament_time: %d\n", tournament_time);
#endif
	g_return_if_fail(params != NULL);
	params->tournament_time = (guint) MAX(0, tournament_time);
}

void cfg_set_quit(GameParams * params, gboolean quitdone)
{
#ifdef PRINT_INFO
	g_print("cfg_set_quit: %d\n", quitdone);
#endif
	g_return_if_fail(params != NULL);
	params->quit_when_done = quitdone;
}

void admin_broadcast(Game * game, const gchar * message)
{
	/* The message that is sent must not be translated */
	player_broadcast(player_none(game), PB_SILENT, FIRST_VERSION,
			 LATEST_VERSION, "NOTE1 %s|%s\n", message, "%s");
}

/* server initialization */
void server_init(void)
{
	/* Broken pipes can happen when multiple players disconnect
	 * simultaneously.  This mostly happens to AI's, which disconnect
	 * when the game is over. */
	/* SIGPIPE does not exist for G_OS_WIN32 */
#ifndef G_OS_WIN32
	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);
#endif				/* G_OS_WIN32 */
}
