/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
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
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <glib.h>
#include <stdio.h>

#include "game.h"
#include "cards.h"
#include "log.h"

/* The macro is a modification of the macro in the glib source
 * The modification allows for const pointers
 */
#ifndef G_STRUCT_MEMBER_CONST
#define G_STRUCT_MEMBER_CONST(member_type, struct_p, struct_offset)   \
    (*(member_type const *) ((gconstpointer) ((const guint8*) (struct_p) + (glong) (struct_offset))))
#endif

const gchar *default_player_style = "square";

typedef enum {
	PARAM_SINGLE_LINE,
	PARAM_MULTIPLE_LINES,
	PARAM_INT,
	PARAM_BOOL,
	PARAM_INTLIST,
	PARAM_OBSOLETE_DATA
} ParamType;

typedef struct {
	const gchar *name; /**< Text version of the parameter */
	ClientVersionType first_version; /**< First version with this parameter */
	ParamType type;	/**< Data type */
	int offset; /**< Offset in the struct */
} Param;

#define PARAM(name, first, type, var) #name, first, type, G_STRUCT_OFFSET(GameParams, var)
#define PARAM_OBSOLETE(var) #var, UNKNOWN_VERSION, PARAM_OBSOLETE_DATA, -1

/* *INDENT-OFF* */
static Param game_params[] = {
	{PARAM(title, FIRST_VERSION, PARAM_SINGLE_LINE, title)},
	{PARAM_OBSOLETE(variant)},
	{PARAM(random-terrain, FIRST_VERSION, PARAM_BOOL, random_terrain)},
	{PARAM(strict-trade, FIRST_VERSION, PARAM_BOOL, strict_trade)},
	{PARAM(domestic-trade, FIRST_VERSION, PARAM_BOOL, domestic_trade)},
	{PARAM(num-players, FIRST_VERSION, PARAM_INT, num_players)},
	{PARAM(sevens-rule, FIRST_VERSION, PARAM_INT, sevens_rule)},
	{PARAM(use-dice-deck, V15, PARAM_BOOL, use_dice_deck)},
	{PARAM(num-dice-decks, V15, PARAM_INT, num_dice_decks)},
	{PARAM(num-removed-dice-cards, V15, PARAM_INT, num_removed_dice_cards)},
	{PARAM(victory-points, FIRST_VERSION, PARAM_INT, victory_points)},
	{PARAM(check-victory-at-end-of-turn, FIRST_VERSION, PARAM_BOOL, check_victory_at_end_of_turn)},
	{PARAM(num-roads, FIRST_VERSION, PARAM_INT, num_build_type[BUILD_ROAD])},
	{PARAM(num-bridges, FIRST_VERSION, PARAM_INT, num_build_type[BUILD_BRIDGE])},
	{PARAM(num-ships, FIRST_VERSION, PARAM_INT, num_build_type[BUILD_SHIP])},
	{PARAM(num-settlements, FIRST_VERSION, PARAM_INT, num_build_type[BUILD_SETTLEMENT])},
	{PARAM(num-cities, FIRST_VERSION, PARAM_INT, num_build_type[BUILD_CITY])},
	{PARAM(num-city-walls, V0_11, PARAM_INT, num_build_type[BUILD_CITY_WALL])},
	{PARAM(resource-count, FIRST_VERSION, PARAM_INT, resource_count)},
	{PARAM(develop-road, FIRST_VERSION, PARAM_INT, num_develop_type[DEVEL_ROAD_BUILDING])},
	{PARAM(develop-monopoly, FIRST_VERSION, PARAM_INT, num_develop_type[DEVEL_MONOPOLY])},
	{PARAM(develop-plenty, FIRST_VERSION, PARAM_INT, num_develop_type[DEVEL_YEAR_OF_PLENTY])},
	{PARAM(develop-chapel, FIRST_VERSION, PARAM_INT, num_develop_type[DEVEL_CHAPEL])},
	{PARAM(develop-university, FIRST_VERSION, PARAM_INT, num_develop_type[DEVEL_UNIVERSITY])},
	{PARAM(develop-governor, FIRST_VERSION, PARAM_INT, num_develop_type[DEVEL_GOVERNORS_HOUSE])},
	{PARAM(develop-library, FIRST_VERSION, PARAM_INT, num_develop_type[DEVEL_LIBRARY])},
	{PARAM(develop-market, FIRST_VERSION, PARAM_INT, num_develop_type[DEVEL_MARKET])},
	{PARAM(develop-soldier, FIRST_VERSION, PARAM_INT, num_develop_type[DEVEL_SOLDIER])},
	{PARAM(use-pirate, FIRST_VERSION, PARAM_BOOL, use_pirate)},
	{PARAM(island-discovery-bonus, FIRST_VERSION, PARAM_INTLIST, island_discovery_bonus)},
	{PARAM(#, FIRST_VERSION, PARAM_MULTIPLE_LINES, comments)},
	{PARAM(desc, V14, PARAM_MULTIPLE_LINES, description)},
};
/* *INDENT-ON* */

GameParams *params_new(void)
{
	GameParams *params;

	params = g_malloc0(sizeof(*params));
	params->num_dice_decks = 2;
	params->num_removed_dice_cards = 5;

	return params;
}

void params_free(GameParams * params)
{
	guint idx;
	gchar *str;
	GArray *int_list;

	if (params == NULL)
		return;

	for (idx = 0; idx < G_N_ELEMENTS(game_params); idx++) {
		Param *param = game_params + idx;

		switch (param->type) {
		case PARAM_SINGLE_LINE:
		case PARAM_MULTIPLE_LINES:
			str =
			    G_STRUCT_MEMBER(gchar *, params,
					    param->offset);
			g_free(str);
			break;
		case PARAM_INT:
		case PARAM_BOOL:
			break;
		case PARAM_INTLIST:
			int_list = G_STRUCT_MEMBER(GArray *, params,
						   param->offset);
			if (int_list != NULL)
				g_array_free(int_list, TRUE);
			break;
		case PARAM_OBSOLETE_DATA:
			/* Obsolete rule: do nothing */
			break;
		}
	}
	map_free(params->map);
	g_free(params);
}

static const gchar *skip_space(const gchar * str)
{
	while (isspace(*str))
		str++;
	return str;
}

static gboolean match_word(const gchar ** str, const gchar * word)
{
	size_t word_len;

	word_len = strlen(word);
	if (strncmp(*str, word, word_len) == 0) {
		*str += word_len;
		*str = skip_space(*str);
		return TRUE;
	}
	return FALSE;
}

GArray *build_int_list(const gchar * str)
{
	GArray *array = g_array_new(FALSE, FALSE, sizeof(gint));
	while (*str != '\0') {
		gint num;
		gint sign = +1;

		/* Skip leading space */
		while (isspace(*str))
			str++;
		if (*str == '\0')
			break;
		/* Get the next number and add it to the array */
		num = 0;
		if (*str == '-') {
			sign = -1;
			str++;
		}
		while (isdigit(*str))
			num = num * 10 + *str++ - '0';
		num *= sign;
		g_array_append_val(array, num);

		/* Skip the non-digits */
		while (!isdigit(*str) && *str != '-' && *str != '\0')
			str++;
	}
	if (array->len == 0) {
		g_array_free(array, FALSE);
		array = NULL;
	}
	return array;
}

gchar *format_int_list(const gchar * name, GArray * array)
{
	gchar *old;
	gchar *str;
	guint idx;

	if (array == NULL)
		return NULL;

	if (array->len == 0) {
		return g_strdup(name);
	}
	if (name == NULL || strlen(name) == 0) {
		str = g_strdup("");
	} else {
		str = g_strdup_printf("%s ", name);
	};
	for (idx = 0; idx < array->len; idx++) {
		old = str;
		if (idx == 0)
			str =
			    g_strdup_printf("%s%d", str,
					    g_array_index(array, gint,
							  idx));
		else
			str = g_strdup_printf("%s,%d", str,
					      g_array_index(array, gint,
							    idx));
		g_free(old);
	}
	return str;
}

struct nosetup_t {
	WriteLineFunc func;
	gpointer user_data;
};

static gboolean find_no_setup(const Hex * hex, gpointer closure)
{
	guint idx;
	struct nosetup_t *data = closure;
	for (idx = 0; idx < G_N_ELEMENTS(hex->nodes); ++idx) {
		const Node *node = hex->nodes[idx];
		if (node->no_setup) {
			gchar buff[50];
			if (node->x != hex->x || node->y != hex->y)
				continue;
			snprintf(buff, sizeof(buff), "nosetup %d %d %d",
				 node->x, node->y, node->pos);
			data->func(data->user_data, buff);
		}
	}
	return FALSE;
}

void params_write_lines(const GameParams * params,
			ClientVersionType version, gboolean write_secrets,
			WriteLineFunc func, gpointer user_data)
{
	guint idx;
	gint y;
	gchar *buff;
	const gchar *str;

	for (idx = 0; idx < G_N_ELEMENTS(game_params); idx++) {
		Param *param = game_params + idx;

		if (param->first_version > version) {
			/* This rule is too new for the recipient */
			/* Only notify the recipient when the rule is not in use */

			switch (param->type) {
			case PARAM_SINGLE_LINE:
			case PARAM_MULTIPLE_LINES:
				str =
				    G_STRUCT_MEMBER_CONST(gchar *, params,
							  param->offset);
				if (!str || strlen(str) < 1) {
					continue;
				};
				break;
			case PARAM_INT:
				if (G_STRUCT_MEMBER_CONST
				    (gint, params, param->offset) < 1) {

					continue;
				};
				/* Sub-parameters to use-dice-deck need never
				 * to be sent to older clients, the message
				 * about use-dice-deck will suffice */
				if (!strcmp(param->name, "num-dice-decks")
				    || !strcmp(param->name,
					       "num-removed-dice-cards")) {
					continue;
				}
				break;
			case PARAM_BOOL:
				if (!G_STRUCT_MEMBER_CONST
				    (gboolean, params, param->offset)) {
					continue;
				}
				break;
			case PARAM_INTLIST:
				buff =
				    format_int_list(param->name,
						    G_STRUCT_MEMBER_CONST
						    (GArray *, params,
						     param->offset));
				if (buff == NULL) {
					continue;
				};
				g_free(buff);
				break;
			case PARAM_OBSOLETE_DATA:
				/* Obsolete rule: go to the next rule */
				continue;
			}
			buff = g_strdup_printf("new-rule %s", param->name);
			func(user_data, buff);
			g_free(buff);
			continue;
		}

		switch (param->type) {
		case PARAM_SINGLE_LINE:
			str =
			    G_STRUCT_MEMBER_CONST(gchar *, params,
						  param->offset);
			if (!str)
				continue;
			buff = g_strdup_printf("%s %s", param->name, str);
			func(user_data, buff);
			g_free(buff);
			break;
		case PARAM_MULTIPLE_LINES:
			str =
			    G_STRUCT_MEMBER_CONST(gchar *, params,
						  param->offset);
			if (str) {
				gchar **strv;
				gchar **strv_it;
				strv = g_strsplit(str, "\n", 0);
				strv_it = strv;
				while (*strv_it) {
					buff =
					    g_strdup_printf("%s %s",
							    param->name,
							    *strv_it);
					func(user_data, buff);
					g_free(buff);
					strv_it++;
				}
				g_strfreev(strv);
			}
			break;
		case PARAM_INT:
			buff = g_strdup_printf("%s %d", param->name,
					       G_STRUCT_MEMBER_CONST(gint,
								     params,
								     param->
								     offset));
			func(user_data, buff);
			g_free(buff);
			break;
		case PARAM_BOOL:
			if (G_STRUCT_MEMBER_CONST
			    (gboolean, params, param->offset)) {
				func(user_data, param->name);
			}
			break;
		case PARAM_INTLIST:
			buff =
			    format_int_list(param->name,
					    G_STRUCT_MEMBER_CONST(GArray *,
								  params,
								  param->
								  offset));
			/* Don't send empty intlists */
			if (buff != NULL) {
				func(user_data, buff);
				g_free(buff);
			}
			break;
		case PARAM_OBSOLETE_DATA:
			/* Obsolete rule: do nothing */
			break;
		}
	}
	buff = format_int_list("chits", params->map->chits);
	func(user_data, buff);
	g_free(buff);
	func(user_data, "map");
	for (y = 0; y < params->map->y_size; y++) {
		buff = map_format_line(params->map, write_secrets, y);
		func(user_data, buff);
		g_free(buff);
	}
	func(user_data, ".");
	if (params->map) {
		struct nosetup_t tmp;
		tmp.user_data = user_data;
		tmp.func = func;
		map_traverse_const(params->map, find_no_setup, &tmp);
	}
}

gboolean params_load_line(GameParams * params, const gchar * line)
{
	guint idx;

	if (params->map == NULL)
		params->map = map_new();
	if (params->parsing_map) {
		if (strcmp(line, ".") == 0) {
			params->parsing_map = FALSE;
			if (!map_parse_finish(params->map)) {
				map_free(params->map);
				params->map = NULL;
				return FALSE;
			}
		} else
			return map_parse_line(params->map, line);
		return TRUE;
	}
	line = skip_space(line);
	if (*line == 0)
		return TRUE;
	if (match_word(&line, "map")) {
		params->parsing_map = TRUE;
		return TRUE;
	}
	if (match_word(&line, "chits")) {
		if (params->map->chits != NULL)
			g_array_free(params->map->chits, TRUE);
		params->map->chits = build_int_list(line);
		if (params->map->chits == NULL) {
			g_warning("Zero length chits array");
			return FALSE;
		}
		return TRUE;
	}
	if (match_word(&line, "nosetup")) {
		gint x = 0, y = 0, pos = 0;
		Node *node;
		/* don't tolerate invalid game descriptions */
		g_assert(params->map != NULL);
		sscanf(line, "%d %d %d", &x, &y, &pos);
		node = map_node(params->map, x, y, pos);
		if (node) {
			node->no_setup = TRUE;
		} else {
			g_warning
			    ("Nosetup node %d %d %d is not in the map", x,
			     y, pos);
		}
		return TRUE;
	}

	for (idx = 0; idx < G_N_ELEMENTS(game_params); idx++) {
		Param *param = game_params + idx;
		gchar *str;
		GArray *array;

		if (!match_word(&line, param->name))
			continue;
		switch (param->type) {
		case PARAM_SINGLE_LINE:
			str =
			    G_STRUCT_MEMBER(gchar *, params,
					    param->offset);
			if (str)
				g_free(str);
			str = g_strchomp(g_strdup(line));
			G_STRUCT_MEMBER(gchar *, params, param->offset) =
			    str;
			return TRUE;
		case PARAM_MULTIPLE_LINES:
			str =
			    G_STRUCT_MEMBER(gchar *, params,
					    param->offset);
			if (str) {
				gchar *copy;
				gchar *line2 = g_strdup(line);
				copy =
				    g_strconcat(str, "\n",
						g_strchomp(line2), NULL);
				g_free(line2);
				g_free(str);
				str = copy;
			} else {
				str = g_strchomp(g_strdup(line));
			}
			G_STRUCT_MEMBER(gchar *, params, param->offset) =
			    str;
			return TRUE;
		case PARAM_INT:
			G_STRUCT_MEMBER(gint, params, param->offset) =
			    atoi(line);
			return TRUE;
		case PARAM_BOOL:
			G_STRUCT_MEMBER(gboolean, params, param->offset) =
			    TRUE;
			return TRUE;
		case PARAM_INTLIST:
			array =
			    G_STRUCT_MEMBER(GArray *, params,
					    param->offset);
			if (array != NULL)
				g_array_free(array, TRUE);
			array = build_int_list(line);
			if (array == NULL) {
				g_warning("Zero length array for %s",
					  param->name);
			}
			G_STRUCT_MEMBER(GArray *, params, param->offset) =
			    array;
			return array != NULL;
		case PARAM_OBSOLETE_DATA:
			log_message(MSG_ERROR, _("Obsolete rule: '%s'\n"),
				    param->name);
			return TRUE;
		}
	}
	if (match_word(&line, "new-rule")) {
		log_message(MSG_INFO,
			    _("The game uses the new rule '%s', which "
			      "is not yet supported. "
			      "Consider upgrading.\n"), line);
		return TRUE;
	}
	g_warning("Unknown keyword: %s", line);
	return FALSE;
}

/* read a line from a file.  The memory needed is allocated.  The returned line
 * is unbounded.  Returns FALSE if no (partial) line could be read */
gboolean read_line_from_file(gchar ** line, FILE * f)
{
	gchar part[512];
	size_t len;

	if (fgets(part, sizeof(part), f) == NULL)
		return FALSE;

	len = strlen(part);
	g_assert(len > 0);
	*line = g_strdup(part);
	while ((*line)[len - 1] != '\n') {
		gchar *oldline;
		if (fgets(part, sizeof(part), f) == NULL)
			break;
		oldline = *line;
		*line = g_strdup_printf("%s%s", *line, part);
		g_free(oldline);
		len = strlen(*line);
	}
	/* In case of error or EOF, just return the part we have.
	 * Otherwise, strip the newline.  */
	if ((*line)[len - 1] == '\n')
		(*line)[len - 1] = '\0';
	return TRUE;
}

GameParams *params_load_file(const gchar * fname)
{
	FILE *fp;
	gchar *line;
	GameParams *params;

	if ((fp = fopen(fname, "r")) == NULL) {
		g_warning("could not open '%s'", fname);
		return NULL;
	}

	params = params_new();
	while (read_line_from_file(&line, fp) && params) {
		if (!params_load_line(params, line)) {
			params_free(params);
			params = NULL;
		}
		g_free(line);
	}
	fclose(fp);
	if (params && !params_load_finish(params)) {
		params_free(params);
		return NULL;
	}
	return params;
}

GameParams *params_copy(const GameParams * params)
{
	GameParams *copy;
	guint idx;
	gchar *buff;

	if (params == NULL)
		return NULL;

	copy = params_new();
	copy->map = map_copy(params->map);

	for (idx = 0; idx < G_N_ELEMENTS(game_params); idx++) {
		Param *param = game_params + idx;

		switch (param->type) {
		case PARAM_SINGLE_LINE:
		case PARAM_MULTIPLE_LINES:
			G_STRUCT_MEMBER(gchar *, copy, param->offset)
			    =
			    g_strdup(G_STRUCT_MEMBER_CONST
				     (gchar *, params, param->offset));
			break;
		case PARAM_INT:
			G_STRUCT_MEMBER(gint, copy, param->offset)
			    = G_STRUCT_MEMBER_CONST(gint, params,
						    param->offset);
			break;
		case PARAM_BOOL:
			G_STRUCT_MEMBER(gboolean, copy, param->offset)
			    = G_STRUCT_MEMBER_CONST(gboolean, params,
						    param->offset);
			break;
		case PARAM_INTLIST:
			buff =
			    format_int_list("",
					    G_STRUCT_MEMBER_CONST(GArray *,
								  params,
								  param->
								  offset));
			if (buff != NULL) {
				G_STRUCT_MEMBER(GArray *, copy,
						param->offset) =
				    build_int_list(buff);
				g_free(buff);
			}
			break;
		case PARAM_OBSOLETE_DATA:
			/* Obsolete rule: do nothing */
			break;
		}
	}

	copy->quit_when_done = params->quit_when_done;
	copy->tournament_time = params->tournament_time;
	return copy;
}

static void append_to_string(gpointer base, const gchar * additional_text)
{
	gchar **b = base;
	gchar *old = *b;
	if (*b == NULL) {
		*b = g_strdup(additional_text);
	} else {
		*b = g_strdup_printf("%s\n%s", old, additional_text);
	}
	g_free(old);
}

gboolean params_is_equal(const GameParams * params1,
			 const GameParams * params2)
{
	gint i;
	guint idx;
	gchar *buff1;
	gchar *buff2;
	gboolean is_different;
	struct nosetup_t tmp;

	/* Compare the map */
	if (params1->map->y_size != params2->map->y_size) {
		return FALSE;
	};
	for (i = 0; i < params1->map->y_size; i++) {
		buff1 = map_format_line(params1->map, TRUE, i);
		buff2 = map_format_line(params2->map, TRUE, i);

		is_different = g_strcmp0(buff1, buff2) != 0;
		g_free(buff1);
		g_free(buff2);
		if (is_different) {
			return FALSE;
		}
	}

	buff1 = format_int_list("", params1->map->chits);
	buff2 = format_int_list("", params2->map->chits);
	is_different = g_strcmp0(buff1, buff2) != 0;
	g_free(buff1);
	g_free(buff2);
	if (is_different) {
		return FALSE;
	}

	buff1 = NULL;
	tmp.user_data = &buff1;
	tmp.func = append_to_string;
	map_traverse_const(params1->map, find_no_setup, &tmp);
	buff2 = NULL;
	tmp.user_data = &buff2;
	map_traverse_const(params2->map, find_no_setup, &tmp);

	is_different = g_strcmp0(buff1, buff2) != 0;
	g_free(buff1);
	g_free(buff2);
	if (is_different) {
		return FALSE;
	}

	/* Compare the game parameters */
	for (idx = 0; idx < G_N_ELEMENTS(game_params); idx++) {
		const Param *param = game_params + idx;

		switch (param->type) {
		case PARAM_SINGLE_LINE:
		case PARAM_MULTIPLE_LINES:
			if (g_strcmp0
			    (G_STRUCT_MEMBER_CONST
			     (gchar *, params1, param->offset),
			     G_STRUCT_MEMBER_CONST(gchar *, params2,
						   param->offset)) != 0) {
				return FALSE;
			}
			break;
		case PARAM_INT:
			if (G_STRUCT_MEMBER_CONST
			    (gint, params1,
			     param->offset) != G_STRUCT_MEMBER_CONST(gint,
								     params2,
								     param->offset))
			{
				return FALSE;
			}
			break;
		case PARAM_BOOL:
			if (G_STRUCT_MEMBER_CONST
			    (gboolean, params1,
			     param->offset) !=
			    G_STRUCT_MEMBER_CONST(gboolean, params2,
						  param->offset)) {
				return FALSE;
			}
			break;
		case PARAM_INTLIST:
			buff1 = format_int_list("",
						G_STRUCT_MEMBER_CONST
						(GArray *, params1,
						 param->offset));
			buff2 =
			    format_int_list("",
					    G_STRUCT_MEMBER_CONST(GArray *,
								  params2,
								  param->offset));

			is_different = g_strcmp0(buff1, buff2) != 0;
			g_free(buff1);
			g_free(buff2);
			if (is_different) {
				return FALSE;
			}
			break;
		case PARAM_OBSOLETE_DATA:
			/* Obsolete rule: do nothing */
			break;
		}
	}
	return TRUE;
}

/** Returns TRUE if the params are valid */
gboolean params_load_finish(GameParams * params)
{
	if (!params->map) {
		g_warning("Missing map");
		return FALSE;
	}
	if (params->parsing_map) {
		g_warning("Map not complete. Missing . after the map?");
		return FALSE;
	}
	if (!params->map->chits) {
		g_warning("No chits defined");
		return FALSE;
	}
	if (params->map->chits->len < 1) {
		g_warning("At least one chit must be defined");
		return FALSE;
	}
	if (!params->title) {
		g_warning("Game has no title");
		return FALSE;
	}
	params->map->have_bridges =
	    params->num_build_type[BUILD_BRIDGE] > 0;
	params->map->has_pirate = params->use_pirate;
	return TRUE;
}

static void write_one_line(gpointer user_data, const gchar * line)
{
	FILE *fp = user_data;
	fprintf(fp, "%s\n", line);
}

gboolean params_write_file(const GameParams * params, const gchar * fname)
{
	FILE *fp;

	if ((fp = fopen(fname, "w")) == NULL) {
		g_warning("could not open '%s'", fname);
		return FALSE;
	}
	params_write_lines(params, LATEST_VERSION, TRUE, write_one_line,
			   fp);

	fclose(fp);

	return TRUE;
}

/* Conversions for ClientVersionType. Keep the newest version on top. */
static struct ClientVersionTypeConversion {
	ClientVersionType type;
	const gchar *string;
} client_version_type_conversions[] = {
	{
	V15, "15"}, {
	V14, "14"}, {
	V0_12, "0.12"}, {
	V0_11, "0.11"}, {
	V0_10, "0.10"}
};

ClientVersionType client_version_type_from_string(const gchar * cvt)
{
	guint i;

	for (i = 0; i < G_N_ELEMENTS(client_version_type_conversions); i++) {
		if (!strcmp
		    (cvt, client_version_type_conversions[i].string)) {
			return client_version_type_conversions[i].type;
		}
	}
	return UNKNOWN_VERSION;
}

const gchar *client_version_type_to_string(ClientVersionType cvt)
{
	guint i;

	for (i = 0; i < G_N_ELEMENTS(client_version_type_conversions); i++) {
		if (cvt == client_version_type_conversions[i].type) {
			return client_version_type_conversions[i].string;
		}
	}
	return "unknown";
}

gboolean can_client_connect_to_server(ClientVersionType client_version,
				      ClientVersionType server_version)
{
	if (client_version == UNKNOWN_VERSION) {
		return FALSE;
	} else if (server_version == UNKNOWN_VERSION) {
		return FALSE;
	} else if (client_version > server_version) {
		/* By design the server must be the newest */
		return FALSE;
	} else if (client_version == server_version) {
		return TRUE;
	} else {
		/* In compatibility mode */
		/* If somehow the backwards compatibility needs to be broken,
		   this function should return FALSE here */
		return TRUE;
	}
}

WinnableState params_check_winnable_state(const GameParams * params,
					  gchar ** win_message,
					  gchar ** point_specification)
{
	guint target;
	guint building;
	guint development;
	guint road;
	guint army;
	guint idx;
	WinnableState return_value;
	gint total_island;
	guint max_island;
	guint number_of_islands;

	if (params == NULL) {
		*win_message = g_strdup("Error: no GameParams provided");
		*point_specification = g_strdup("");
		return PARAMS_NO_WIN;
	}

	/* Check whether the game is winnable at all */
	target = params->victory_points;
	building =
	    params->num_build_type[BUILD_SETTLEMENT] +
	    (params->num_build_type[BUILD_SETTLEMENT] > 0 ?
	     params->num_build_type[BUILD_CITY] * 2 : 0);
	road =
	    (params->num_build_type[BUILD_ROAD] +
	     params->num_build_type[BUILD_SHIP] +
	     params->num_build_type[BUILD_BRIDGE]) >= 5 ? 2 : 0;
	army = params->num_develop_type[DEVEL_SOLDIER] >= 3 ? 2 : 0;
	development = 0;
	for (idx = 0; idx < NUM_DEVEL_TYPES; idx++) {
		if (is_victory_card(idx))
			development += params->num_develop_type[idx];
	}

	number_of_islands = map_count_islands(params->map);
	if (number_of_islands == 0) {
		*win_message = g_strdup(_("This game cannot be won."));
		*point_specification = g_strdup(_("There is no land."));
		return PARAMS_NO_WIN;
	}

	/* It is not guaranteed that the islands can be reached */
	total_island = 0;
	max_island = 0;
	if (params->island_discovery_bonus != NULL
	    && params->island_discovery_bonus->len > 0
	    && (params->num_build_type[BUILD_SHIP] +
		params->num_build_type[BUILD_BRIDGE] > 0)) {
		guint i;
		for (i = 0; i < number_of_islands - 1; i++) {
			total_island +=
			    g_array_index(params->island_discovery_bonus,
					  gint,
					  MIN
					  (params->
					   island_discovery_bonus->len - 1,
					   i));
			/* The island score can be negative */
			if ((gint) max_island < total_island)
				max_island = total_island;
		}
	}

	if (target > building) {
		if (target >
		    building + development + road + army + max_island) {
			*win_message =
			    g_strdup(_("This game cannot be won."));
			return_value = PARAMS_NO_WIN;
		} else {
			*win_message =
			    g_strdup(_(""
				       "It is possible that this "
				       "game cannot be won."));
			return_value = PARAMS_WIN_PERHAPS;
		}
	} else {
		*win_message = g_strdup(_("This game can be won by only "
					  "building all settlements and "
					  "cities."));
		return_value = PARAMS_WIN_BUILD_ALL;
	}
	*point_specification =
	    g_strdup_printf(_(""
			      "Required victory points: %d\n"
			      "Points obtained by building all: %d\n"
			      "Points in development cards: %u\n"
			      "Longest road/largest army: %d+%d\n"
			      "Maximum island discovery bonus: %d\n"
			      "Total: %d"), target, building, development,
			    road, army, max_island,
			    building + (gint) development + road + army +
			    max_island);
	return return_value;
}

gboolean params_game_is_unstartable(const GameParams * params)
{
	return params->num_build_type[BUILD_SETTLEMENT] < 2;
}

PlayerType determine_player_type(const gchar * style)
{
	gchar **style_parts;
	PlayerType type;

	if (style == NULL)
		return PLAYER_UNKNOWN;

	style_parts = g_strsplit(style, " ", 0);
	if (!strcmp(style_parts[0], "ai")) {
		type = PLAYER_COMPUTER;
	} else if (!strcmp(style_parts[0], "human")
		   || !strcmp(style, default_player_style)) {
		type = PLAYER_HUMAN;
	} else {
		type = PLAYER_UNKNOWN;
	}
	g_strfreev(style_parts);
	return type;
}

Points *points_new(gint id, const gchar * name, gint points)
{
	Points *p = g_malloc0(sizeof(Points));
	p->id = id;
	p->name = g_strdup(name);
	p->points = points;
	return p;
}

void points_free(Points * points)
{
	g_free(points->name);
}

/* Not translated, these strings are parts of the communication protocol */
static const gchar *resource_types[] = {
	"brick",
	"grain",
	"ore",
	"wool",
	"lumber"
};

static ssize_t get_num(const gchar * str, gint * num)
{
	ssize_t len = 0;
	gboolean is_negative = FALSE;

	if (*str == '-') {
		is_negative = TRUE;
		str++;
		len++;
	}
	*num = 0;
	while (isdigit(*str)) {
		*num = *num * 10 + *str++ - '0';
		len++;
	}
	if (is_negative)
		*num = -*num;
	return len;
}

static ssize_t get_unum(const gchar * str, guint * num)
{
	ssize_t len = 0;

	*num = 0;
	while (isdigit(*str)) {
		*num = *num * 10 + (guint) (*str++ - '0');
		len++;
	}
	return len;
}

ssize_t game_scanf(const gchar * line, const gchar * fmt, ...)
{
	va_list ap;
	ssize_t offset;

	va_start(ap, fmt);
	offset = game_vscanf(line, fmt, ap);
	va_end(ap);

	return offset;
}

ssize_t game_vscanf(const gchar * line, const gchar * fmt, va_list ap)
{
	ssize_t offset = 0;

	while (*fmt != '\0' && line[offset] != '\0') {
		gchar **str;
		gint *num;
		guint *unum;
		gint idx;
		ssize_t len;
		BuildType *build_type;
		Resource *resource;

		if (*fmt != '%') {
			if (line[offset] != *fmt)
				return -1;
			fmt++;
			offset++;
			continue;
		}
		fmt++;

		switch (*fmt++) {
		case 'S':	/* string from current position to end of line */
			str = va_arg(ap, gchar **);
			*str = g_strdup(line + offset);
			offset += (ssize_t) strlen(*str);
			break;
		case 'd':	/* integer */
			num = va_arg(ap, gint *);
			len = get_num(line + offset, num);
			if (len == 0)
				return -1;
			offset += len;
			break;
		case 'u':	/* unsigned integer */
			unum = va_arg(ap, guint *);
			len = get_unum(line + offset, unum);
			if (len == 0)
				return -1;
			offset += len;
			break;
		case 'B':	/* build type */
			build_type = va_arg(ap, BuildType *);
			if (strncmp(line + offset, "road", 4) == 0) {
				*build_type = BUILD_ROAD;
				offset += 4;
			} else if (strncmp(line + offset, "bridge", 6) ==
				   0) {
				*build_type = BUILD_BRIDGE;
				offset += 6;
			} else if (strncmp(line + offset, "ship", 4) == 0) {
				*build_type = BUILD_SHIP;
				offset += 4;
			} else if (strncmp(line + offset, "settlement", 10)
				   == 0) {
				*build_type = BUILD_SETTLEMENT;
				offset += 10;
			} else if (strncmp(line + offset, "city_wall", 9)
				   == 0) {
				*build_type = BUILD_CITY_WALL;
				offset += 9;
			} else if (strncmp(line + offset, "city", 4) == 0) {
				*build_type = BUILD_CITY;
				offset += 4;
			} else
				return -1;
			break;
		case 'R':	/* list of 5 integer resource counts */
			num = va_arg(ap, gint *);
			for (idx = 0; idx < NO_RESOURCE; idx++) {
				while (line[offset] == ' ')
					offset++;
				len = get_num(line + offset, num);
				if (len == 0)
					return -1;
				offset += len;
				num++;
			}
			break;
		case 'D':	/* development card type */
			num = va_arg(ap, gint *);
			len = get_num(line + offset, num);
			if (len == 0)
				return -1;
			offset += len;
			break;
		case 'r':	/* resource type */
			resource = va_arg(ap, Resource *);
			for (idx = 0; idx < NO_RESOURCE; idx++) {
				const gchar *type = resource_types[idx];
				len = (ssize_t) strlen(type);
				if (strncmp
				    (line + offset, type,
				     (size_t) len) == 0) {
					offset += len;
					*resource = idx;
					break;
				}
			}
			if (idx == NO_RESOURCE)
				return -1;
			break;
		}
	}
	if (*fmt != '\0')
		return -1;
	return offset;
}

#define buff_append(result, format, value) \
	do { \
		gchar *old = result; \
		result = g_strdup_printf("%s" format, result, value); \
		g_free(old); \
	} while (0)

gchar *game_printf(const gchar * fmt, ...)
{
	va_list ap;
	gchar *result;

	va_start(ap, fmt);
	result = game_vprintf(fmt, ap);
	va_end(ap);

	return result;
}

gchar *game_vprintf(const gchar * fmt, va_list ap)
{
	/* initialize result to an allocated empty string */
	gchar *result = g_strdup("");

	while (*fmt != '\0') {
		gchar *pos = strchr(fmt, '%');
		gchar *text_without_format;

		if (pos == NULL) {
			buff_append(result, "%s", fmt);
			break;
		}
		/* add format until next % to result */
		text_without_format = g_strndup(fmt, (gsize) (pos - fmt));
		buff_append(result, "%s", text_without_format);
		g_free(text_without_format);
		fmt = pos + 1;

		switch (*fmt++) {
			BuildType build_type;
			const gint *num;
			gint idx;
		case 's':	/* string */
			buff_append(result, "%s", va_arg(ap, gchar *));
			break;
		case 'd':	/* integer */
		case 'D':	/* development card type */
			buff_append(result, "%d", va_arg(ap, gint));
			break;
		case 'u':	/* unsigned integer */
			buff_append(result, "%u", va_arg(ap, guint));
			break;
		case 'B':	/* build type */
			build_type = va_arg(ap, BuildType);
			switch (build_type) {
			case BUILD_ROAD:
				buff_append(result, "%s", "road");
				break;
			case BUILD_BRIDGE:
				buff_append(result, "%s", "bridge");
				break;
			case BUILD_SHIP:
				buff_append(result, "%s", "ship");
				break;
			case BUILD_SETTLEMENT:
				buff_append(result, "%s", "settlement");
				break;
			case BUILD_CITY:
				buff_append(result, "%s", "city");
				break;
			case BUILD_CITY_WALL:
				buff_append(result, "%s", "city_wall");
				break;
			case BUILD_NONE:
				g_error
				    ("BUILD_NONE passed to game_printf");
				break;
			case BUILD_MOVE_SHIP:
				g_error
				    ("BUILD_MOVE_SHIP passed to game_printf");
				break;
			}
			break;
		case 'R':	/* list of 5 integer resource counts */
			num = va_arg(ap, gint *);
			for (idx = 0; idx < NO_RESOURCE; idx++) {
				if (idx > 0)
					buff_append(result, " %d",
						    num[idx]);
				else
					buff_append(result, "%d",
						    num[idx]);
			}
			break;
		case 'r':	/* resource type */
			buff_append(result, "%s",
				    resource_types[va_arg(ap, Resource)]);
			break;
		}
	}
	return result;
}
