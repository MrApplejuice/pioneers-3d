/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003,2006 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2011 Roland Clobus <rclobus@rclobus.nl>
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
#include "version.h"
#include "game.h"
#include "ai.h"
#include "client.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static char *server = NULL;
static char *port = NULL;
static char *name = NULL;
char *chromosomeFile = NULL;
static char *ai;
static int waittime = 1000;
static gboolean silent = FALSE;
static gboolean enable_debug = FALSE;
static gboolean show_version = FALSE;
static Map *map = NULL;

/** Randomizer only to be used for chat messages */
GRand *chat_rand;

/** Use any of the messages from the array to chat.
 * @param array An array for gchar * containing chat messages.
 * @param nochat_percent Chance (in percent) of not chatting
 */
#define randchat(array,nochat_percent)				\
	do {							\
		if (g_rand_int_range(chat_rand, 0, 101) > nochat_percent) { \
			ai_chat(array[g_rand_int_range(chat_rand, 0, G_N_ELEMENTS(array))]); \
		} \
	} while(0)

/** Avoid multiple chat messages when more than one other player
 * must discard resources */
gboolean discard_starting;


static void logbot_init(void);

static const struct algorithm_info {
	/** Name of the algorithm (for commandline) */
	const gchar *name;
	/** Init function */
	void (*init_func) (void);
	/** Request to be a player? */
	gboolean request_player;
	/** Quit if request not honoured */
	gboolean quit_if_not_request;
} algorithms[] = {
/* *INDENT-OFF* */
	{ "greedy", &greedy_init, TRUE, TRUE},
	{ "genetic", &genetic_init, TRUE, TRUE},
	{ "lobbybot", &lobbybot_init, FALSE, TRUE},
	{ "logbot", &logbot_init, FALSE, TRUE},
/* *INDENT-ON* */
};

static guint active_algorithm = 0;

UIDriver Glib_Driver;

static GOptionEntry commandline_entries[] = {
	{"chromosome-file", '\0', 0, G_OPTION_ARG_STRING, &chromosomeFile,
	 /* Commandline pioneersai: chromosome-file */
	 N_("Chromosome File"), NULL},
	{"server", 's', 0, G_OPTION_ARG_STRING, &server,
	 /* Commandline pioneersai: server */
	 N_("Server Host"), PIONEERS_DEFAULT_GAME_HOST},
	{"port", 'p', 0, G_OPTION_ARG_STRING, &port,
	 /* Commandline pioneersai: port */
	 N_("Server Port"), PIONEERS_DEFAULT_GAME_PORT},
	{"name", 'n', 0, G_OPTION_ARG_STRING, &name,
	 /* Commandline pioneersai: name */
	 N_("Computer name (mandatory)"), NULL},
	{"time", 't', 0, G_OPTION_ARG_INT, &waittime,
	 /* Commandline pioneersai: time */
	 N_("Time to wait between turns (in milliseconds)"), "1000"},
	{"chat-free", 'c', 0, G_OPTION_ARG_NONE, &silent,
	 /* Commandline pioneersai: chat-free */
	 N_("Stop computer player from talking"), NULL},
	{"algorithm", 'a', 0, G_OPTION_ARG_STRING, &ai,
	 /* Commandline pioneersai: algorithm */
	 N_("Type of computer player"), "greedy"},
	{"debug", '\0', 0, G_OPTION_ARG_NONE, &enable_debug,
	 /* Commandline option of ai: enable debug logging */
	 N_("Enable debug messages"), NULL},
	{"version", '\0', 0, G_OPTION_ARG_NONE, &show_version,
	 /* Commandline option of ai: version */
	 N_("Show version information"), NULL},
	{NULL, '\0', 0, 0, NULL, NULL, NULL}
};

static void ai_init_glib_et_al(int argc, char **argv)
{
	GOptionContext *context;
	GError *error = NULL;

	context =
	    /* Long description in the commandline for pioneersai: help */
	    g_option_context_new(_("- Computer player for Pioneers"));
	g_option_context_add_main_entries(context, commandline_entries,
					  PACKAGE);
	g_option_context_parse(context, &argc, &argv, &error);
	g_option_context_free(context);

	if (error != NULL) {
		g_print("%s\n", error->message);
		g_error_free(error);
		exit(1);
	}
	if (show_version) {
		g_print(_("Pioneers version:"));
		g_print(" ");
		g_print(FULL_VERSION);
		g_print("\n");
		exit(0);
	}

	g_type_init();
	set_ui_driver(&Glib_Driver);
	log_set_func_default();
}

static void ai_init(void)
{
	set_enable_debug(enable_debug);

	if (server == NULL)
		server = g_strdup(PIONEERS_DEFAULT_GAME_HOST);
	if (port == NULL)
		port = g_strdup(PIONEERS_DEFAULT_GAME_PORT);

	if (!name) {
		/* ai commandline error */
		g_print(_("A name must be provided.\n"));
		exit(0);
	}

	if (ai != NULL) {
		guint i;
		for (i = 0; i < G_N_ELEMENTS(algorithms); i++) {
			if (!strcmp(algorithms[i].name, ai))
				active_algorithm = i;
		}
	}
	log_message(MSG_INFO, _("Type of computer player: %s\n"),
		    algorithms[active_algorithm].name);
	algorithms[active_algorithm].init_func();
}

void ai_panic(const char *message)
{
	cb_chat(message);
	callbacks.quit();
}

static void ai_offline(void)
{
	gchar *style;

	callbacks.offline = callbacks.quit;
	notifying_string_set(requested_name, name);
	style =
	    g_strdup_printf("ai %s", algorithms[active_algorithm].name);
	notifying_string_set(requested_style, style);
	cb_connect(server, port,
		   !algorithms[active_algorithm].request_player);
	g_free(style);
	g_free(name);
}

static void ai_start_game(void)
{
	if (algorithms[active_algorithm].request_player ==
	    my_player_spectator()
	    && algorithms[active_algorithm].quit_if_not_request) {
		ai_panic(N_("The game is already full. I'm leaving."));
	}
}

void ai_wait(void)
{
	g_usleep((gulong) waittime * 1000u);
}

void ai_chat(const char *message)
{
	if (!silent)
		cb_chat(message);
}

static Map *ai_get_map(void)
{
	return map;
}

static void ai_set_map(Map * new_map)
{
	map = new_map;
}

/* Chat messages */
static const char *chat_turn_start[] = {
	/* AI chat at the start of the turn */
	N_("Ok, let's go!"),
	/* AI chat at the start of the turn */
	N_("I'll beat you all now! ;)"),
	/* AI chat at the start of the turn */
	N_("Now for another try..."),
};

static const char *chat_receive_one[] = {
	/* AI chat when one resource is received */
	N_("At least I get something..."),
	/* AI chat when one resource is received */
	N_("One is better than none..."),
};

static const char *chat_receive_many[] = {
	/* AI chat when more than one resource is received */
	N_("Wow!"),
	/* AI chat when more than one resource is received */
	N_("Ey, I'm becoming rich ;)"),
	/* AI chat when more than one resource is received */
	N_("This is really a good year!"),
};

static const char *chat_other_receive_many[] = {
	/* AI chat when other players receive more than one resource */
	N_("You really don't deserve that much!"),
	/* AI chat when other players receive more than one resource */
	N_("You don't know what to do with that many resources ;)"),
	/* AI chat when other players receive more than one resource */
	N_("Ey, wait for my robber and lose all this again!"),
};

static const char *chat_self_moved_robber[] = {
	/* AI chat when it moves the robber */
	N_("Hehe!"),
	/* AI chat when it moves the robber */
	N_("Go, robber, go!"),
};

static const char *chat_moved_robber_to_me[] = {
	/* AI chat when the robber is moved to it */
	N_("You bastard!"),
	/* AI chat when the robber is moved to it */
	N_("Can't you move that robber somewhere else?!"),
	/* AI chat when the robber is moved to it */
	N_("Why always me??"),
};

static const char *chat_discard_self[] = {
	/* AI chat when it must discard resources */
	N_("Oh no!"),
	/* AI chat when it must discard resources */
	N_("Grrr!"),
	/* AI chat when it must discard resources */
	N_("Who the hell rolled that 7??"),
	/* AI chat when it must discard resources */
	N_("Why always me?!?"),
};

static const char *chat_discard_other[] = {
	/* AI chat when other players must discard */
	N_("Say good bye to your cards... :)"),
	/* AI chat when other players must discard */
	N_("*evilgrin*"),
	/* AI chat when other players must discard */
	N_("/me says farewell to your cards ;)"),
	/* AI chat when other players must discard */
	N_("That's the price for being rich... :)"),
};

static const char *chat_stole_from_me[] = {
	/* AI chat when someone steals from it */
	N_("Ey! Where's that card gone?"),
	/* AI chat when someone steals from it */
	N_("Thieves! Thieves!!"),
	/* AI chat when someone steals from it */
	N_("Wait for my revenge..."),
};

static const char *chat_monopoly_other[] = {
	/* AI chat when someone plays the monopoly card */
	N_("Oh no :("),
	/* AI chat when someone plays the monopoly card */
	N_("Must this happen NOW??"),
	/* AI chat when someone plays the monopoly card */
	N_("Args"),
};

static const char *chat_largestarmy_self[] = {
	/* AI chat when it has the largest army */
	N_("Hehe, my soldiers rule!"),
};

static const char *chat_largestarmy_other[] = {
	/* AI chat when another player that the largest army */
	N_("First robbing us, then grabbing the points..."),
};

static const char *chat_longestroad_self[] = {
	/* AI chat when it has the longest road */
	N_("See that road!"),
};

static const char *chat_longestroad_other[] = {
	/* AI chat when another player has the longest road */
	N_("Pf, you won't win with roads alone..."),
};

/* functions for chatting follow */
static void chat_discard_start(void)
{
	discard_starting = TRUE;
}

void ai_chat_discard(gint player_num, G_GNUC_UNUSED gint discard_num)
{
	if (player_num == my_player_num()) {
		randchat(chat_discard_self, 10);
	} else {
		if (discard_starting) {
			discard_starting = FALSE;
			randchat(chat_discard_other, 10);
		}
	}
}

static void chat_player_turn(gint player)
{
	if (player == my_player_num())
		randchat(chat_turn_start, 70);
}

void ai_chat_self_moved_robber(void)
{
	randchat(chat_self_moved_robber, 15);
}

static void chat_robber_moved(G_GNUC_UNUSED Hex * old, Hex * new)
{
	guint idx;
	gboolean iam_affected = FALSE;
	for (idx = 0; idx < G_N_ELEMENTS(new->nodes); idx++) {
		if (new->nodes[idx]->owner == my_player_num())
			iam_affected = TRUE;
	}
	if (iam_affected)
		randchat(chat_moved_robber_to_me, 20);
}

static void chat_player_robbed(G_GNUC_UNUSED gint robber_num,
			       gint victim_num,
			       G_GNUC_UNUSED Resource resource)
{
	if (victim_num == my_player_num())
		randchat(chat_stole_from_me, 15);
}

static void chat_get_rolled_resources(gint player_num,
				      const gint * resources,
				      G_GNUC_UNUSED const gint * wanted)
{
	gint total = 0, i;
	for (i = 0; i < NO_RESOURCE; ++i)
		total += resources[i];
	if (player_num == my_player_num()) {
		if (total == 1)
			randchat(chat_receive_one, 60);
		else if (total >= 3)
			randchat(chat_receive_many, 20);
	} else if (total >= 3)
		randchat(chat_other_receive_many, 30);
}

static void chat_played_develop(gint player_num,
				G_GNUC_UNUSED guint card_idx,
				DevelType type)
{
	if (player_num != my_player_num() && type == DEVEL_MONOPOLY)
		randchat(chat_monopoly_other, 20);
}

static void chat_new_statistics(gint player_num, StatisticType type,
				gint num)
{
	if (num != 1)
		return;
	if (type == STAT_LONGEST_ROAD) {
		if (player_num == my_player_num())
			randchat(chat_longestroad_self, 10);
		else
			randchat(chat_longestroad_other, 10);
	} else if (type == STAT_LARGEST_ARMY) {
		if (player_num == my_player_num())
			randchat(chat_largestarmy_self, 10);
		else
			randchat(chat_largestarmy_other, 10);
	}
}

static void ai_error(const gchar * message)
{
	gchar *buffer;

	buffer =
	    g_strdup_printf(_(""
			      "Received error from server: %s.  Quitting\n"),
			    message);
	cb_chat(buffer);
	g_free(buffer);
	cb_disconnect();
}

static void ai_game_over(gint player_num, G_GNUC_UNUSED gint points)
{
	if (player_num == my_player_num()) {
		/* AI chat when it wins */
		ai_chat(N_("Yippie!"));
	} else {
		/* AI chat when another player wins */
		ai_chat(N_("My congratulations"));
	}
	cb_disconnect();
}

void frontend_set_callbacks(void)
{
	callbacks.init_glib_et_al = &ai_init_glib_et_al;
	callbacks.init = &ai_init;
	callbacks.offline = &ai_offline;
	callbacks.start_game = &ai_start_game;
	callbacks.get_map = &ai_get_map;
	callbacks.set_map = &ai_set_map;
	callbacks.error = &ai_error;
	callbacks.game_over = &ai_game_over;

	/* chatting */
	callbacks.player_turn = &chat_player_turn;
	callbacks.robber_moved = &chat_robber_moved;
	callbacks.discard = &chat_discard_start;
	callbacks.player_robbed = &chat_player_robbed;
	callbacks.get_rolled_resources = &chat_get_rolled_resources;
	callbacks.played_develop = &chat_played_develop;
	callbacks.new_statistics = &chat_new_statistics;

	chat_rand = g_rand_new();
}

/* The logbot is intended to be used as a spectator in a game, and to collect
 * a transcript of the game in human readable form, which can be analysed
 * using external tools. */
void logbot_init(void)
{
}
