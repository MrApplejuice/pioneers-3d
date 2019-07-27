/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2000 Dave Cole
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

/* config-gnome.h	-- configuration via. gnome-config (header)
 * initial draft
 *
 * 19 July 2000
 * Bibek Sahu
 */

/*************************** description *******************************

	To simplify a cross-platform API, we'll just demand that all
configuration sets be synchronous.  This is what most people expect, anyway.

	Also, all the paths used for getting/setting items will be made
relative, and a config prefix will be pushed on the stack by config_init().

	The API is essentially mimics a subset of the gnome_config API, but I
believe it's similar (at least in spirit) to the Windows Registry, at least
on the surface.


	The config API will contain the following items (for now):
	
	void config_init( string absolute_path_prefix )
	
	string config_get_string( string relative_path, pointer_to_bool default_used )
	int config_get_int( string relative_path, pointer_to_bool default_used )
	
	// all config_set_* functions must be synchronous
	void config_set_string( string relative_path, string value )
	void config_set_int( string relative_path, int value )

************************* end description *****************************/

/* necessary headers */
#include <glib.h>

/******************************* functions **************************/

/**** initialize configuration setup ****/

/* set the prefix in some static manner so that we don't need to hard-code
 * it in the main code.  Thus, different architectures can have different
 * prefixes depending on what's relevant for said arch.
 */
void config_init(const gchar * path_prefix);

/** Free resources */
void config_finish(void);

/**** get configuration settings ****/

/* get a string.  If a default is sent as part of the path, and the default
 * is returned, set *default_used to 1.
 */
gchar *config_get_string(const gchar * path, gboolean * default_used);

/* get an integer.  If a default is sent as part of the path, and the
 * default is returned, set *default_used to 1.
 */
gint config_get_int(const gchar * path, gboolean * default_used);

/* get an integer.  If the setting is not found, return the default value */
gint config_get_int_with_default(const gchar * path, gint default_value);

/**** set configuration settings ****/
/* these MUST be synchronous */

/* set a string; make sure the configuration set is sync'd afterwards. */
void config_set_string(const gchar * path, const gchar * value);

/* set an int; make sure the configuration set is sync'd afterwards. */
void config_set_int(const gchar * path, gint value);
