/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2009 Andreas Steinel <lnxbil@users.sourceforge.net>
 * Copyright (C) 2010 Roland Clobus <rclobus@rclobus.nl>
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

#ifndef __AVAHI_BROWSER_H__
#define __AVAHI_BROWSER_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#define AVAHIBROWSER_TYPE            (avahibrowser_get_type ())
#define AVAHIBROWSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AVAHIBROWSER_TYPE, AvahiBrowser))
#define AVAHIBROWSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AVAHIBROWSER_TYPE, AvahiBrowserClass))
#define IS_AVAHIBROWSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AVAHIBROWSER_TYPE))
#define IS_AVAHIBROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AVAHIBROWSER_TYPE))
typedef struct _AvahiBrowser AvahiBrowser;
typedef struct _AvahiBrowserClass AvahiBrowserClass;

struct _AvahiBrowser {
	GtkGrid grid;

	GtkWidget *combo_box;
	GtkListStore *data;
	GtkWidget *connect_button;
};

struct _AvahiBrowserClass {
	GtkComboBoxClass parent_class;
};

GType avahibrowser_get_type(void);
GtkWidget *avahibrowser_new(GtkWidget * connect_button);
void avahibrowser_refresh(AvahiBrowser * ab);
void avahibrowser_add(AvahiBrowser * ab, const char *service_name,
		      const char *resolved_hostname, const char *host_name,
		      const gchar * port, const char *version,
		      const char *title);
void avahibrowser_del(AvahiBrowser * ab, const char *service_name);

gchar *avahibrowser_get_server(AvahiBrowser * ab);
gchar *avahibrowser_get_port(AvahiBrowser * ab);

G_END_DECLS
#endif				/* __AVAHI_BROWSER_H__ */
