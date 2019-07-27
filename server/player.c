/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003-2007 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2005-2010 Roland Clobus <rclobus@rclobus.nl>
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "server.h"
#include "network.h"
#include "random.h"

/* Local function prototypes */
static gboolean mode_check_version(Player * player, gint event);
static gboolean mode_check_status(Player * player, gint event);
static gboolean mode_bad_version(Player * player, gint event);
static gboolean mode_global(Player * player, gint event);
static gboolean mode_unhandled(Player * player, gint event);
static void player_setup(Player * player, gint playernum,
			 const gchar * name, gboolean force_spectator);
static Player *player_by_name(Game * game, char *name);

#define tournament_minute 1000 * 60
#define time_to_wait_for_players 30 * 60 * 1000

/** Is the game a tournament game?
 *  @param game The game
 *  @return TRUE if this game is a tournament game
*/
static gboolean is_tournament_game(const Game * game)
{
	return game->params->tournament_time > 0;
}

/** Find a free number for a connecting player.
 * The number has not been used before.
 *  @param game The game
 *  @param force_spectator The connecting player must be a spectator
 */
static gint next_free_player_num(Game * game, gboolean force_spectator)
{
	gint idx;

	if (!force_spectator) {
		GList *list;
		gboolean player_taken[MAX_PLAYERS];
		guint available = game->params->num_players;

		memset(player_taken, 0, sizeof(player_taken));
		playerlist_inc_use_count(game);
		for (list = game->player_list;
		     list != NULL; list = g_list_next(list)) {
			Player *player = list->data;
			if (player->num >= 0
			    && !player_is_spectator(game, player->num)) {
				player_taken[player->num] = TRUE;
				--available;
			}
		}
		playerlist_dec_use_count(game);
		if (available > 0) {
			guint skip;
			if (game->random_order) {
				skip = random_guint(available);
			} else {
				skip = 0;
			}
			idx = 0;
			++skip;
			while (player_taken[idx] || --skip != 0)
				++idx;
			return idx;
		}
	}

	/* No players available/wanted, look for a spectator number */
	idx = (gint) game->params->num_players;
	while (player_by_num(game, idx) != NULL)
		++idx;
	return idx;
}

static gboolean mode_global(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	Game *game = player->game;
	gchar *text;

	switch (event) {
	case SM_FREE:
		if (player->name != NULL)
			g_free(player->name);
		if (player->style != NULL)
			g_free(player->style);
		if (player->location != NULL)
			g_free(player->location);
		if (player->devel != NULL)
			deck_free(player->devel, NULL);
		if (player->num >= 0
		    && !player_is_spectator(game, player->num)
		    && !player->disconnected) {
			game->num_players--;
			meta_report_num_players(game->num_players);
		}
		g_list_free(player->build_list);
		g_list_free(player->special_points);
		g_free(player);
		return TRUE;
	case SM_NET_CLOSE:
		player_remove(player);
		if (player->num >= 0) {
			player_broadcast(player, PB_OTHERS, FIRST_VERSION,
					 LATEST_VERSION, "has quit\n");
			player_archive(player);
		} else {
			player_free(player);
		}
		driver->player_change(game);
		return TRUE;
	case SM_RECV:
		if (sm_recv(sm, "chat %S", &text)) {
			if (strlen(text) > MAX_CHAT)
				player_send(player, FIRST_VERSION,
					    LATEST_VERSION, "ERR %s\n",
					    _("chat too long"));
			else
				player_broadcast(player, PB_ALL,
						 FIRST_VERSION,
						 LATEST_VERSION,
						 "chat %s\n", text);
			g_free(text);
			return TRUE;
		}
		if (sm_recv(sm, "name %S", &text)) {
			if (text[0] == '\0')
				player_send(player, FIRST_VERSION,
					    LATEST_VERSION,
					    "ERR invalid-name\n");
			else if (strlen(text) > MAX_NAME_LENGTH)
				player_send(player, FIRST_VERSION,
					    LATEST_VERSION, "ERR %s\n",
					    _("name too long"));
			else
				player_set_name(player, text);
			g_free(text);
			return TRUE;
		}
		if (sm_recv(sm, "style %S", &text)) {
			if (player->style)
				g_free(player->style);
			player->style = text;
			player_broadcast(player, PB_ALL, V0_11,
					 LATEST_VERSION, "style %s\n",
					 text);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

static gboolean mode_unhandled(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	gchar *text;

	switch (event) {
	case SM_RECV:
		if (sm_recv(sm, "extension %S", &text)) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "NOTE %s\n",
				    N_("ignoring unknown extension"));
			log_message(MSG_INFO,
				    "ignoring unknown extension from %s: %s\n",
				    player->name, text);
			g_free(text);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

/* Called to start the game (if it hasn't been yet). Add computer
 * players to fill any empty spots
 *
 */
static gboolean tournament_start_cb(gpointer data)
{
	guint i;
	Game *game = (Game *) data;
	GList *player;
	gboolean human_player_present;

	g_source_remove(game->tournament_timer);
	game->tournament_timer = 0;

	/* if game already started */
	if (game->num_players == game->params->num_players)
		return FALSE;

	if (game->num_players == 0) {
		player_broadcast(player_none(game), PB_SILENT,
				 FIRST_VERSION, LATEST_VERSION,
				 "NOTE %s\n",
				 N_
				 ("The last player left, the "
				  "tournament timer is reset."));
		game->tournament_countdown = game->params->tournament_time;
		return FALSE;
	}

	/* remove all disconnected players */
	playerlist_inc_use_count(game);
	for (player = game->player_list; player != NULL;
	     player = g_list_next(player)) {
		Player *p = player->data;
		if (p->disconnected && !sm_get_use_cache(p->sm)) {
			player_free(p);
		}
	}
	playerlist_dec_use_count(game);

	/* if no human players are present, quit */
	playerlist_inc_use_count(game);
	human_player_present = FALSE;
	for (player = game->player_list;
	     player != NULL && !human_player_present;
	     player = g_list_next(player)) {
		Player *p = player->data;
		if (!player_is_spectator(game, p->num)
		    && determine_player_type(p->style) == PLAYER_HUMAN) {
			human_player_present = TRUE;
		}
	}
	playerlist_dec_use_count(game);
	if (!human_player_present) {
		player_broadcast(player_none(game), PB_SILENT,
				 FIRST_VERSION, LATEST_VERSION,
				 "NOTE %s\n",
				 N_("No human players present. Bye."));
		request_server_stop(game);
		return FALSE;
	}

	player_broadcast(player_none(game), PB_SILENT, FIRST_VERSION,
			 LATEST_VERSION, "NOTE %s\n",
			 N_("Game starts, adding computer players."));

	/* add computer players to start game */
	for (i = game->num_players; i < game->params->num_players; i++) {
		add_computer_player(game, TRUE);
	}

	return FALSE;
}

/*
 * Keep players notified about when the tournament game is going to start
 *
 */
static gboolean talk_about_tournament_cb(gpointer data)
{
	Game *game = (Game *) data;
	const gchar *message;

	/* if game already started */
	if (game->num_players == game->params->num_players)
		return FALSE;

	if (game->num_players == 0) {
		if (game->tournament_timer != 0) {
			player_broadcast(player_none(game), PB_SILENT,
					 FIRST_VERSION, LATEST_VERSION,
					 "NOTE %s\n",
					 N_
					 ("The last player left, the "
					  "tournament timer is reset."));
			game->tournament_countdown =
			    game->params->tournament_time;
			g_source_remove(game->tournament_timer);
			game->tournament_timer = 0;
		}
		return FALSE;
	}

	/* ngettext can not be used here,
	 * because the string must be sent untranslated */
	message = game->tournament_countdown != 1 ?
	    N_("The game starts in %s minutes.") :
	    N_("The game starts in %s minute.");

	player_broadcast(player_none(game), PB_SILENT, FIRST_VERSION,
			 LATEST_VERSION, "NOTE1 %d|%s\n",
			 game->tournament_countdown, message);
	game->tournament_countdown--;

	if (game->tournament_countdown > 0)
		g_timeout_add(tournament_minute,
			      &talk_about_tournament_cb, game);

	return FALSE;
}

/** Generate a name for a computer player.
 * The name will be unique for the game.
*/
static gchar *generate_name_for_computer_player(Game * game)
{
	gchar *filename;
	FILE *stream;
	gchar *line;
	gchar *name = NULL;
	int num = 1;

	filename =
	    g_build_filename(get_pioneers_dir(), "computer_names", NULL);
	stream = fopen(filename, "r");
	if (!stream) {
		g_warning("Unable to open %s", filename);
		/* Default name for the AI when the computer_names file
		 * is not found or empty.
		 */
	} else {
		while (read_line_from_file(&line, stream)) {
			if (player_by_name(game, line) == NULL) {
				if (g_random_int_range(0, num) == 0) {
					if (name)
						g_free(name);
					name = g_strdup(line);
				}
				num++;
			}
		}
		fclose(stream);
		if (num == 1) {
			g_warning("Empty file or all names taken: %s",
				  filename);
		}
	}
	g_free(filename);

	if (name == NULL) {
		gint counter = 2;

		/* Default name for the AI when the computer_names file
		 * is not found or empty.
		 */
		name = g_strdup(_("Computer Player"));

		while (player_by_name(game, name) != NULL) {
			g_free(name);
			name =
			    g_strdup_printf("%s (%d)",
					    _("Computer Player"),
					    counter++);
		}
	}

	return name;
}

/** Add a new computer player (disconnected)
 */
gchar *player_new_computer_player(Game * game)
{
	Player *player;
	gchar *name;

	/* Reserve the name, so the names of the computer players will
	   be unique */
	name = generate_name_for_computer_player(game);
	player = player_new(game, name);
	player->disconnected = TRUE;
	sm_goto(player->sm, (StateFunc) mode_idle);
	return name;
}

/** Allocate a new Player struct.
 *  The StateMachine is not initialized.
 *   */
Player *player_new(Game * game, const gchar * name)
{
	Player *player;
	StateMachine *sm;

	player = g_malloc0(sizeof(*player));
	sm = player->sm = sm_new(player);

	sm_global_set(sm, (StateFunc) mode_global);
	sm_unhandled_set(sm, (StateFunc) mode_unhandled);

	player->game = game;
	player->location = g_strdup("not connected");
	player->devel = deck_new();
	game->player_list = g_list_append(game->player_list, player);
	player->num = -1;
	player->chapel_played = 0;
	player->univ_played = 0;
	player->gov_played = 0;
	player->libr_played = 0;
	player->market_played = 0;
	player->islands_discovered = 0;
	player->disconnected = FALSE;
	player->name = g_strdup(name);
	player->style = NULL;
	player->special_points = NULL;
	player->special_points_next_id = 0;

	driver->player_change(game);

	return player;
}

Player *player_new_connection(Game * game, Session * ses)
{
	gchar name[100];
	size_t i;
	Player *player;
	StateMachine *sm;
	GError *error;
	gchar *location;
	gchar *port;

	error = NULL;
	if (!net_get_peer_name(ses, &location, &port, &error)) {
		/* %s = error message */
		log_message(MSG_ERROR,
			    _("Unable to determine the "
			      "hostname of the player: %s"),
			    error->message);
		g_error_free(error);
	}

	/* give player a name, some functions need it */
	strcpy(name, "connecting");
	for (i = strlen(name); i < G_N_ELEMENTS(name) - 1; ++i) {
		if (player_by_name(game, name) == NULL)
			break;
		name[i] = '_';
		name[i + 1] = 0;
	}
	if (i == G_N_ELEMENTS(name) - 1) {
		/* there are too many pending connections */
		net_write(ses, "ERR Too many connections\n");
		g_free(location);
		g_free(port);
		return NULL;
	}

	if (game->is_game_over) {
		/* The game is over, don't accept new players */
		/* Message to send to the client when the game is already over
		 * when a connection is made. */
		net_printf(ses, "NOTE %s\n", N_("Sorry, game is over."));
		log_message(MSG_INFO,
			    _("Player from %s is refused: game is over\n"),
			    location);
		g_free(location);
		g_free(port);
		return NULL;
	}

	player = player_new(game, name);
	sm = player->sm;
	sm_set_session(sm, ses);
	net_set_check_connection_alive(ses, 30);
	g_free(player->location);
	player->location = g_strdup(location);

	/* Cache messages of the game in progress until all initial
	 * messages have been sent
	 */
	sm_set_use_cache(sm, TRUE);

	sm_goto(sm, (StateFunc) mode_check_version);

	driver->player_change(game);
	return player;
}

/* set the player name.  Most of the time, player_set_name is called instead,
 * which calls this function with public set to TRUE.  Only player_setup calls
 * this with public == FALSE, because it doesn't want the broadcast. */
static void player_set_name_real(Player * player, gchar * name,
				 gboolean public)
{
	Game *game = player->game;
	Player *player_temp;

	g_assert(name[0] != 0);

	if (((player_temp = player_by_name(game, name)) != NULL) &&
	    (player_temp != player)) {
		/* make it a note, not an error, so nothing bad happens
		 * (on error the AI would disconnect) */
		player_send(player, FIRST_VERSION, LATEST_VERSION,
			    "NOTE %s\n",
			    N_(""
			       "Name not changed: new name is already in use"));
		return;
	}

	if (player->name != name) {
		g_free(player->name);
		player->name = g_strdup(name);
	}

	if (public)
		player_broadcast(player, PB_ALL, FIRST_VERSION,
				 LATEST_VERSION, "is %s\n", player->name);

	driver->player_renamed(player);
	driver->player_change(game);
}

static void player_setup(Player * player, gint playernum,
			 const gchar * name, gboolean force_spectator)
{
	gchar nm[MAX_NAME_LENGTH + 1];
	Game *game = player->game;
	StateMachine *sm = player->sm;
	Player *other;

	player->num = playernum;
	if (player->num < 0) {
		player->num = next_free_player_num(game, force_spectator);
	}

	if (!player_is_spectator(game, player->num)) {
		game->num_players++;
		meta_report_num_players(game->num_players);
	}

	player->num_roads = 0;
	player->num_bridges = 0;
	player->num_ships = 0;
	player->num_settlements = 0;
	player->num_cities = 0;

	/* give the player her new name */
	if (name == NULL) {
		if (player_is_spectator(game, player->num)) {
			gint num = 1;
			do {
				sprintf(nm, _("Spectator %d"), num++);
			} while (player_by_name(game, nm) != NULL);
		} else {
			sprintf(nm, _("Player %d"), player->num);
		}
	} else {
		strncpy(nm, name, G_N_ELEMENTS(nm));
		nm[G_N_ELEMENTS(nm) - 1] = '\0';
	}

	/* if the new name exists, try padding it with underscores */
	other = player_by_name(game, nm);
	if (other != player && other != NULL) {
		size_t i;
		/* add underscores until the name is unique */
		for (i = strlen(nm); i < G_N_ELEMENTS(nm) - 1; ++i) {
			if (player_by_name(game, nm) == NULL)
				break;
			nm[i] = '_';
			nm[i + 1] = 0;
		}
		/* Adding underscores was not enough to make the name unique.
		 * While staying within the maximum name length,
		 * create numbers at the end of the name.
		 * Repeat until an unique name has been found.
		 */
		while (player_by_name(game, nm)) {
			gint digit = 10;
			i = G_N_ELEMENTS(nm) - 1;
			while (digit == 10 && i > 0) {
				/* Digit will be: 0..10 */
				--i;
				digit = g_ascii_digit_value(nm[i]) + 1;
				nm[i] = (gchar) ('0' + digit % 10);
			}
		}
	}
	/* copy the (possibly new) name to dynamic memory */
	/* don't broadcast the name.  This is done by mode_pre_game, after
	 * telling the user how many players are in the game.
	 * That should keep things easier for the client. */
	player_set_name_real(player, nm, FALSE);

	/* add the info in the output device */
	driver->player_added(player);
	driver->player_change(game);
	if (playernum < 0)
		sm_goto(sm, (StateFunc) mode_pre_game);
}

void player_free(Player * player)
{
	Game *game = player->game;

	if (game->player_list_use_count > 0) {
		game->dead_players =
		    g_list_append(game->dead_players, player);
		player->disconnected = TRUE;
		return;
	}

	game->player_list = g_list_remove(game->player_list, player);
	driver->player_change(game);

	sm_free(player->sm);
}

static gboolean timed_out(gpointer data)
{
	Game *game = data;
	log_message(MSG_INFO,
		    _(""
		      "Was hanging around for too long without players... bye.\n"));
	player_broadcast(player_none(game), PB_SILENT,
			 FIRST_VERSION, LATEST_VERSION,
			 "NOTE %s\n",
			 N_("No human players present. Bye."));
	request_server_stop(game);
	return FALSE;
}

void player_archive(Player * player)
{
	StateFunc state;
	Game *game = player->game;
	gboolean human_player_present;
	GList *pl;

	/* If this was a spectator, forget about him */
	if (player_is_spectator(game, player->num)) {
		player_free(player);
		return;
	}
	/* If this game can't be started, forget old players */
	if (params_game_is_unstartable(game->params)) {
		player_free(player);
		return;
	}

	/* If the player was in the middle of a trade, pop the state
	   machine and inform others as necessary */
	state = sm_current(player->sm);
	if (state == (StateFunc) mode_domestic_quote_rejected) {
		/* No special actions needed */
	} else if (state == (StateFunc) mode_domestic_quote) {
		/* Retract all quotes */
		for (;;) {
			QuoteInfo *quote;
			quote = quotelist_find_domestic(game->quotes,
							player->num, -1);
			if (quote == NULL)
				break;
			quotelist_delete(game->quotes, quote);
			player_broadcast(player, PB_RESPOND, FIRST_VERSION,
					 LATEST_VERSION,
					 "domestic-quote delete %d\n",
					 quote->var.d.quote_num);
		}
	} else if (state == (StateFunc) mode_domestic_initiate) {
		/* End the trade */
		trade_finish_domestic(player);
	}

	/* If the player was robbing something, auto-undo to robber
	 * placement.  */
	if (state == (StateFunc) mode_select_robbed
	    || state == (StateFunc) mode_select_pirated)
		robber_undo(player);

	/* Mark the player as disconnected */
	player->disconnected = TRUE;
	game->num_players--;
	meta_report_num_players(game->num_players);

	/* if no human players are present, start timer */
	playerlist_inc_use_count(game);
	human_player_present = FALSE;
	for (pl = game->player_list;
	     pl != NULL && !human_player_present; pl = g_list_next(pl)) {
		Player *p = pl->data;
		if (!player_is_spectator(game, p->num)
		    && !p->disconnected
		    && determine_player_type(p->style) == PLAYER_HUMAN) {
			human_player_present = TRUE;
		}
	}
	playerlist_dec_use_count(game);
	if (!human_player_present && game->no_humans_timer == 0
	    && is_tournament_game(game)) {
		game->no_humans_timer =
		    g_timeout_add(time_to_wait_for_players, timed_out,
				  game);
		player_broadcast(player_none(game), PB_SILENT,
				 FIRST_VERSION, LATEST_VERSION,
				 "NOTE %s\n",
				 N_
				 ("The last human player left. Waiting for the return of a player."));
	}
}

/* Try to revive the player
   newp: Player* attempt to revive this player
   name: The player wants to have this name, if possible
*/
void player_revive(Player * newp, char *name)
{
	Game *game = newp->game;
	GList *current = NULL;
	Player *p = NULL;
	gboolean reviving_player_in_setup;
	gchar *safe_name;

	if (game->no_humans_timer != 0) {
		g_source_remove(game->no_humans_timer);
		game->no_humans_timer = 0;
		player_broadcast(player_none(game), PB_SILENT,
				 FIRST_VERSION, LATEST_VERSION,
				 "NOTE %s\n", N_("Resuming the game."));
	}

	/* first see if a player with the given name exists */
	if (name) {
		playerlist_inc_use_count(game);
		for (current = game->player_list; current != NULL;
		     current = g_list_next(current)) {
			p = current->data;
			if (!strcmp(name, p->name))
				if (p->disconnected
				    && !sm_get_use_cache(p->sm)
				    && p != newp)
					break;
		}
		playerlist_dec_use_count(game);
	}
	/* if not, try to find an unused player number */
	if (current == NULL) {
		gint num;

		num = next_free_player_num(game, FALSE);
		if (num < (gint) game->params->num_players) {
			player_setup(newp, -1, name, FALSE);
			return;
		}
	}
	/* if not, try to take over another disconnected player */
	if (current == NULL) {
		playerlist_inc_use_count(game);
		for (current = game->player_list; current != NULL;
		     current = g_list_next(current)) {
			p = current->data;
			if (p->disconnected && !sm_get_use_cache(p->sm)
			    && p != newp)
				break;
		}
		playerlist_dec_use_count(game);
	}
	/* if still no player is found, do a normal setup */
	if (current == NULL) {
		player_setup(newp, -1, name, FALSE);
		return;
	}

	/* Reviving the player that is currently in the setup phase */
	reviving_player_in_setup =
	    (game->setup_player && game->setup_player->data == p);

	/* remove the disconnected player from the player list, it's memory will be freed at the end of this routine */
	game->player_list = g_list_remove(game->player_list, p);

	/* initialize the player */
	player_setup(newp, p->num, name, FALSE);

	/* mark the player as a reconnect */
	newp->disconnected = TRUE;

	/* Don't use the old player's name */

	/* copy over all the data from p */
	g_assert(newp->build_list == NULL);
	newp->build_list = p->build_list;
	p->build_list = NULL;	/* prevent deletion */

	memcpy(newp->prev_assets, p->prev_assets,
	       sizeof(newp->prev_assets));
	memcpy(newp->assets, p->assets, sizeof(newp->assets));
	newp->gold = p->gold;
	/* take over the development deck */
	deck_free(newp->devel, NULL);
	newp->devel = p->devel;
	p->devel = NULL;

	g_assert(newp->special_points == NULL);
	newp->special_points = p->special_points;
	p->special_points = NULL;	/* prevent deletion */

	newp->discard_num = p->discard_num;
	newp->num_roads = p->num_roads;
	newp->num_bridges = p->num_bridges;
	newp->num_ships = p->num_ships;
	newp->num_settlements = p->num_settlements;
	newp->num_cities = p->num_cities;
	newp->num_soldiers = p->num_soldiers;
	newp->develop_points = p->develop_points;
	newp->chapel_played = p->chapel_played;
	newp->univ_played = p->univ_played;
	newp->gov_played = p->gov_played;
	newp->libr_played = p->libr_played;
	newp->market_played = p->market_played;
	/* Not copied: sm, game, location, num, client_version */

	sm_copy_stack(newp->sm, p->sm);
	if (sm_current(newp->sm) != (StateFunc) mode_pre_game)
		sm_push(newp->sm, (StateFunc) mode_pre_game);
	else
		sm_goto(newp->sm, (StateFunc) mode_pre_game);

	/* Copy longest road and largest army */
	if (game->longest_road == p)
		game->longest_road = newp;
	if (game->largest_army == p)
		game->largest_army = newp;

	if (reviving_player_in_setup) {
		/* Restore the pointer */
		game->setup_player = game->player_list;
		while (game->setup_player
		       && game->setup_player->data != newp) {
			game->setup_player =
			    g_list_next(game->setup_player);
		}
		g_assert(game->setup_player != NULL);
	}
	p->num = -1;		/* prevent the number of players
				   from getting decremented */

	player_free(p);

	/* Make sure the name in the broadcast doesn't contain the separator */
	safe_name = g_strdup(newp->name);
	g_strdelimit(safe_name, "|", '_');
	player_broadcast(newp, PB_SILENT, FIRST_VERSION, LATEST_VERSION,
			 "NOTE1 %s|%s\n", safe_name,
			 /* %s is the name of the reconnecting player */
			 N_("%s has reconnected."));
	g_free(safe_name);
	return;
}

gboolean mode_spectator(Player * player, gint event)
{
	gint num;
	Game *game = player->game;
	StateMachine *sm = player->sm;
	Player *other;

	sm_state_name(sm, "mode_spectator");
	if (event != SM_RECV)
		return FALSE;
	/* first see if this is a valid event for this mode */
	if (sm_recv(sm, "play")) {
		/* try to be the first available player */
		num = next_free_player_num(game, FALSE);
		if (num >= (gint) game->params->num_players) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR game-full");
			return TRUE;
		}
	} else if (sm_recv(sm, "play %d", &num)) {
		/* try to be the specified player number */
		if (num >= (gint) game->params->num_players
		    || num < 0
		    || !player_by_num(game, num)->disconnected) {
			player_send(player, FIRST_VERSION, LATEST_VERSION,
				    "ERR invalid-player");
			return TRUE;
		}
	} else
		/* input was not what we expected,
		 * see if mode_unhandled likes it */
		return FALSE;

	other = player_by_num(game, num);
	if (other == NULL) {
		player_send(player, FIRST_VERSION, LATEST_VERSION, "Ok\n");
		player_broadcast(player, PB_ALL, FIRST_VERSION,
				 LATEST_VERSION, "was spectator %d\n",
				 player->num);
		sm_set_use_cache(player->sm, TRUE);
		player_setup(player, -1, player->name, FALSE);
		sm_goto(sm, (StateFunc) mode_pre_game);
		return TRUE;
	}
	sm_set_use_cache(player->sm, TRUE);
	player_revive(player, player->name);
	return TRUE;
}

static gboolean mode_bad_version(Player * player, gint event)
{
	StateMachine *sm = player->sm;

	sm_state_name(sm, "mode_bad_version");
	switch (event) {
	case SM_ENTER:
		player_send_uncached(player, FIRST_VERSION, LATEST_VERSION,
				     "ERR sorry, version conflict\n");
		player_free(player);
		break;
	}
	return FALSE;
}

static gboolean mode_check_version(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	gchar *version;

	sm_state_name(sm, "mode_check_version");
	switch (event) {
	case SM_ENTER:
		player_send_uncached(player, UNKNOWN_VERSION,
				     UNKNOWN_VERSION, "version report\n");
		break;

	case SM_RECV:
		if (sm_recv(sm, "version %S", &version)) {
			ClientVersionType cvt =
			    client_version_type_from_string(version);
			player->version = cvt;
			if (can_client_connect_to_server
			    (cvt, LATEST_VERSION)) {
				sm_goto(sm, (StateFunc) mode_check_status);
			} else {
				gchar *mismatch =
				    g_strdup_printf("%s <-> %s",
						    client_version_type_to_string
						    (LATEST_VERSION),
						    version);
				/* Make sure the argument does not contain the separator */
				g_strdelimit(mismatch, "|", '_');
				player_send_uncached(player, cvt, cvt,
						     "NOTE1 %s|%s\n",
						     mismatch,
						     N_(""
							"Version mismatch: %s"));
				g_free(mismatch);
				sm_goto(sm, (StateFunc) mode_bad_version);
			}
			g_free(version);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

static void start_tournament_mode(Player * player)
{
	Game *game = player->game;

	if (is_tournament_game(game)) {
		/* if first player in and this is a tournament start the timer */
		if (game->num_players == 1) {
			game->tournament_countdown =
			    game->params->tournament_time;
			game->tournament_timer =
			    g_timeout_add(game->tournament_countdown *
					  tournament_minute + 500,
					  &tournament_start_cb, game);
			g_timeout_add(1000, &talk_about_tournament_cb,
				      game);
		} else {
			if (game->tournament_timer != 0
			    && game->num_players !=
			    game->params->num_players) {
				player_send(player, FIRST_VERSION,
					    LATEST_VERSION, "NOTE %s\n",
					    N_
					    ("This game will start soon."));
			}
		}
	}
}

static gboolean mode_check_status(Player * player, gint event)
{
	StateMachine *sm = player->sm;
	gchar *playername;

	sm_state_name(sm, "mode_check_status");
	switch (event) {
	case SM_ENTER:
		player_send_uncached(player, FIRST_VERSION, LATEST_VERSION,
				     "status report\n");
		break;

	case SM_RECV:
		if (sm_recv(sm, "status newplayer")) {
			player_setup(player, -1, NULL, FALSE);
			start_tournament_mode(player);
			return TRUE;
		}
		if (sm_recv(sm, "status reconnect %S", &playername)) {
			/* if possible, try to revive the player */
			player_revive(player, playername);
			g_free(playername);
			start_tournament_mode(player);
			return TRUE;
		}
		if (sm_recv(sm, "status newviewer")) {
			player_setup(player, -1, NULL, TRUE);
			return TRUE;
		}
		if (sm_recv(sm, "status viewer %S", &playername)) {
			player_setup(player, -1, playername, TRUE);
			g_free(playername);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

/* Returns a GList* to player 0 */
GList *player_first_real(Game * game)
{
	GList *list;
	/* search for player 0 */
	playerlist_inc_use_count(game);
	for (list = game->player_list;
	     list != NULL; list = g_list_next(list)) {
		Player *player = list->data;
		if (player->num == 0)
			break;
	}
	playerlist_dec_use_count(game);
	return list;
}

/* Returns a GList * to a player with a number one higher than last */
GList *player_next_real(GList * last)
{
	Player *player;
	Game *game;
	guint numplayers;
	gint nextnum;
	GList *list;

	if (!last)
		return NULL;

	player = last->data;
	game = player->game;
	numplayers = game->params->num_players;
	nextnum = player->num + 1;

	if (nextnum >= (gint) numplayers)
		return NULL;

	playerlist_inc_use_count(game);
	for (list = game->player_list; list != NULL;
	     list = g_list_next(list)) {
		Player *scan = list->data;
		if (scan->num == nextnum)
			break;
	}
	playerlist_dec_use_count(game);
	return list;
}

static Player *player_by_name(Game * game, char *name)
{
	GList *list;

	playerlist_inc_use_count(game);
	for (list = game->player_list;
	     list != NULL; list = g_list_next(list)) {
		Player *player = list->data;

		if (player->name != NULL
		    && strcmp(player->name, name) == 0) {
			playerlist_dec_use_count(game);
			return player;
		}
	}
	playerlist_dec_use_count(game);

	return NULL;
}

Player *player_by_num(Game * game, gint num)
{
	GList *list;

	playerlist_inc_use_count(game);
	for (list = game->player_list;
	     list != NULL; list = g_list_next(list)) {
		Player *player = list->data;

		if (player->num == num) {
			playerlist_dec_use_count(game);
			return player;
		}
	}
	playerlist_dec_use_count(game);

	return NULL;
}

gboolean player_is_spectator(Game * game, gint player_num)
{
	return (gint) game->params->num_players <= player_num;
}

/* Returns a player that's not part of the game.
 */
Player *player_none(Game * game)
{
	static Player player;

	player.game = game;
	player.num = -1;
	player.disconnected = TRUE;
	return &player;
}

/** Broadcast a message to all players and spectators - prepend "player %d " to
 * all players except the one generating the message.
 * Also prepend 'extension' when this message is a protocol extension.
 *
 *  send to  PB_SILENT PB_RESPOND PB_ALL PB_OTHERS
 *  player      -           -       +        **
 *  other       -           +       +        +
 * ** = don't send to the player
 * +  = prepend 'player %d' to the message
 * -  = don't alter the message
 */
static void player_broadcast_internal(Player * player, BroadcastType type,
				      const gchar * message,
				      gboolean is_extension,
				      ClientVersionType
				      first_supported_version,
				      ClientVersionType
				      last_supported_version)
{
	Game *game = player->game;
	GList *list;

	playerlist_inc_use_count(game);
	for (list = game->player_list; list != NULL;
	     list = g_list_next(list)) {
		Player *scan = list->data;
		if ((scan->disconnected && !sm_get_use_cache(scan->sm))
		    || scan->num < 0
		    || scan->version < first_supported_version
		    || scan->version > last_supported_version)
			continue;
		if (type == PB_SILENT
		    || (scan == player && type == PB_RESPOND)) {
			if (is_extension) {
				player_send(scan, first_supported_version,
					    last_supported_version,
					    "extension %s", message);
			} else {
				player_send(scan, first_supported_version,
					    last_supported_version, "%s",
					    message);
			}
		} else if (scan != player || type == PB_ALL) {
			if (is_extension) {
				player_send(scan, first_supported_version,
					    last_supported_version,
					    "extension player %d %s",
					    player->num, message);
			} else {
				player_send(scan, first_supported_version,
					    last_supported_version,
					    "player %d %s", player->num,
					    message);
			}

		}
	}
	playerlist_dec_use_count(game);
}

/** As player_broadcast, but will add the 'extension' keyword */
void player_broadcast_extension(Player * player, BroadcastType type,
				ClientVersionType first_supported_version,
				ClientVersionType last_supported_version,
				const char *fmt, ...)
{
	gchar *buff;
	va_list ap;

	va_start(ap, fmt);
	buff = game_vprintf(fmt, ap);
	va_end(ap);

	player_broadcast_internal(player, type, buff, TRUE,
				  first_supported_version,
				  last_supported_version);
	g_free(buff);
}

/** Broadcast a message to all players and spectators */
void player_broadcast(Player * player, BroadcastType type,
		      ClientVersionType first_supported_version,
		      ClientVersionType last_supported_version,
		      const char *fmt, ...)
{
	gchar *buff;
	va_list ap;

	va_start(ap, fmt);
	buff = game_vprintf(fmt, ap);
	va_end(ap);

	player_broadcast_internal(player, type, buff, FALSE,
				  first_supported_version,
				  last_supported_version);
	g_free(buff);
}

/** Send a message to one player */
void player_send(Player * player,
		 ClientVersionType first_supported_version,
		 ClientVersionType last_supported_version, const char *fmt,
		 ...)
{
	gchar *buff;
	va_list ap;

	if (player->version < first_supported_version
	    || player->version > last_supported_version)
		return;

	va_start(ap, fmt);
	buff = game_vprintf(fmt, ap);
	va_end(ap);

	sm_write(player->sm, buff);
	g_free(buff);
}

/** Send a message to one player, even when caching is turned on */
void player_send_uncached(Player * player,
			  ClientVersionType first_supported_version,
			  ClientVersionType last_supported_version,
			  const char *fmt, ...)
{
	gchar *buff;
	va_list ap;

	if (player->version < first_supported_version
	    || player->version > last_supported_version)
		return;

	va_start(ap, fmt);
	buff = game_vprintf(fmt, ap);
	va_end(ap);

	sm_write_uncached(player->sm, buff);
	g_free(buff);
}

void player_set_name(Player * player, gchar * name)
{
	player_set_name_real(player, name, TRUE);
}

void player_remove(Player * player)
{
	driver->player_removed(player);
}

GList *list_from_player(Player * player)
{
	GList *list;
	for (list = player_first_real(player->game); list != NULL;
	     list = player_next_real(list))
		if (list->data == player)
			break;
	g_assert(list != NULL);
	return list;
}

GList *next_player_loop(GList * current, Player * first)
{
	current = player_next_real(current);
	if (current == NULL)
		current = player_first_real(first->game);
	if (current->data == first)
		return NULL;
	return current;
}

void playerlist_inc_use_count(Game * game)
{
	game->player_list_use_count++;
}

void playerlist_dec_use_count(Game * game)
{
	game->player_list_use_count--;
	if (game->player_list_use_count == 0) {
		GList *current;
		GList *all_dead_players;
		current = game->dead_players;
		all_dead_players = game->dead_players;	/* Remember this for g_list_free */
		game->dead_players = NULL;	/* Clear the list */
		for (; current != NULL; current = g_list_next(current)) {
			Player *p = current->data;
			player_free(p);
		}
		g_list_free(all_dead_players);
	}
}
