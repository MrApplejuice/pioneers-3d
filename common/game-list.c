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
#include <glib.h>
#include "game-list.h"
#include "log.h"
#include "network.h"

static GSList *_game_list = NULL;	/* The list of GameParams, ordered by title */

static gint sort_function(gconstpointer a, gconstpointer b)
{
	return (strcmp(((const GameParams *) a)->title,
		       ((const GameParams *) b)->title));
}

static gboolean game_list_add_item(GameParams * item)
{
	/* check for name collisions */
	if (item->title && game_list_find_item(item->title)) {

		gchar *nt;
		gint i;

		if (params_is_equal
		    (item, game_list_find_item(item->title))) {
			return FALSE;
		}

		/* append a number */
		for (i = 1; i <= INT_MAX; i++) {
			nt = g_strdup_printf("%s%d", item->title, i);
			if (!game_list_find_item(nt)) {
				g_free(item->title);
				item->title = nt;
				break;
			}
			g_free(nt);
		}
		/* give up and skip this game */
		if (item->title != nt) {
			g_free(nt);
			return FALSE;
		}
	}

	_game_list =
	    g_slist_insert_sorted(_game_list, item, sort_function);
	return TRUE;
}

/** Returns TRUE if the game list is empty */
gboolean game_list_is_empty(void)
{
	return _game_list == NULL;
}

static gint game_list_locate(gconstpointer param, gconstpointer argument)
{
	const GameParams *data = param;
	const gchar *title = argument;
	return strcmp(data->title, title);
}

const GameParams *game_list_find_item(const gchar * title)
{
	GSList *result;
	if (!_game_list) {
		return NULL;
	}

	result = g_slist_find_custom(_game_list, title, game_list_locate);
	if (result)
		return result->data;
	else
		return NULL;
}

void game_list_foreach(GFunc func, gpointer user_data)
{
	if (_game_list) {
		g_slist_foreach(_game_list, func, user_data);
	}
}

static void game_list_prepare_directory(const gchar * directory)
{
	GDir *dir;
	const gchar *fname;
	gchar *fullname;

	log_message(MSG_INFO, _("Looking for games in '%s'\n"), directory);
	if ((dir = g_dir_open(directory, 0, NULL)) == NULL) {
		log_message(MSG_INFO, _("Game directory '%s' not found\n"),
			    directory);
		return;
	}

	while ((fname = g_dir_read_name(dir))) {
		GameParams *params;
		size_t len = strlen(fname);

		if (len < 6 || strcmp(fname + len - 5, ".game") != 0)
			continue;
		fullname = g_build_filename(directory, fname, NULL);
		params = params_load_file(fullname);
		if (params) {
			if (!game_list_add_item(params))
				params_free(params);
		} else {
			log_message(MSG_ERROR,
				    _("Unable to load game: '%s'\n"),
				    fullname);
		}
		g_free(fullname);
	}
	g_dir_close(dir);
}

void game_list_prepare(void)
{
	gchar *directory;

	directory =
	    g_build_filename(g_get_user_data_dir(), "pioneers", NULL);
	game_list_prepare_directory(directory);
	g_free(directory);

	game_list_prepare_directory(get_pioneers_dir());

	if (game_list_is_empty())
		log_message(MSG_ERROR, _("No games available\n"));
}

void game_list_cleanup(void)
{
	GSList *games = _game_list;
	while (games) {
		params_free(games->data);
		games = g_slist_next(games);
	}
	g_slist_free(_game_list);
}
