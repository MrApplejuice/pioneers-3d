/* Pioneers - Implementation of the excellent Settlers of Catan board game
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

#include "config.h"
#include "colors.h"
#include "player-icon.h"
#include "game.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static gboolean load_pixbuf(const gchar * name, GdkPixbuf ** pixbuf);
static void replace_colors(GdkPixbuf * pixbuf,
			   const GdkRGBA * replace_this,
			   const GdkRGBA * replace_with);

GdkRGBA default_face_color = { 0.84, 0.50, 0.12, 1.0 };
GdkRGBA default_variant_color = { 0.00, 0.00, 0.00, 1.0 };

GSList *player_avatar;
gpointer ai_avatar_data;
gsize ai_avatar_size;

static gchar *build_image_filename(const gchar * name)
{
	return g_build_filename(DATADIR, "pixmaps", "pioneers", name,
				NULL);
}

static gboolean load_pixbuf(const gchar * name, GdkPixbuf ** pixbuf)
{
	gchar *filename;

	filename = build_image_filename(name);
	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
		GError *error = NULL;
		*pixbuf = gdk_pixbuf_new_from_file(filename, &error);
		if (error != NULL) {
			g_warning("Error loading image %s\n", filename);
			g_error_free(error);
			g_free(filename);
			return FALSE;
		}
		g_free(filename);
		return TRUE;
	} else {
		g_free(filename);
		return FALSE;
	}
}

void playericon_init(void)
{
	gint idx;
	gboolean good;
	gchar *filename;

	player_avatar = NULL;
	idx = 1;
	do {
		gchar *name;
		GdkPixbuf *pixbuf;

		name = g_strdup_printf("style-human-%d.png", idx);
		good = load_pixbuf(name, &pixbuf);
		if (good) {
			player_avatar =
			    g_slist_append(player_avatar, pixbuf);
		}
		++idx;
		g_free(name);
	} while (good);

	filename = build_image_filename("style-ai.svg");
	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
		GError *error = NULL;
		GFile *file;
		GMemoryOutputStream *os;
		GFileInputStream *is;

		os = G_MEMORY_OUTPUT_STREAM(g_memory_output_stream_new
					    (NULL, 0, g_realloc, g_free));
		file = g_file_new_for_path(filename);
		is = g_file_read(file, NULL, &error);

		g_output_stream_splice(G_OUTPUT_STREAM(os),
				       G_INPUT_STREAM(is),
				       G_OUTPUT_STREAM_SPLICE_NONE, NULL,
				       &error);
		g_input_stream_close(G_INPUT_STREAM(is), NULL, &error);
		g_output_stream_close(G_OUTPUT_STREAM(os), NULL, &error);

		ai_avatar_size = g_memory_output_stream_get_data_size(os);
		ai_avatar_data = g_memory_output_stream_steal_data(os);
		g_object_unref(is);
		g_object_unref(os);
		g_object_unref(file);
	}
	g_free(filename);
}

guint playericon_human_style_count(void)
{
	return g_slist_length(player_avatar);
}

/** Replace colours in SVG data.
 * @param original_data The SVG data stream
 * @param size The size of the data stream
 * @param replace_this The old colour
 * @param replace_with The new colour
 * @return Newly allocated data, with the colour replaced. Free with g_free()
 */
static gchar *replace_colors_in_svg_data(gconstpointer original_data,
					 gsize size,
					 const GdkRGBA * replace_this,
					 const GdkRGBA * replace_with)
{
	gchar *replace_here;
	gchar *data;
	gchar *color_old;
	gchar *color_new;
	gsize counter;

	data = g_malloc(size);
	memcpy(data, original_data, size);

	color_old = g_strdup_printf("fill:#%02x%02x%02x",
				    (unsigned) (replace_this->red * 255),
				    (unsigned) (replace_this->green * 255),
				    (unsigned) (replace_this->blue * 255));
	color_new = g_strdup_printf("fill:#%02x%02x%02x",
				    (unsigned) (replace_with->red * 255),
				    (unsigned) (replace_with->green * 255),
				    (unsigned) (replace_with->blue * 255));

	g_assert(strlen(color_old) == 12);
	g_assert(strlen(color_new) == 12);

	replace_here = data;
	counter = 0;
	while (counter < size) {
		if (strncmp(replace_here, color_old, 12) == 0) {
			memcpy(replace_here, color_new, 12);
		} else {
			counter++;
			replace_here++;
		}
	}
	g_free(color_old);
	g_free(color_new);
	return data;
}

static void replace_colors(GdkPixbuf * pixbuf,
			   const GdkRGBA * replace_this,
			   const GdkRGBA * replace_with)
{
	gint i;
	guint new_color;
	guint old_color;
	guint *pixel;

	g_assert(gdk_pixbuf_get_colorspace(pixbuf) == GDK_COLORSPACE_RGB);
	g_assert(gdk_pixbuf_get_bits_per_sample(pixbuf) == 8);
	g_assert(gdk_pixbuf_get_has_alpha(pixbuf));
	g_assert(gdk_pixbuf_get_n_channels(pixbuf) == 4);

	pixel = (guint *) gdk_pixbuf_get_pixels(pixbuf);
	new_color = 0xff000000 |
	    (unsigned) (replace_with->red * 255) |
	    (unsigned) (replace_with->green * 255) << 8 |
	    (unsigned) (replace_with->blue * 255) << 16;
	old_color = 0xff000000 |
	    (unsigned) (replace_this->red * 255) |
	    (unsigned) (replace_this->green * 255) << 8 |
	    (unsigned) (replace_this->blue * 255) << 16;
	for (i = 0;
	     i <
	     (gdk_pixbuf_get_rowstride(pixbuf) / 4 *
	      gdk_pixbuf_get_height(pixbuf)); i++) {
		if (*pixel == old_color)
			*pixel = new_color;
		pixel++;
	}
}

cairo_surface_t *playericon_create_icon(const gchar * style,
					GdkRGBA * color,
					gboolean spectator,
					gboolean connected, gdouble width,
					gdouble height)
{
	PlayerType player_type;
	cairo_surface_t *surface;
	cairo_t *cr;

	player_type = determine_player_type(style);

	/* Human players are allowed to have the square icon */
	if (player_type == PLAYER_HUMAN
	    && !strcmp(style, default_player_style)) {
		player_type = PLAYER_UNKNOWN;
	}

	cairo_rectangle_t extent = { 0.0, 0.0, width, height };
	surface =
	    cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA,
					   &extent);
	cr = cairo_create(surface);
	switch (player_type) {
	case PLAYER_COMPUTER:{
			/* This is an AI */
			GError *error;
			GInputStream *is;
			GdkPixbuf *pixbuf;
			gchar *data;

			/* Replace the chromakey color with the player color */
			data =
			    replace_colors_in_svg_data(ai_avatar_data,
						       ai_avatar_size,
						       &blue, color);

			error = NULL;
			is = g_memory_input_stream_new_from_data(data,
								 ai_avatar_size,
								 NULL);
			pixbuf =
			    gdk_pixbuf_new_from_stream_at_scale(is, width,
								height,
								TRUE, NULL,
								&error);
			g_input_stream_close(is, NULL, &error);
			g_object_unref(is);

			g_assert(pixbuf != NULL);
			g_assert(error == NULL);

			gdk_cairo_set_source_pixbuf(cr, pixbuf, 0.0, 0.0);
			cairo_rectangle(cr, 0, 0, width, height);
			cairo_fill(cr);
			g_object_unref(pixbuf);
			g_free(data);
		}
		break;
	case PLAYER_HUMAN:{
			/* Draw a bust */
			guint variant;
			GdkRGBA face_color;
			gdouble face_radius;
			GdkRGBA variant_color;
			GdkPixbuf *pixbuf;
			GdkPixbuf *pixbuf_scaled;

			playericon_parse_human_style(style, &face_color,
						     &variant,
						     &variant_color);
			gdk_cairo_set_source_rgba(cr, color);
			cairo_set_line_width(cr, 1.0);
			cairo_arc_negative(cr, width / 2, height,
					   width / 2, 0.0, M_PI);
			cairo_fill(cr);
			gdk_cairo_set_source_rgba(cr, &black);
			cairo_arc_negative(cr, width / 2, height,
					   width / 2, 0.0, M_PI);
			cairo_move_to(cr, 0.0, height - 0.5);
			cairo_line_to(cr, width, height - 0.5);
			cairo_stroke(cr);
			gdk_cairo_set_source_rgba(cr, &face_color);
			face_radius = 25.0 / 64.0 * height / 2.0;
			cairo_arc(cr, width / 2 + 0.5,
				  height / 2 - face_radius, face_radius,
				  0.0, 2 * M_PI);
			cairo_fill(cr);
			gdk_cairo_set_source_rgba(cr, &black);
			cairo_arc(cr, width / 2 + 0.5,
				  height / 2 - face_radius, face_radius,
				  0.0, 2 * M_PI);
			cairo_stroke(cr);

			pixbuf =
			    gdk_pixbuf_copy(g_slist_nth
					    (player_avatar,
					     variant)->data);

			replace_colors(pixbuf, &blue, &variant_color);
			pixbuf_scaled =
			    gdk_pixbuf_scale_simple(pixbuf, width, height,
						    GDK_INTERP_BILINEAR);

			gdk_cairo_set_source_pixbuf(cr, pixbuf_scaled, 0.0,
						    0.0);
			cairo_rectangle(cr, 0.0, 0.0, width, height);
			cairo_fill(cr);
			g_object_unref(pixbuf_scaled);
			g_object_unref(pixbuf);
		}
		break;
	default:
		/* Unknown or square */
		if (spectator) {
			/* Spectators have a transparent icon */
			cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
			cairo_rectangle(cr, 0, 0, width, height);
			cairo_fill(cr);
		} else {
			gdk_cairo_set_source_rgba(cr, color);
			cairo_rectangle(cr, 0, 0, width, height);
			cairo_fill(cr);

			gdk_cairo_set_source_rgba(cr, &black);
			cairo_set_line_width(cr, 1.0);
			cairo_rectangle(cr, 0.5, 0.5, width - 1,
					height - 1);
			cairo_stroke(cr);
			if (!connected) {
				cairo_rectangle(cr, 3.5, 3.5, width - 7,
						height - 7);
				cairo_rectangle(cr, 6.5, 6.5, width - 13,
						height - 13);
				cairo_stroke(cr);
				/* Don't draw the other emblem */
				connected = TRUE;
			}
		}
	}

	if (!connected) {
		/* Slightly transparent red */
		cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.8);
		cairo_move_to(cr, width / 2, height / 2);
		cairo_arc(cr, width * 3 / 4, height / 4, width / 4, 0.0,
			  2 * M_PI);
		cairo_fill(cr);
		gdk_cairo_set_source_rgba(cr, &white);
		cairo_rectangle(cr,
				width / 2 + 2, height / 4 - height / 16,
				width / 2 - 4, height / 8);
		cairo_fill(cr);
	}
	cairo_destroy(cr);

	return surface;
}

gchar *playericon_create_human_style(const GdkRGBA * face_color,
				     gint variant,
				     const GdkRGBA * variant_color)
{
	gchar *c1;
	gchar *c2;
	gchar *style;

	c1 = color_to_string(*face_color);
	c2 = color_to_string(*variant_color);

	style = g_strdup_printf("human %s %d %s", c1, variant, c2);

	g_free(c1);
	g_free(c2);
	return style;
}

gboolean playericon_parse_human_style(const gchar * style,
				      GdkRGBA * face_color,
				      guint * variant,
				      GdkRGBA * variant_color)
{
	gchar **style_parts;
	gboolean parse_ok;

	/* Determine the style for the player/spectator */
	style_parts = g_strsplit(style, " ", 0);
	parse_ok = FALSE;
	if (!strcmp(style_parts[0], "human")) {
		parse_ok = style_parts[1] != NULL
		    && string_to_color(style_parts[1], face_color);
		if (parse_ok) {
			parse_ok = style_parts[2] != NULL;
		}
		if (parse_ok) {
			*variant = atoi(style_parts[2]);
			parse_ok =
			    *variant <= g_slist_length(player_avatar);
		}
		if (parse_ok) {
			parse_ok = style_parts[3] != NULL
			    && string_to_color(style_parts[3],
					       variant_color);
		}
	}

	if (!parse_ok) {
		/* Something was wrong, revert to the default */
		*face_color = default_face_color;
		*variant = 0;
		*variant_color = default_variant_color;
	}
	g_strfreev(style_parts);
	return parse_ok;
}


gboolean string_to_color(const gchar * spec, GdkRGBA * color)
{
	PangoColor pango_color;

	if (pango_color_parse(&pango_color, spec)) {
		color->red = pango_color.red / 255.0 / 256;
		color->green = pango_color.green / 255.0 / 256;
		color->blue = pango_color.blue / 255.0 / 256;
		color->alpha = 1.0;
		return TRUE;
	}
	return FALSE;
}

gchar *color_to_string(GdkRGBA color)
{
	return g_strdup_printf("#%04x%04x%04x",
			       (unsigned) (color.red * 255 * 256),
			       (unsigned) (color.green * 255 * 256),
			       (unsigned) (color.blue * 255 * 256));
}
