/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2000 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2005 Roland Clobus <rclobus@bigfoot.com>
 * Copyright (C) 2005 Ferenc Bánhidi <banhidi@inf.elte.hu>
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

/* config-gnome.c	-- configuration via. gnome-config
 * initial draft
 *
 * 19 July 2000
 * Bibek Sahu
 */

/*
Functions that need mapping (so far):
	// get
	gnome_config_get_string_with_default( path, default_used_bool )
	gnome_config_get_int_with_default( path, default_used_bool )
	
	// set
	gnome_config_set_string( path, string )
	gnome_config_set_int( path, int )
	
	// sync
	gnome_config_sync() [?]

----

	To simplify a cross-platform API, we'll just demand that all
configuration sets be synchronous.  This is what most people expect, anyway.

	Also, all the paths used for getting/setting items will be made
relative, and a config prefix will be pushed on the stack by config_init().

	The API is essentially mimics a subset of the gnome_config API, but I
believe it's similar (at least in spirit) to the Windows Registry, at least
on the surface.
*/

/*
	The config API will contain the following items (for now):
	
	void config_init( string absolute_path_prefix )
	
	string config_get_string( string relative_path, pointer_to_bool default_used )
	int config_get_int( string relative_path, pointer_to_bool default_used )
	
	// all config_set_* functions must be synchronous
	void config_set_string( string relative_path, string value )
	void config_set_int( string relative_path, int value )
*/

#include "config.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include "config-gnome.h"

/* initialize configuration setup */

static GKeyFile *keyfile = NULL;
static gchar *filename = NULL;

static void config_sync(void)
{
	gsize length;
	GError *error = NULL;
	gchar *data;

	g_return_if_fail(filename != NULL);

	data = g_key_file_to_data(keyfile, &length, &error);

	if (!error) {
		int f = g_open(filename, O_WRONLY | O_CREAT | O_TRUNC,
			       S_IRUSR | S_IWUSR);
		if (f == -1) {
			/* Create the config dir, if it is missing */
			/* Access mode: 0700 (drwx------) */
			g_mkdir(g_get_user_config_dir(), S_IRWXU);
			f = g_open(filename, O_WRONLY | O_CREAT | O_TRUNC,
				   S_IRUSR | S_IWUSR);
		};
		if (f != -1) {
			if (write(f, data, length) < (ssize_t) length) {
				g_warning
				    ("Incomplete settings file written");
			}
			close(f);
		} else {
			g_warning("Could not write settings file");
		}
	} else {
		g_warning("Could not write settings file: %s",
			  error->message);
		g_error_free(error);
	}
}

/* set the prefix in some static manner so that we don't need to hard-code
 * it in the main code.  Thus, different architectures can have different
 * prefixes depending on what's relevant for said arch.
 */
void config_init(const gchar * path_prefix)
{
	GError *error = NULL;

	/* Don't initialize more than once */
	g_return_if_fail(keyfile == NULL);

	keyfile = g_key_file_new();

	filename =
	    g_build_filename(g_get_user_config_dir(), path_prefix, NULL);

	g_key_file_load_from_file(keyfile, filename,
				  G_KEY_FILE_KEEP_COMMENTS |
				  G_KEY_FILE_KEEP_TRANSLATIONS, &error);

	if (error != NULL) {
		g_warning("Error while loading settings: %s",
			  error->message);
		g_error_free(error);
	}
}

void config_finish(void)
{
	g_free(filename);
	g_key_file_free(keyfile);
}

/* get configuration settings */

/* get a string.  If a default is sent as part of the path, and the default
 * is returned, set *default_used to TRUE.
 */
gchar *config_get_string(const gchar * path, gboolean * default_used)
{
	gchar **tokens;
	gchar *value;
	GError *error = NULL;

	g_return_val_if_fail(keyfile != NULL, g_strdup(""));

	tokens = g_strsplit_set(path, "/=", 3);

	value =
	    g_key_file_get_string(keyfile, tokens[0], tokens[1], &error);
	if (error != NULL) {
		if (tokens[2] == NULL) {
			value = g_strdup("");
		} else {
			value = g_strdup(tokens[2]);
		}
		*default_used = TRUE;
		g_error_free(error);
	} else {
		*default_used = FALSE;
	}
	g_strfreev(tokens);
	return value;
}

/* get an integer.  If a default is sent as part of the path, and the
 * default is returned, set *default_used to TRUE.
 */
gint config_get_int(const gchar * path, gboolean * default_used)
{
	gchar **tokens;
	gint value;
	GError *error = NULL;

	g_return_val_if_fail(keyfile != NULL, 0);

	tokens = g_strsplit_set(path, "/=", 3);

	value =
	    g_key_file_get_integer(keyfile, tokens[0], tokens[1], &error);
	if (error != NULL) {
		if (tokens[2] == NULL) {
			value = 0;
		} else {
			value = atoi(tokens[2]);
		}
		*default_used = TRUE;
		g_error_free(error);
	} else {
		*default_used = FALSE;
	}
	g_strfreev(tokens);
	return value;
}

/* get an integer.  If the setting is not found, return the default value */
gint config_get_int_with_default(const gchar * path, gint default_value)
{
	gboolean default_used;
	gchar *temp;
	gint value;

	temp = g_strdup_printf("%s=%d", path, default_value);
	value = config_get_int(temp, &default_used);
	g_free(temp);
	return value;
}

/* set configuration settings */
/* these MUST be synchronous */

/* set a string; make sure the configuration set is sync'd afterwards. */
void config_set_string(const gchar * path, const gchar * value)
{
	gchar **tokens;

	g_return_if_fail(keyfile != NULL);

	tokens = g_strsplit_set(path, "/", 2);

	if (tokens[1] == NULL) {
		g_warning("Key is missing");
	} else {
		g_key_file_set_string(keyfile, tokens[0], tokens[1],
				      value);
	}
	g_strfreev(tokens);
	config_sync();
}

/* set an int; make sure the configuration set is sync'd afterwards. */
void config_set_int(const gchar * path, gint value)
{
	gchar **tokens;

	g_return_if_fail(keyfile != NULL);

	tokens = g_strsplit_set(path, "/", 2);

	if (tokens[1] == NULL) {
		g_warning("Key is missing");
	} else {
		g_key_file_set_integer(keyfile, tokens[0], tokens[1],
				       value);
	}
	g_strfreev(tokens);
	config_sync();
}
