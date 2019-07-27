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
#include "frontend.h"
#include "common_gtk.h"
#include "audio.h"

static GtkWidget *chat_entry;	/* messages text widget */
static GtkListStore *chat_completion_model = NULL;
static gboolean chat_grab_focus_on_update = FALSE; /**< Flag to indicate
 * whether the chat widget should grab the focus whenever a GUI_UPDATE is sent */

enum {
	CHAT_PLAYER_NUM, /**< Player number */
	CHAT_PLAYER_ICON, /**< The player icon */
	CHAT_BEEP_TEXT,	/**< Text for the completion */
	CHAT_COLUMN_LAST
};

static void chat_cb(GtkEntry * entry, G_GNUC_UNUSED gpointer user_data)
{
	const gchar *text = gtk_entry_get_text(entry);

	if (text[0] != '\0') {
		gchar buff[MAX_CHAT + 1];
		gint idx;

		strncpy(buff, text, sizeof(buff) - 1);
		buff[sizeof(buff) - 1] = '\0';
		/* Replace newlines in message with spaces.  In a line
		 * oriented protocol, newlines are a bit confusing :-)
		 */
		for (idx = 0; buff[idx] != '\0'; idx++)
			if (buff[idx] == '\n')
				buff[idx] = ' ';

		cb_chat(buff);
		gtk_entry_set_text(entry, "");
	}
}

GtkWidget *chat_build_panel(void)
{
	GtkWidget *hbox;
	GtkWidget *label;
	GtkEntryCompletion *completion;
	GtkCellRenderer *cell;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_show(hbox);

	label = gtk_label_new(NULL);
	/* Label text */
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Chat</b>"));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	chat_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(chat_entry), MAX_CHAT);
	g_signal_connect(G_OBJECT(chat_entry), "activate",
			 G_CALLBACK(chat_cb), NULL);
	gtk_widget_show(chat_entry);
	gtk_box_pack_start(GTK_BOX(hbox), chat_entry, TRUE, TRUE, 0);

	completion = gtk_entry_completion_new();
	gtk_entry_set_completion(GTK_ENTRY(chat_entry), completion);

	chat_completion_model =
	    gtk_list_store_new(CHAT_COLUMN_LAST, G_TYPE_INT,
			       GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_entry_completion_set_model(completion,
				       GTK_TREE_MODEL
				       (chat_completion_model));
	g_object_unref(chat_completion_model);

	/* In GTK 2.4 the text column cannot be set with g_object_set yet.
	 * Set the column, clear the renderers, and add our own. */
	gtk_entry_completion_set_text_column(completion, CHAT_BEEP_TEXT);

	gtk_cell_layout_clear(GTK_CELL_LAYOUT(completion));

	cell = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(completion),
				   cell, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(completion),
				       cell,
				       "pixbuf", CHAT_PLAYER_ICON, NULL);

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(completion),
				   cell, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(completion),
				       cell, "text", CHAT_BEEP_TEXT, NULL);

	gtk_entry_completion_set_minimum_key_length(completion, 2);
	g_object_unref(completion);

	return hbox;
}

void chat_set_grab_focus_on_update(gboolean grab)
{
	chat_grab_focus_on_update = grab;
}

void chat_set_focus(void)
{
	if (chat_grab_focus_on_update && !gtk_widget_is_focus(chat_entry)) {
		gtk_widget_grab_focus(chat_entry);
		gtk_editable_set_position(GTK_EDITABLE(chat_entry), -1);
	}
}

void chat_player_name(gint player_num, const gchar * name)
{
	GtkTreeIter iter;
	enum TFindResult found;
	GdkPixbuf *pixbuf;

	found =
	    find_integer_in_tree(GTK_TREE_MODEL(chat_completion_model),
				 &iter, CHAT_PLAYER_NUM, player_num);

	switch (found) {
	case FIND_NO_MATCH:
		gtk_list_store_append(chat_completion_model, &iter);
		break;
	case FIND_MATCH_INSERT_BEFORE:
		gtk_list_store_insert_before(chat_completion_model, &iter,
					     &iter);
		break;
	case FIND_MATCH_EXACT:
		break;
	};

	/* connected icon */
	pixbuf = player_create_icon(player_num, TRUE);
	gtk_list_store_set(chat_completion_model, &iter,
			   CHAT_PLAYER_NUM, player_num,
			   CHAT_PLAYER_ICON, pixbuf,
			   CHAT_BEEP_TEXT, g_strdup_printf("/beep %s",
							   name), -1);
	g_object_unref(pixbuf);
}

void chat_player_style(gint player_num)
{
	GtkTreeIter iter;
	enum TFindResult found;
	GdkPixbuf *pixbuf;

	found =
	    find_integer_in_tree(GTK_TREE_MODEL(chat_completion_model),
				 &iter, CHAT_PLAYER_NUM, player_num);

	g_return_if_fail(found == FIND_MATCH_EXACT);

	/* connected icon */
	pixbuf = player_create_icon(player_num, TRUE);
	gtk_list_store_set(chat_completion_model, &iter,
			   CHAT_PLAYER_ICON, pixbuf, -1);
	g_object_unref(pixbuf);
}

void chat_player_quit(gint player_num)
{
	GtkTreeIter iter;
	enum TFindResult found;

	found =
	    find_integer_in_tree(GTK_TREE_MODEL(chat_completion_model),
				 &iter, CHAT_PLAYER_NUM, player_num);
	if (found == FIND_MATCH_EXACT) {
		/* not connected icon */
		GdkPixbuf *pixbuf = player_create_icon(player_num, FALSE);

		gtk_list_store_set(chat_completion_model,
				   &iter, CHAT_PLAYER_ICON, pixbuf, -1);
		g_object_unref(pixbuf);
	}
}

void chat_spectator_quit(gint spectator_num)
{
	GtkTreeIter iter;
	enum TFindResult found;

	found =
	    find_integer_in_tree(GTK_TREE_MODEL(chat_completion_model),
				 &iter, CHAT_PLAYER_NUM, spectator_num);
	if (found == FIND_MATCH_EXACT) {
		gtk_list_store_remove(chat_completion_model, &iter);
	}
}

void chat_clear_names(void)
{
	gtk_list_store_clear(chat_completion_model);
}

/** Beep a player (if the name is found)
 * @param beeping_player The player that sent the /beep
 * @param name           The name of the beeped player
 */
static void beep_player(gint beeping_player, const gchar * name)
{
	gint beeped_player = find_player_by_name(name);
	if (beeped_player != -1) {
		if (beeped_player == my_player_num()) {
			play_sound(SOUND_BEEP);
			frontend_gui_update();
			if (beeping_player == my_player_num())
				log_message(MSG_BEEP, _("Beeper test.\n"));
			else
				log_message(MSG_BEEP,
					    _("%s beeped you.\n"),
					    player_name(beeping_player,
							TRUE));
		} else if (beeping_player == my_player_num()) {
			log_message(MSG_BEEP, _("You beeped %s.\n"), name);
		}
	} else {
		if (beeping_player == my_player_num()) {
			/* No success */
			log_message(MSG_BEEP,
				    _("You could not beep %s.\n"), name);
		}
	}
}

void chat_parser(gint player_num, const gchar * chat)
{
	int tempchatcolor = MSG_INFO;
	gchar *chat_str;
	gchar *chat_alloc;
	const gchar *joining_text;

	/* If the chat matches chat from the AI, translate it.
	 * FIXME: There should be a flag to indicate the player is an AI,
	 *        so that chat from human players will not be translated
	 */
	chat_alloc = g_strdup(_(chat));
	chat_str = chat_alloc;

	if (!strncmp(chat_str, "/beep", 5)) {
		chat_str += 5;
		chat_str += strspn(chat_str, " \t");
		beep_player(player_num, chat_str);
		g_free(chat_alloc);
		return;
	} else if (!strncmp(chat_str, "/me", 3)) {
		/* IRC-compatible /me */
		chat_str += 3;
		chat_str += strspn(chat_str, " \t") - 1;
		chat_str[0] = ':';
	}

	switch (chat_str[0]) {
	case ':':
		chat_str += 1;
		joining_text = " ";
		break;
	case ';':
		chat_str += 1;
		joining_text = "";
		break;
	default:
		joining_text = _(" said: ");
		break;
	}

	if (color_chat_enabled) {
		if (player_is_spectator(player_num))
			tempchatcolor = MSG_SPECTATOR_CHAT;
		else
			switch (player_num) {
			case 0:
				tempchatcolor = MSG_PLAYER1;
				break;
			case 1:
				tempchatcolor = MSG_PLAYER2;
				break;
			case 2:
				tempchatcolor = MSG_PLAYER3;
				break;
			case 3:
				tempchatcolor = MSG_PLAYER4;
				break;
			case 4:
				tempchatcolor = MSG_PLAYER5;
				break;
			case 5:
				tempchatcolor = MSG_PLAYER6;
				break;
			case 6:
				tempchatcolor = MSG_PLAYER7;
				break;
			case 7:
				tempchatcolor = MSG_PLAYER8;
				break;
			default:
				g_assert_not_reached();
				break;
			}
	} else {
		tempchatcolor = MSG_CHAT;
	}
	log_message_chat(player_name(player_num, TRUE), joining_text,
			 tempchatcolor, chat_str);
	g_free(chat_alloc);
	return;
}
