/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2007 Giancarlo Capella <giancarlo@comm.cc>
 * Copyright (C) 2007 Roland Clobus <rclobus@bigfoot.com>
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

#ifndef __playericon_h
#define __playericon_h

#include <gtk/gtk.h>

/** Initialize the player icons */
void playericon_init(void);

guint playericon_human_style_count(void);

/** Create an icon to represent a player.
 *  @param style   The style of the icon
 *  @param color   The base color of the player
 *  @param spectator  TRUE if a spectator icon is requested
 *  @param connected Is the player currently connected
 *  @param width The width
 *  @param height The height
 *  @return The icon for the player. Call cairo_surface_destroy()
 *          when not needed anymore.
 */
cairo_surface_t *playericon_create_icon(const gchar * style,
					GdkRGBA * color,
					gboolean spectator,
					gboolean connected, gdouble width,
					gdouble height);

/** Create a style string for the player.
 *  @param face_color    The color of the face
 *  @param variant       The variant
 *  @param variant_color The color of the variant
 *  @return The style string. Call g_free() when not needed anymore.
 */
gchar *playericon_create_human_style(const GdkRGBA * face_color,
				     gint variant,
				     const GdkRGBA * variant_color);

/** Parse the style string in its components.
 *  @param  style         The style
 *  @retval face_color    The color of the face
 *  @retval variant       The variant
 *  @retval variant_color The color of the variant
 *  @return TRUE if the style could be parsed. When FALSE, the return values contain the default values
 */
gboolean playericon_parse_human_style(const gchar * style,
				      GdkRGBA * face_color,
				      guint * variant,
				      GdkRGBA * variant_color);

/** Convert a string to a color.
 *  The color is allocated in the system colormap.
 * @param spec   The name of the color
 * @retval color The color
 * @return TRUE if the conversion succeeded.
 */
gboolean string_to_color(const gchar * spec, GdkRGBA * color);

/** Convert a color to a string.
 *  After use, the string must be freed with g_free()
 *  @param color The color
 *  @return the string
 */
gchar *color_to_string(GdkRGBA color);

#endif
