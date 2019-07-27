/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2005, 2009 Roland Clobus <rclobus@bigfoot.com>
 * Copyright (C) 2013 Micah Bunting <Amnykon@gmail.com>
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

#include "map-icons.h"
#include "theme.h"

/** Callback to draw a terrain.
 * @param widget The widget to draw on
 * @param cr The cairo context
 * @param terrain_data The Terrain enumeration value.
 */
static gboolean draw_terrain_cb(GtkWidget * widget, cairo_t * cr,
				gpointer terrain_data)
{
	MapTheme *theme = theme_get_current();
	GdkPixbuf *p;
	gint height;
	GtkAllocation allocation;
	Terrain terrain = GPOINTER_TO_INT(terrain_data);

	gtk_widget_get_allocation(widget, &allocation);
	height = allocation.width / theme->scaledata[terrain].aspect;
	p = gdk_pixbuf_scale_simple(theme->scaledata[terrain].native_image,
				    allocation.width, height,
				    GDK_INTERP_BILINEAR);

	gdk_cairo_set_source_pixbuf(cr, p, 0, 0);
	cairo_rectangle(cr, 0, 0, allocation.width, height);
	cairo_fill(cr);

	g_object_unref(p);
	return TRUE;
}

GtkWidget *terrain_icon_new(Terrain terrain)
{
	GtkWidget *area;
	gint width;
	gint height;
	MapTheme *theme = theme_get_current();

	gtk_icon_size_lookup(GTK_ICON_SIZE_DND, &width, &height);
	if (height > width / theme->scaledata[terrain].aspect) {
		height = width / theme->scaledata[terrain].aspect;
	} else {
		width = height * theme->scaledata[terrain].aspect;
	}

	area = gtk_drawing_area_new();
	gtk_widget_show(area);
	gtk_widget_set_size_request(area, width, height);
	g_signal_connect(G_OBJECT(area), "draw",
			 G_CALLBACK(draw_terrain_cb),
			 GINT_TO_POINTER(terrain));
	return area;
}
