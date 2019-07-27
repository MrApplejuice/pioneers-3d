/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2006 Roland Clobus <rclobus@bigfoot.com>
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
#include "ai.h"
#include "cost.h"
#include <stdio.h>
#include <string.h>
/*
 * This is a chatty AI for Pioneers.
 *
 * It is intended to populate the lobby and to give help to new players.
 * When used in other games, it will leave the game when it starts.
*/

static GHashTable *players = NULL;
static gboolean chatting = FALSE;

struct _PlayerInfo {
	/** Name of the player */
	gchar *name;
};

typedef struct _PlayerInfo PlayerInfo;

static PlayerInfo *player_info_new(const gchar * name)
{
	PlayerInfo *object = g_malloc(sizeof(PlayerInfo));
	object->name = g_strdup(name);
	return object;
}

static void player_info_free(PlayerInfo * player_info)
{
	g_return_if_fail(player_info != NULL);
	g_free(player_info->name);
	g_free(player_info);
}

static void player_info_set_name(PlayerInfo * player_info,
				 const gchar * name)
{
	g_return_if_fail(player_info != NULL);
	if (player_info->name != NULL)
		g_free(player_info->name);
	player_info->name = g_strdup(name);
}

static void lobbybot_player_name_changed(gint player_num,
					 const gchar * name)
{
	PlayerInfo *info =
	    g_hash_table_lookup(players, GINT_TO_POINTER(player_num));
	if (info) {
		player_info_set_name(info, name);
	} else {
		info = player_info_new(name);
		g_hash_table_insert(players, GINT_TO_POINTER(player_num),
				    info);

		if (my_player_num() != player_num && chatting)
			/* Translators: don't translate '/help' */
			ai_chat(N_("Hello, welcome to the lobby. I am a "
				   "simple robot. Type '/help' in the chat "
				   "to see the list of commands I know."));
	}
}

static void lobbybot_player_quit(gint player_num)
{
	gboolean did_remove =
	    g_hash_table_remove(players, GINT_TO_POINTER(player_num));
	g_return_if_fail(did_remove);
}

static void lobbybot_chat_parser(gint player_num, const gchar * chat)
{
	PlayerInfo *info;

	if (player_num == my_player_num()) {
		/* Don't log own responses */
		return;
	}

	info = g_hash_table_lookup(players, GINT_TO_POINTER(player_num));
	g_assert(info != NULL);

	if (!strncmp(chat, "/help", 5)) {
		/* Translators: don't translate '/help' */
		ai_chat(N_("'/help' shows this message again"));

		/* Translators: don't translate '/why' */
		ai_chat(N_("'/why' explains the purpose of this strange "
			   "board layout"));
		/* Translators: don't translate '/news' */
		ai_chat(N_("'/news' tells the last released version"));
		return;
	}

	if (!strncmp(chat, "/why", 4)) {
		/* AI chat that explains '/why' */
		ai_chat(N_("This board is not intended to be a game that "
			   "can be played. Instead, players can find "
			   "eachother here, and decide which board they "
			   "want to play. Then, one of the players will "
			   "host the proposed game by starting a server, "
			   "and registers it at the metaserver. The other "
			   "players can subsequently disconnect from the "
			   "lobby, and enter that game."));
		return;
	}
	if (!strncmp(chat, "/news", 5)) {
		ai_chat(N_("The last released version of Pioneers is"));
		/* Update this string when releasing a new version */
		ai_chat("15.5");
		return;
	}
}

static void hash_data_free(gpointer data)
{
	player_info_free((PlayerInfo *) data);
}

static void lobbybot_turn(G_GNUC_UNUSED gint player_num)
{
	ai_chat(
		       /* The lobbybot leaves when a game is starting */
		       N_(""
			  "The game is starting. I'm not needed anymore. Goodbye."));
	cb_disconnect();
}

static void lobbybot_start_game(void)
{
	/* The rules are known, enable chat */
	chatting = TRUE;
}

void lobbybot_init(void)
{
	g_assert(players == NULL);
	players =
	    g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
				  hash_data_free);
	/* Don't chat before the rules are known */
	chatting = FALSE;

	callbacks.spectator_name = &lobbybot_player_name_changed;
	callbacks.player_name = &lobbybot_player_name_changed;
	callbacks.spectator_quit = &lobbybot_player_quit;
	callbacks.player_quit = &lobbybot_player_quit;
	callbacks.incoming_chat = &lobbybot_chat_parser;
	callbacks.player_turn = &lobbybot_turn;
	callbacks.start_game = &lobbybot_start_game;
}
