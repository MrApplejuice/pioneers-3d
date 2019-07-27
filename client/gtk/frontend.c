/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
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
#include "frontend.h"
#include <gdk/gdkkeysyms.h>

static const int MAX_NUMBER_OF_WIDGETS_PER_EVENT = 2;

GHashTable *frontend_widgets;
gboolean frontend_waiting_for_network;

static void set_sensitive(G_GNUC_UNUSED void *key, GuiWidgetState * gui,
			  G_GNUC_UNUSED void *user_data)
{
	if (gui->destroy_only)
		/* Do not modify sensitivity on destroy only events
		 */
		return;

	if (frontend_waiting_for_network)
		gui->next = FALSE;

	if (gui->widget != NULL && gui->next != gui->current) {
		if (GTK_IS_ACTION(gui->widget))
			gtk_action_set_sensitive(GTK_ACTION(gui->widget),
						 gui->next);
		else
			gtk_widget_set_sensitive(gui->widget, gui->next);
	}
	if (gui->next != gui->current) {
		switch (gui->id) {
		case GUI_ROAD:
			guimap_single_click_set_road_mask(gui->next);
			break;
		case GUI_SHIP:
			guimap_single_click_set_ship_mask(gui->next);
			break;
		case GUI_BRIDGE:
			guimap_single_click_set_bridge_mask(gui->next);
			break;
		case GUI_SETTLEMENT:
			guimap_single_click_set_settlement_mask(gui->next);
			break;
		case GUI_CITY:
			guimap_single_click_set_city_mask(gui->next);
			break;
		case GUI_CITY_WALL:
			guimap_single_click_set_city_wall_mask(gui->next);
			break;
		case GUI_MOVE_SHIP:
			guimap_single_click_set_ship_move_mask(gui->next);
			break;
		default:
			break;
		}
	}
	gui->current = gui->next;
	gui->next = FALSE;
}

void frontend_gui_register_init(void)
{
	frontend_widgets = g_hash_table_new(NULL, NULL);
}

void frontend_gui_update(void)
{
	route_gui_event(GUI_UPDATE);
	g_hash_table_foreach(frontend_widgets, (GHFunc) set_sensitive,
			     NULL);
}

void frontend_gui_check(GuiEvent event, gboolean sensitive)
{
	GuiWidgetState *gui;
	gint i;
	gint key = event * MAX_NUMBER_OF_WIDGETS_PER_EVENT;

	/* Set all related widgets */
	for (i = 0; i < MAX_NUMBER_OF_WIDGETS_PER_EVENT; ++i) {
		gui = g_hash_table_lookup(frontend_widgets,
					  GINT_TO_POINTER(key + i));
		if (gui != NULL)
			gui->next = sensitive;
	}
}

gboolean frontend_gui_get_sensitive(GuiEvent event)
{
	GuiWidgetState *gui;
	gint i;
	gint key = event * MAX_NUMBER_OF_WIDGETS_PER_EVENT;

	/* Look at all related widgets */
	for (i = 0; i < MAX_NUMBER_OF_WIDGETS_PER_EVENT; ++i) {
		gui = g_hash_table_lookup(frontend_widgets,
					  GINT_TO_POINTER(key + i));
		if (gui != NULL)
			return gui->current;
	}
	return FALSE;
}

static GuiWidgetState *gui_new(void *widget, gint id)
{
	gint i;
	gint key = id * MAX_NUMBER_OF_WIDGETS_PER_EVENT;
	GuiWidgetState *gui = g_malloc0(sizeof(*gui));
	gui->widget = widget;
	gui->id = id;

	/* Find an empty key */
	i = 0;
	while (i < MAX_NUMBER_OF_WIDGETS_PER_EVENT) {
		if (g_hash_table_lookup(frontend_widgets,
					GINT_TO_POINTER(key + i)))
			++i;
		else
			break;
	}
	g_assert(i != MAX_NUMBER_OF_WIDGETS_PER_EVENT);
	g_hash_table_insert(frontend_widgets, GINT_TO_POINTER(key + i),
			    gui);
	return gui;
}

static void gui_free(GuiWidgetState * gui)
{
	gint i;
	gint key = gui->id * MAX_NUMBER_OF_WIDGETS_PER_EVENT;

	/* Find an empty key */
	for (i = 0; i < MAX_NUMBER_OF_WIDGETS_PER_EVENT; ++i) {
		g_hash_table_remove(frontend_widgets,
				    GINT_TO_POINTER(key + i));
	}
	g_free(gui);
}

static void route_event(G_GNUC_UNUSED void *widget, GuiWidgetState * gui)
{
	route_gui_event(gui->id);
}

static void destroy_event_cb(G_GNUC_UNUSED void *widget,
			     GuiWidgetState * gui)
{
	gui_free(gui);
}

static void destroy_route_event_cb(G_GNUC_UNUSED void *widget,
				   GuiWidgetState * gui)
{
	route_gui_event(gui->id);
	gui_free(gui);
}

void frontend_gui_register_destroy(GtkWidget * widget, GuiEvent id)
{
	GuiWidgetState *gui = gui_new(widget, id);
	gui->destroy_only = TRUE;
	g_signal_connect(G_OBJECT(widget), "destroy",
			 G_CALLBACK(destroy_route_event_cb), gui);
}

void frontend_gui_register_action(GtkAction * action, GuiEvent id)
{
	GuiWidgetState *gui = gui_new(action, id);
	gui->signal = NULL;
	gui->current = TRUE;
	gui->next = FALSE;
}

void frontend_gui_register(GtkWidget * widget, GuiEvent id,
			   const gchar * gui_signal)
{
	GuiWidgetState *gui = gui_new(widget, id);
	gui->signal = gui_signal;
	gui->current = TRUE;
	gui->next = FALSE;
	g_signal_connect(G_OBJECT(widget), "destroy",
			 G_CALLBACK(destroy_event_cb), gui);
	if (gui_signal != NULL)
		g_signal_connect(G_OBJECT(widget), gui_signal,
				 G_CALLBACK(route_event), gui);
}

gint hotkeys_handler(G_GNUC_UNUSED GtkWidget * w, GdkEvent * e,
		     G_GNUC_UNUSED gpointer data)
{
	GuiWidgetState *gui;
	GuiEvent arg;
	switch (e->key.keyval) {
	case GDK_KEY_Escape:
		arg = GUI_QUOTE_REJECT;
		break;
	default:
		return 0;	/* not handled */
	}
	gui = g_hash_table_lookup(frontend_widgets,
				  GINT_TO_POINTER(arg *
						  MAX_NUMBER_OF_WIDGETS_PER_EVENT));
	if (!gui || !gui->current)
		return 0;	/* not handled */
	route_gui_event(arg);
	return 1;		/* handled */
}
