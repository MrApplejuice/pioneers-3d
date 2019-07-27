/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2007, 2013 Roland Clobus <rclobus@rclobus.nl>
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

/* Pioneers Console Server Administrator interface
 *
 * The strings in the admin interface are intentionally not translated.
 * They would otherwise reflect the language of the server that is
 * running the server, instead of the language of the connecting user.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>

#include "admin.h"
#include "game.h"
#include "server.h"
#include "network.h"
#include "version.h"

/* network administration functions */
static Game **admin_game;
static gint admin_dice_roll = 0;
static gchar *server_port = NULL;
static gboolean register_server = TRUE;
static GameParams *params = NULL;
static Service *service = NULL;

typedef enum {
	BADCOMMAND,
	SETPORT,
	STARTSERVER,
	STOPSERVER,
	REGISTERSERVER,
	NUMPLAYERS,
	SEVENSRULE,
	DICEDECK,
	NUMDICEDECKS,
	NUMREMOVEDDICECARDS,
	VICTORYPOINTS,
	RANDOMTERRAIN,
	SETGAME,
	QUIT,
	MESSAGE,
	HELP,
	INFO,
	FIXDICE,
	GETBANK,
	SETBANK,
	GETASSETS,
	SETASSETS
} AdminCommandType;

typedef enum {
	NONEED,
	NEEDPARAMS,
	NEEDGAME
} AdminRequirementType;

typedef struct {
	AdminCommandType type;
	const gchar *command;
	gboolean need_argument;
	gboolean stop_server;
	AdminRequirementType requirement;
} AdminCommand;

/* *INDENT-OFF* */
static AdminCommand admin_commands[] = {
	{ BADCOMMAND,          "",                    FALSE, FALSE, NONEED     },
	{ SETPORT,             "set-port",            TRUE,  TRUE,  NEEDPARAMS },
	{ STARTSERVER,         "start-server",        FALSE, TRUE,  NEEDPARAMS },
	{ STOPSERVER,          "stop-server",         FALSE, TRUE,  NONEED },
	{ REGISTERSERVER,      "set-register-server", TRUE,  TRUE,  NONEED },
	{ NUMPLAYERS,          "set-num-players",     TRUE,  TRUE,  NEEDPARAMS },
	{ SEVENSRULE,          "set-sevens-rule",     TRUE,  TRUE,  NEEDPARAMS },
	{ DICEDECK,            "set-dice-deck",       TRUE,  TRUE,  NEEDPARAMS },
	{ NUMDICEDECKS,        "set-num-dice-decks",  TRUE,  TRUE,  NEEDPARAMS },
	{ NUMREMOVEDDICECARDS, "set-num-removed-dice-cards",
	                                              TRUE,  TRUE,  NEEDPARAMS },

	{ VICTORYPOINTS,       "set-victory-points",  TRUE,  TRUE,  NEEDPARAMS },
	{ RANDOMTERRAIN,       "set-random-terrain",  TRUE,  TRUE,  NEEDPARAMS },
	{ SETGAME,             "set-game",            TRUE,  TRUE,  NONEED     },
	{ QUIT,                "quit",                FALSE, FALSE, NONEED     },
	{ MESSAGE,             "send-message",        TRUE,  FALSE, NEEDGAME   },
	{ HELP,                "help",                FALSE, FALSE, NONEED     },
	{ INFO,                "info",                FALSE, FALSE, NONEED     },
	{ FIXDICE,             "fix-dice",            TRUE,  FALSE, NEEDGAME   },
	{ GETBANK,             "get-bank",            FALSE, FALSE, NEEDGAME   },
	{ SETBANK,             "set-bank",            TRUE,  FALSE, NEEDGAME   },
	{ GETASSETS,           "get-assets",          TRUE,  FALSE, NEEDGAME   },
	{ SETASSETS,           "set-assets",          TRUE,  FALSE, NEEDGAME   },
};
/* *INDENT-ON* */

/* parse 'line' and run the command requested */
static void admin_run_command(Session * admin_session, const gchar * line)
{
	const gchar *command_start;
	gchar *command;
	gchar *argument;
	guint command_number;

	if (!g_str_has_prefix(line, "admin")) {
		net_printf(admin_session,
			   "no admin prefix in command: '%s'\n", line);
		return;
	}

	line += 5;		/* length of "admin" */
	while (*line && g_ascii_isspace(*line))
		++line;
	if (!*line) {
		net_printf(admin_session, "no command found: '%s'\n",
			   line);
		return;
	}

	/* parse the line down into command and argument */
	command_start = line;
	while (*line && !g_ascii_isspace(*line))
		++line;
	command = g_strndup(command_start, (gsize) (line - command_start));

	if (*line) {
		while (*line && g_ascii_isspace(*line))
			++line;
		argument = g_strdup(line);
	} else {
		argument = NULL;
	}

	/* command[0] is the fall-back */
	for (command_number = 1;
	     command_number < G_N_ELEMENTS(admin_commands);
	     ++command_number) {
		if (!strcmp
		    (command, admin_commands[command_number].command)) {
			break;
		}
	}
	if (command_number == G_N_ELEMENTS(admin_commands)) {
		command_number = 0;
	}
	if (admin_commands[command_number].need_argument
	    && NULL == argument) {
		net_printf(admin_session,
			   "ERROR command '%s' needs an argument\n",
			   command);
	} else if (admin_commands[command_number].requirement == NEEDPARAMS
		   && NULL == params) {
		net_printf(admin_session,
			   "ERROR command '%s' needs valid game parameters\n",
			   command);
	} else if (admin_commands[command_number].requirement == NEEDGAME
		   && (NULL == *admin_game
		       || !server_is_running(*admin_game))) {
		net_printf(admin_session,
			   "ERROR command '%s' needs a valid game\n",
			   command);
	} else {
		if (admin_commands[command_number].stop_server
		    && server_is_running(*admin_game)) {
			server_stop(*admin_game);
			game_free(*admin_game);
			*admin_game = NULL;
			net_write(admin_session, "INFO server stopped\n");
		}
		switch (admin_commands[command_number].type) {
		case BADCOMMAND:
			net_printf(admin_session,
				   "ERROR unrecognized command: '%s'\n",
				   command);
			break;
		case SETPORT:
			if (server_port)
				g_free(server_port);
			server_port = g_strdup(argument);
			break;
		case STARTSERVER:
			{
				gchar *metaserver_name =
				    get_metaserver_name(TRUE);
				if (!server_port)
					server_port =
					    g_strdup
					    (PIONEERS_DEFAULT_GAME_PORT);
				if (*admin_game != NULL)
					game_free(*admin_game);
				*admin_game =
				    server_start(params, get_server_name(),
						 server_port,
						 register_server,
						 metaserver_name, TRUE);
				g_free(metaserver_name);
			}
			break;
		case STOPSERVER:
			server_stop(*admin_game);
			break;
		case REGISTERSERVER:
			register_server = atoi(argument);
			break;
		case NUMPLAYERS:
			cfg_set_num_players(params, atoi(argument));
			break;
		case SEVENSRULE:
			cfg_set_sevens_rule(params, atoi(argument));
			break;
		case DICEDECK:
			cfg_set_use_dice_deck(params, atoi(argument));
			break;
		case NUMDICEDECKS:
			cfg_set_num_dice_decks(params, atoi(argument));
			break;
		case NUMREMOVEDDICECARDS:
			cfg_set_num_removed_dice_cards(params,
						       atoi(argument));
			break;
		case VICTORYPOINTS:
			cfg_set_victory_points(params, atoi(argument));
			break;
		case RANDOMTERRAIN:
			cfg_set_terrain_type(params, atoi(argument));
			break;
		case SETGAME:
			if (params)
				params_free(params);
			params = cfg_set_game(argument);
			if (!params) {
				net_printf(admin_session,
					   "ERROR game '%s' not set\n",
					   argument);
			}
			break;
		case QUIT:
			net_close(admin_session);
			/* Quit the server if the admin leaves */
			if (!server_is_running(*admin_game))
				exit(0);
			break;
		case MESSAGE:
			g_strdelimit(argument, "|", '_');
			if (server_is_running(*admin_game))
				admin_broadcast(*admin_game, argument);
			break;
		case HELP:
			for (command_number = 1;
			     command_number < G_N_ELEMENTS(admin_commands);
			     ++command_number) {
				if (admin_commands
				    [command_number].need_argument) {
					net_printf(admin_session,
						   "INFO %s argument\n",
						   admin_commands
						   [command_number].
						   command);
				} else {
					net_printf(admin_session,
						   "INFO %s\n",
						   admin_commands
						   [command_number].
						   command);
				}
			}
			break;
		case INFO:
			net_printf(admin_session, "INFO server-port %s\n",
				   server_port ? server_port :
				   PIONEERS_DEFAULT_GAME_PORT);
			net_printf(admin_session,
				   "INFO register-server %d\n",
				   register_server);
			net_printf(admin_session,
				   "INFO server running %d\n",
				   server_is_running(*admin_game));
			if (params) {
				net_printf(admin_session, "INFO game %s\n",
					   params->title);
				net_printf(admin_session,
					   "INFO players %d\n",
					   params->num_players);
				net_printf(admin_session,
					   "INFO victory-points %d\n",
					   params->victory_points);
				net_printf(admin_session,
					   "INFO random-terrain %d\n",
					   params->random_terrain);
				net_printf(admin_session,
					   "INFO sevens-rule %d\n",
					   params->sevens_rule);
				if (server_is_running(*admin_game)) {
					gchar *s =
					    game_printf("INFO bank %R\n",
							(*admin_game)->bank_deck);
					net_printf(admin_session, "%s", s);
					g_free(s);

					playerlist_inc_use_count
					    (*admin_game);
					GList *player =
					    player_first_real(*admin_game);
					while (player) {
						Player *p = (Player *)
						    player->data;
						if (player_is_spectator
						    (*admin_game,
						     p->num)) {
							s = game_printf
							    ("INFO spectator %d\n",
							     p->num);
						} else {
							s = game_printf
							    ("INFO player %d assets %R\n",
							     p->num,
							     p->assets);
						}
						net_printf(admin_session,
							   "%s", s);
						g_free(s);
						player =
						    player_next_real
						    (player);
					}
					playerlist_dec_use_count
					    (*admin_game);
				}
			} else {
				net_printf(admin_session,
					   "INFO no game set\n");
			}
			if (admin_dice_roll != 0)
				net_printf(admin_session,
					   "INFO dice fixed to %d\n",
					   admin_dice_roll);
			break;
		case FIXDICE:
			admin_dice_roll = CLAMP(atoi(argument), 0, 12);
			if (admin_dice_roll == 1)
				admin_dice_roll = 0;
			if (admin_dice_roll != 0) {
				(*admin_game)->is_manipulated = TRUE;
				net_printf(admin_session,
					   "INFO dice fixed to %d\n",
					   admin_dice_roll);
			} else
				net_printf(admin_session,
					   "INFO dice rolled normally\n");
			break;
		case SETBANK:
			{
				game_scanf(argument, "%R",
					   &(*admin_game)->bank_deck);
				(*admin_game)->is_manipulated = TRUE;
			}
			// FALL THROUGH
		case GETBANK:
			{
				gchar *s = game_printf("INFO bank %R\n",
						       (*admin_game)->bank_deck);
				net_printf(admin_session, "%s", s);
				g_free(s);
			}
			break;
		case SETASSETS:
			{
				gint player_num;
				gint assets[NO_RESOURCE];
				Player *player;
				gint i;

				game_scanf(argument, "%d %R", &player_num,
					   &assets);
				player =
				    player_by_num(*admin_game, player_num);
				if (player != NULL
				    && !player_is_spectator(*admin_game,
							    player_num)) {
					for (i = 0; i < NO_RESOURCE; i++) {
						(*admin_game)->bank_deck[i]
						    +=
						    player->assets[i] -
						    assets[i];
						player->assets[i] =
						    assets[i];
					}
				}
				(*admin_game)->is_manipulated = TRUE;
			}
			// FALL THROUGH
		case GETASSETS:
			{
				gint player_num;
				Player *player;
				game_scanf(argument, "%d", &player_num);
				player =
				    player_by_num(*admin_game, player_num);
				if (player != NULL) {
					if (player_is_spectator
					    (*admin_game, player_num)) {
						net_printf(admin_session,
							   "INFO player %d is spectator\n",
							   player_num);
					} else {
						gchar *s =
						    game_printf
						    ("INFO player %d assets %R\n",
						     player_num,
						     player->assets);
						net_printf(admin_session,
							   "%s", s);
						g_free(s);
					}
				} else {
					net_printf(admin_session,
						   "INFO player %d not found\n",
						   player_num);
				}

			}
			break;
		}
	}
	g_free(command);
	if (argument)
		g_free(argument);
}

/* network event handler */
static void admin_event(Session * ses, NetEvent event, const gchar * line,
			G_GNUC_UNUSED gpointer user_data)
{
#ifdef PRINT_INFO
	g_print
	    ("admin_event: event = %#x, admin_session = %p, line = %s\n",
	     event, ses, line);
#endif

	switch (event) {
	case NET_READ:
		/* there is data to be read */

#ifdef PRINT_INFO
		g_print("admin_event: NET_READ: line = '%s'\n", line);
#endif
		admin_run_command(ses, line);
		break;
	case NET_CLOSE:
		/* connection has been closed */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CLOSE\n");
#endif
		net_free(&ses);
		break;
	case NET_CONNECT:
		/* new connection was made */
		net_printf(ses,
			   "welcome to the pioneers admin connection, "
			   "version %s\n", FULL_VERSION);
		net_set_check_connection_alive(ses, 0);
#ifdef PRINT_INFO
		g_print("admin_event: NET_CONNECT\n");
#endif
		break;
	case NET_CONNECT_FAIL:
		/* connect() failed -- shouldn't get here */

#ifdef PRINT_INFO
		g_print("admin_event: NET_CONNECT_FAIL\n");
#endif
		net_free(&ses);
		break;
	}
}

/* set up the administration port */
gboolean admin_init(const gchar * port, Game ** game)
{
	gchar *error_message;

	admin_game = game;
	if (*admin_game != NULL) {
		params = params_copy((*admin_game)->params);
		server_port = g_strdup((*admin_game)->server_port);
	}

	service =
	    net_service_new(atoi(port), admin_event, NULL, &error_message);

	if (!service) {
		log_message(MSG_ERROR, "%s\n", error_message);
		g_free(error_message);
		return FALSE;
	}
	return TRUE;
}

gint admin_get_dice_roll(void)
{
	return admin_dice_roll;
}
