/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2015 Roland Clobus <rclobus@rclobus.nl>
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

/*
 * Common code for displaying an about box.
 *
 */

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "aboutbox.h"
#include "authors.h"		/* defines AUTHORLIST, as a char **, NULL-ended */
#include "version.h"

void aboutbox_display(GtkWindow * parent, const gchar * title)
{
	GdkPixbuf *logo;
	gchar *filename;
	const gchar *authors[] = {
		AUTHORLIST
	};

	filename = g_build_filename(DATADIR, "pixmaps", "pioneers",
				    "splash.png", NULL);
	logo = gdk_pixbuf_new_from_file(filename, NULL);
	g_free(filename);

	/* *INDENT-OFF* */
	gtk_show_about_dialog(
		parent,
		"authors", authors,
		"comments",
		      _("Pioneers is based upon the excellent\n"
			"Settlers of Catan board game.\n"),
		//  "copyright", _("Copyright \xc2\xa9 2002 the Free Software Foundation"),
		"license-type", GTK_LICENSE_GPL_2_0,
		"logo", logo,
		"title", title,
		/* Translators: add your name here. Keep the list
                 * alphabetically, do not remove any names, and add
                 * \n after your name (except the last name).
		 */
		"translator-credits", _("translator-credits"),
		"version", FULL_VERSION,
		"website", "http://pio.sourceforge.net",
		"website-label", "pio.sourceforge.net",
		NULL);
	/* *INDENT-ON* */

	if (logo)
		g_object_unref(logo);
}
