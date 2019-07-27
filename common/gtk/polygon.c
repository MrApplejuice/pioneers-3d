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
#include <math.h>
#include <gdk/gdk.h>

#include "polygon.h"

void poly_offset(Polygon * poly, gint x_offset, gint y_offset)
{
	int idx;
	GdkPoint *points;

	for (idx = 0, points = poly->points; idx < poly->num_points;
	     idx++, points++) {
		points->x += x_offset;
		points->y += y_offset;
	}
}

void poly_bound_rect(const Polygon * poly, int pad, GdkRectangle * rect)
{
	int idx;
	GdkPoint tl;
	GdkPoint br;
	GdkPoint *points;

	points = poly->points;
	tl = points[0];
	br = points[0];
	for (idx = 1, points++; idx < poly->num_points; idx++, points++) {
		if (points->x < tl.x)
			tl.x = points->x;
		else if (points->x > br.x)
			br.x = points->x;
		if (points->y < tl.y)
			tl.y = points->y;
		else if (points->y > br.y)
			br.y = points->y;
	}
	rect->x = tl.x - pad;
	rect->y = tl.y - pad;
	rect->width = br.x - tl.x + pad + 1;
	rect->height = br.y - tl.y + pad + 1;
}

void poly_draw(cairo_t * cr, gboolean filled, const Polygon * poly)
{
	gint i;

	if (poly->num_points > 0) {
		cairo_move_to(cr, poly->points[poly->num_points - 1].x,
			      poly->points[poly->num_points - 1].y);
		for (i = 0; i < poly->num_points; i++) {
			cairo_line_to(cr, poly->points[i].x,
				      poly->points[i].y);
		}
		if (filled) {
			cairo_fill(cr);
		} else {
			cairo_stroke(cr);
		}
	}
}

void poly_draw_with_border(cairo_t * cr,
			   const GdkRGBA * border_color,
			   const Polygon * poly)
{
	poly_draw(cr, TRUE, poly);
	gdk_cairo_set_source_rgba(cr, border_color);
	poly_draw(cr, FALSE, poly);
}
