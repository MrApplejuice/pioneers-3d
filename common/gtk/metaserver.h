/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2008 Roland Clobus <rclobus@bigfoot.com>
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

#ifndef __METASERVER_H__
#define __METASERVER_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#define METASERVER_TYPE            (metaserver_get_type ())
#define METASERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), METASERVER_TYPE, MetaServer))
#define METASERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), METASERVER_TYPE, MetaServerClass))
#define IS_METASERVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), METASERVER_TYPE))
#define IS_METASERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), METASERVER_TYPE))
typedef struct _MetaServer MetaServer;
typedef struct _MetaServerClass MetaServerClass;

struct _MetaServer {
	GtkGrid grid;

	GtkWidget *combo_box;
	GtkListStore *data;
};

struct _MetaServerClass {
	GtkGridClass parent_class;
};

GType metaserver_get_type(void);
GtkWidget *metaserver_new(void);
void metaserver_add(MetaServer * ms, const gchar * text);
gchar *metaserver_get(MetaServer * ms);

G_END_DECLS
#endif				/* __METASERVER_H__ */
