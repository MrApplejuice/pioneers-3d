/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003 Bas Wijnen <shevek@fmf.nl>
 * Copyright (C) 2004-2005 Roland Clobus <rclobus@bigfoot.com>
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
#include <ctype.h>
#include <assert.h>

#include "aboutbox.h"
#include "frontend.h"
#include "cards.h"
#include "cost.h"
#include "log.h"
#include "common_gtk.h"
#include "histogram.h"
#include "theme.h"
#include "config-gnome.h"
#include "audio.h"
#include "notification.h"

#include "ogre/main.h"

static GtkWidget *preferences_dlg;
GtkWidget *app_window;		/* main application window */

#define MAP_WIDTH 350		/* default map width */
#define MAP_HEIGHT 200		/* default map height */

#define PIONEERS_ICON_FILE	"pioneers.png"

static GuiMap *gmap;		/* handle to map drawing code */

enum {
	MAP_PAGE,		/* the map */
	TRADE_PAGE,		/* trading interface */
	QUOTE_PAGE,		/* submit quotes page */
	LEGEND_PAGE,		/* legend */
	SPLASH_PAGE		/* splash screen */
};

static GtkWidget *map_notebook;	/* map area panel */
static GtkWidget *trade_page;	/* trade page in map area */
static GtkWidget *quote_page;	/* quote page in map area */
static GtkWidget *legend_page;	/* legend page in map area */
static GtkWidget *splash_page;	/* splash page in map area */

static GtkWidget *develop_notebook;	/* development card area panel */

static GtkWidget *messages_txt;	/* messages text widget */
static GtkWidget *prompt_lbl;	/* big prompt messages */

static GtkWidget *app_bar;
static GtkWidget *net_status;
static GtkWidget *vp_target_status;

static GtkWidget *main_paned;	/* Horizontal for 16:9, Vertical for 4:3 mode */
static GtkWidget *chat_panel = NULL;	/* Panel for chat, placed below or to the right */

static GtkUIManager *ui_manager = NULL;	/* The manager of the GtkActions */
static GtkWidget *toolbar = NULL;	/* The toolbar */

static gboolean toolbar_show_accelerators = TRUE;
static gboolean color_messages_enabled = TRUE;
static gboolean legend_page_enabled = TRUE;

static GList *rules_callback_list = NULL;

#define PIONEERS_PIXMAP_SPLASH "pioneers/splash.png"

static const gchar *pioneers_pixmaps[] = {
	PIONEERS_PIXMAP_DICE,
	PIONEERS_PIXMAP_TRADE,
	PIONEERS_PIXMAP_ROAD,
	PIONEERS_PIXMAP_SHIP,
	PIONEERS_PIXMAP_SHIP_MOVEMENT,
	PIONEERS_PIXMAP_BRIDGE,
	PIONEERS_PIXMAP_SETTLEMENT,
	PIONEERS_PIXMAP_CITY,
	PIONEERS_PIXMAP_CITY_WALL,
	PIONEERS_PIXMAP_DEVELOP,
	PIONEERS_PIXMAP_FINISH
};

static void gui_set_toolbar_visible(void);
static void gui_toolbar_show_accelerators(gboolean show_accelerators);

static void game_new_cb(void)
{
	route_gui_event(GUI_CONNECT);
}

static void game_leave_cb(void)
{
	frontend_quote_end();
	route_gui_event(GUI_DISCONNECT);
}

static void playername_cb(void)
{
	route_gui_event(GUI_CHANGE_NAME);
}

static void game_quit_cb(void)
{
	route_gui_event(GUI_QUIT);
}

static void roll_dice_cb(void)
{
	route_gui_event(GUI_ROLL);
}

static void trade_cb(void)
{
	route_gui_event(GUI_TRADE);
}

static void undo_cb(void)
{
	route_gui_event(GUI_UNDO);
}

static void finish_cb(void)
{
	route_gui_event(GUI_FINISH);
}

static void build_road_cb(void)
{
	route_gui_event(GUI_ROAD);
}

static void build_ship_cb(void)
{
	route_gui_event(GUI_SHIP);
}

static void move_ship_cb(void)
{
	route_gui_event(GUI_MOVE_SHIP);
}

static void build_bridge_cb(void)
{
	route_gui_event(GUI_BRIDGE);
}

static void build_settlement_cb(void)
{
	route_gui_event(GUI_SETTLEMENT);
}

static void build_city_cb(void)
{
	route_gui_event(GUI_CITY);
}

static void buy_development_cb(void)
{
	route_gui_event(GUI_BUY_DEVELOP);
}

static void build_city_wall_cb(void)
{
	route_gui_event(GUI_CITY_WALL);
}

static void showhide_toolbar_cb(void);
static void preferences_cb(void);

static void help_about_cb(void);
static void game_legend_cb(void);
static void game_histogram_cb(void);
static void game_settings_cb(void);
#ifdef HAVE_HELP
static void help_manual_cb(void);
#endif

/** Toggles full screen mode.
 * @param GtkToggleAction The calling action.
 * @param main_window The window to toggle full screen mode.
*/
static void toggle_full_screen_cb(GtkToggleAction * caller,
				  gpointer main_window)
{
	if (gtk_toggle_action_get_active(caller)) {
		gtk_window_fullscreen(GTK_WINDOW(main_window));
	} else {
		gtk_window_unfullscreen(GTK_WINDOW(main_window));
	}
}

static void zoom_normal_cb(void)
{
	guimap_zoom_normal(gmap);
}

static void zoom_center_map_cb(void)
{
	guimap_zoom_center_map(gmap);
}

/* Normal items */
static GtkActionEntry entries[] = {
	{"GameMenu", NULL,
	 /* Menu entry */
	 N_("_Game"), NULL, NULL, NULL},
	{"GameNew", NULL,
	 /* Menu entry */
	 N_("_New Game"), "<control>N",
	 /* Tooltip for New Game menu entry */
	 N_("Start a new game"), game_new_cb},
	{"GameLeave", NULL,
	 /* Menu entry */
	 N_("_Leave Game"), NULL,
	 /* Tooltip for Leave Game menu entry */
	 N_("Leave this game"), game_leave_cb},
#ifdef ADMIN_GTK
	{"GameAdmin", NULL,
	 /* Menu entry */
	 N_("_Admin"), "<control>A",
	 /* Tooltip for Admin menu entry */
	 N_("Administer Pioneers server"), show_admin_interface},
#endif
	{"PlayerName", NULL,
	 /* Menu entry */
	 N_("_Player Name"), "<control>P",
	 /* Tooltip for Player Name menu entry */
	 N_("Change your player name"), playername_cb},
	{"Legend", NULL,
	 /* Menu entry */
	 N_("L_egend"), NULL,
	 /* Tooltip for Legend menu entry */
	 N_("Terrain legend and building costs"), game_legend_cb},
	{"GameSettings", NULL,
	 /* Menu entry */
	 N_("_Game Settings"), NULL,
	 /* Tooltip for Game Settings menu entry */
	 N_("Settings for the current game"), game_settings_cb},
	{"DiceHistogram", NULL,
	 /* Menu entry */
	 N_("_Dice Histogram"), NULL,
	 /* Tooltip for Dice Histogram menu entry */
	 N_("Histogram of dice rolls"), game_histogram_cb},
	{"GameQuit", NULL,
	 /* Menu entry */
	 N_("_Quit"), "<control>Q",
	 /* Tooltip for Quit menu entry */
	 N_("Quit the program"), game_quit_cb},
	{"ActionsMenu", NULL,
	 /* Menu entry */
	 N_("_Actions"), NULL, NULL, NULL},
	{"RollDice", PIONEERS_PIXMAP_DICE,
	 /* Menu entry */
	 N_("Roll Dice"), "F1",
	 /* Tooltip for Roll Dice menu entry */
	 N_("Roll the dice"), roll_dice_cb},
	{"Trade", PIONEERS_PIXMAP_TRADE,
	 /* Menu entry */
	 N_("Trade"), "F2",
	 /* Tooltip for Trade menu entry */
	 N_("Trade"),
	 trade_cb},
	{"Undo", "edit-undo",
	 /* Menu entry */
	 N_("Undo"), "F3",
	 /* Tooltip for Undo menu entry */
	 N_("Undo"), undo_cb},
	{"Finish", PIONEERS_PIXMAP_FINISH,
	 /* Menu entry */
	 N_("Finish"), "F4",
	 /* Tooltip for Finish menu entry */
	 N_("Finish"), finish_cb},
	{"BuildRoad", PIONEERS_PIXMAP_ROAD,
	 /* Menu entry */
	 N_("Road"), "F5",
	 /* Tooltip for Road menu entry */
	 N_("Build a road"), build_road_cb},
	{"BuildShip", PIONEERS_PIXMAP_SHIP,
	 /* Menu entry */
	 N_("Ship"), "F6",
	 /* Tooltip for Ship menu entry */
	 N_("Build a ship"), build_ship_cb},
	{"MoveShip", PIONEERS_PIXMAP_SHIP_MOVEMENT,
	 /* Menu entry */
	 N_("Move Ship"), "F7",
	 /* Tooltip for Move Ship menu entry */
	 N_("Move a ship"), move_ship_cb},
	{"BuildBridge", PIONEERS_PIXMAP_BRIDGE,
	 /* Menu entry */
	 N_("Bridge"), "F8",
	 /* Tooltip for Bridge menu entry */
	 N_("Build a bridge"), build_bridge_cb},
	{"BuildSettlement", PIONEERS_PIXMAP_SETTLEMENT,
	 /* Menu entry */
	 N_("Settlement"),
	 "F9",
	 /* Tooltip for Settlement menu entry */
	 N_("Build a settlement"), build_settlement_cb},
	{"BuildCity", PIONEERS_PIXMAP_CITY,
	 /* Menu entry */
	 N_("City"), "F10",
	 /* Tooltip for City menu entry */
	 N_("Build a city"), build_city_cb},
	{"BuyDevelopment", PIONEERS_PIXMAP_DEVELOP,
	 /* Menu entry */
	 N_("Develop"), "F11",
	 /* Tooltip for Develop menu entry */
	 N_("Buy a development card"), buy_development_cb},
	{"BuildCityWall", PIONEERS_PIXMAP_CITY_WALL,
	 /* Menu entry */
	 N_("City Wall"), NULL,
	 /* Tooltip for City Wall menu entry */
	 N_("Build a city wall"), build_city_wall_cb},

	{"SettingsMenu", NULL,
	 /* Menu entry */
	 N_("_Settings"), NULL, NULL, NULL},
	{"Preferences", NULL,
	 /* Menu entry */
	 N_("Prefere_nces"), NULL,
	 /* Tooltip for Preferences menu entry */
	 N_("Configure the application"), preferences_cb},

	{"ViewMenu", NULL,
	 /* Menu entry */
	 N_("_View"), NULL, NULL, NULL},
	{"Full", NULL,
	 /* Menu entry */
	 N_("_Reset"),
	 "<control>0",
	 /* Tooltip for Reset menu entry */
	 N_("View the full map"), zoom_normal_cb},
	{"Center", NULL,
	 /* Menu entry */
	 N_("_Center"), NULL,
	 /* Tooltip for Center menu entry */
	 N_("Center the map"), zoom_center_map_cb},

	{"HelpMenu", NULL,
	 /* Menu entry */
	 N_("_Help"), NULL, NULL, NULL},
	{"HelpAbout", NULL,
	 /* Menu entry */
	 N_("_About Pioneers"), NULL,
	 /* Tooltip for About Pioneers menu entry */
	 N_("Information about Pioneers"), help_about_cb},
#ifdef HAVE_HELP
	{"HelpManual", NULL,
	 /* Menu entry */
	 N_("_Help"), "<control>H",
	 /* Tooltip for Help menu entry */
	 N_("Show the manual"), help_manual_cb}
#endif
};

/* Toggle items */
static GtkToggleActionEntry toggle_entries[] = {
	{"FullScreen", NULL,
	 /* Menu entry */
	 N_("_Fullscreen"),
	 "<alt>Return",
	 /* Tooltip for Fullscreen menu entry */
	 N_("Set window to full screen mode"),
	 G_CALLBACK(toggle_full_screen_cb),
	 FALSE},
	{"ShowHideToolbar", NULL,
	 /* Menu entry */
	 N_("_Toolbar"), NULL,
	 /* Tooltip for Toolbar menu entry */
	 N_("Show or hide the toolbar"), showhide_toolbar_cb, TRUE}
};

/* *INDENT-OFF* */
static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='GameMenu'>"
"      <menuitem action='GameNew'/>"
"      <menuitem action='GameLeave'/>"
#ifdef ADMIN_GTK
"      <menuitem action='GameAdmin'/>"
#endif
"      <separator/>"
"      <menuitem action='PlayerName'/>"
"      <separator/>"
"      <menuitem action='Legend'/>"
"      <menuitem action='GameSettings'/>"
"      <menuitem action='DiceHistogram'/>"
"      <separator/>"
"      <menuitem action='GameQuit'/>"
"    </menu>"
"    <menu action='ActionsMenu'>"
"      <menuitem action='RollDice'/>"
"      <menuitem action='Trade'/>"
"      <menuitem action='Undo'/>"
"      <menuitem action='Finish'/>"
"      <separator/>"
"      <menuitem action='BuildRoad'/>"
"      <menuitem action='BuildShip'/>"
"      <menuitem action='MoveShip'/>"
"      <menuitem action='BuildBridge'/>"
"      <menuitem action='BuildSettlement'/>"
"      <menuitem action='BuildCity'/>"
"      <menuitem action='BuyDevelopment'/>"
"      <menuitem action='BuildCityWall'/>"
"    </menu>"
"    <menu action='SettingsMenu'>"
"      <menuitem action='ShowHideToolbar'/>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='FullScreen'/>"
"      <menuitem action='Full'/>"
"      <menuitem action='Center'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='HelpAbout'/>"
#ifdef HAVE_HELP
"      <menuitem action='HelpManual'/>"
#endif
"    </menu>"
"  </menubar>"
"  <toolbar name='MainToolbar'>"
"    <toolitem action='RollDice'/>"
"    <toolitem action='Trade'/>"
"    <toolitem action='Undo'/>"
"    <toolitem action='Finish'/>"
"    <toolitem action='BuildRoad'/>"
"    <toolitem action='BuildShip'/>"
"    <toolitem action='MoveShip'/>"
"    <toolitem action='BuildBridge'/>"
"    <toolitem action='BuildSettlement'/>"
"    <toolitem action='BuildCity'/>"
"    <toolitem action='BuyDevelopment'/>"
"    <toolitem action='BuildCityWall'/>"
"  </toolbar>"
"</ui>";
/* *INDENT-ON* */

GtkWidget *gui_get_dialog_button(GtkDialog * dlg, gint button)
{
	GList *list;

	g_return_val_if_fail(dlg != NULL, NULL);
	g_assert(gtk_dialog_get_action_area(dlg) != NULL);

	list =
	    gtk_container_get_children(GTK_CONTAINER
				       (gtk_dialog_get_action_area(dlg)));
	list = g_list_nth(list, button);
	if (list != NULL) {
		g_assert(list->data != NULL);
		return GTK_WIDGET(list->data);
	}
	return NULL;
}

void gui_reset(void)
{
	guimap_reset(gmap);
}

void gui_set_instructions(const gchar * text)
{
	gtk_statusbar_push(GTK_STATUSBAR(app_bar), 0, text);
}

void gui_set_vp_target_value(gint vp)
{
	gchar *vp_text;

	/* Victory points target in statusbar */
	vp_text = g_strdup_printf(_("Points needed to win: %i"), vp);

	gtk_label_set_text(GTK_LABEL(vp_target_status), vp_text);
	g_free(vp_text);
}

void gui_set_net_status(const gchar * text)
{
	gtk_label_set_text(GTK_LABEL(net_status), text);
}

void gui_cursor_none(void)
{
	MapElement dummyElement;
	dummyElement.pointer = NULL;
	guimap_cursor_set(gmap, NO_CURSOR, -1, NULL, NULL, NULL,
			  &dummyElement, FALSE);
}

void gui_cursor_set(CursorType type,
		    CheckFunc check_func, SelectFunc select_func,
		    CancelFunc cancel_func, const MapElement * user_data)
{
	guimap_cursor_set(gmap, type, my_player_num(),
			  check_func, select_func, cancel_func, user_data,
			  FALSE);
}

void gui_draw_hex(const Hex * hex)
{
	if (gmap->surface != NULL)
		guimap_draw_hex(gmap, hex);
}

void gui_draw_edge(const Edge * edge)
{
	if (gmap->surface != NULL)
		guimap_draw_edge(gmap, edge);
}

void gui_draw_node(const Node * node)
{
	if (gmap->surface != NULL)
		guimap_draw_node(gmap, node);
}

void gui_highlight_chits(gint roll)
{
	guimap_highlight_chits(gmap, roll);
}

static gint button_press_map_cb(GtkWidget * area, GdkEventButton * event,
				G_GNUC_UNUSED gpointer user_data)
{
	if (gtk_widget_get_window(area) == NULL || gmap->map == NULL)
		return FALSE;

	if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
		guimap_cursor_select(gmap);
		return TRUE;
	}
	return FALSE;
}

static GtkWidget *build_map_area(void)
{
	GtkWidget *map_area = guimap_build_drawingarea(gmap, MAP_WIDTH,
						       MAP_HEIGHT);
	gtk_widget_add_events(map_area, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(G_OBJECT(map_area), "button_press_event",
			 G_CALLBACK(button_press_map_cb), NULL);

	return map_area;
}

static GtkWidget *build_messages_panel(void)
{
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *scroll_win;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_widget_show(vbox);

	/* Label for messages log */
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Messages</b>"));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scroll_win, -1, 80);
	gtk_widget_show(scroll_win);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	messages_txt = gtk_text_view_new();
	gtk_widget_show(messages_txt);
	gtk_container_add(GTK_CONTAINER(scroll_win), messages_txt);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(messages_txt), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(messages_txt),
				    GTK_WRAP_WORD);

	message_window_set_text(messages_txt, chat_panel);

	return vbox;
}

void gui_show_trade_page(gboolean show)
{
	/* Normal keyboard focus when visible */
	chat_set_grab_focus_on_update(!show);
	if (show) {
		gtk_widget_show(trade_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      TRADE_PAGE);
	} else {
		gtk_notebook_prev_page(GTK_NOTEBOOK(map_notebook));
		gtk_widget_hide(trade_page);
	}
}

void gui_show_quote_page(gboolean show)
{
	/* Normal keyboard focus when visible */
	chat_set_grab_focus_on_update(!show);
	if (show) {
		gtk_widget_show(quote_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      QUOTE_PAGE);
	} else {
		gtk_notebook_prev_page(GTK_NOTEBOOK(map_notebook));
		gtk_widget_hide(quote_page);
	}
}

static void gui_theme_changed(void)
{
	g_assert(legend_page != NULL);
	gtk_widget_queue_draw(legend_page);
	gtk_widget_queue_draw_area(gmap->area, 0, 0, gmap->width,
				   gmap->height);
}

void gui_show_legend_page(gboolean show)
{
	if (show) {
		gtk_widget_show(legend_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      LEGEND_PAGE);
	} else
		gtk_widget_hide(legend_page);
}

void gui_show_splash_page(gboolean show, GtkWidget * chat_widget)
{
	static GtkWidget *widget = NULL;
	if (chat_widget != NULL)
		widget = chat_widget;
	g_assert(widget != NULL);

	chat_set_grab_focus_on_update(TRUE);
	if (show) {
		gtk_widget_show(splash_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      SPLASH_PAGE);
		gtk_widget_hide(widget);
	} else {
		gtk_widget_hide(splash_page);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(map_notebook),
					      MAP_PAGE);
		gtk_widget_show(widget);
	}
}

static GtkWidget *splash_build_page(void)
{
	GtkWidget *pm;
	GtkWidget *viewport;
	gchar *filename;

	filename = g_build_filename(DATADIR, "pixmaps", "pioneers",
				    "splash.png", NULL);
	pm = gtk_image_new_from_file(filename);
	g_free(filename);

	/* The viewport avoids that the pixmap is drawn up into the tab area if
	 * it's too large for the space provided. */
	viewport = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport),
				     GTK_SHADOW_NONE);
	gtk_widget_show(viewport);
	gtk_widget_set_size_request(pm, 1, 1);
	gtk_widget_show(pm);
	gtk_container_add(GTK_CONTAINER(viewport), pm);
	return viewport;
}

static GtkWidget *build_map_panel(void)
{
	GtkWidget *lbl;
	GtkWidget *hbox;
	GtkWidget *close_button;

	map_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(map_notebook), GTK_POS_TOP);
	gtk_widget_show(map_notebook);

	/* Tab page name */
	lbl = gtk_label_new(_("Map"));
	gtk_widget_show(lbl);
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 build_map_area(), lbl, MAP_PAGE);

	hbox = create_label_with_close_button(
						     /* Tab page name */
						     _("Trade"),
						     /* Tooltip */
						     _("Finish trading"),
						     &close_button);
	frontend_gui_register(close_button, GUI_TRADE_FINISH, "clicked");
	trade_page = trade_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 trade_page, hbox, TRADE_PAGE);
	gtk_widget_hide(trade_page);

	hbox = create_label_with_close_button(
						     /* Tab page name */
						     _("Quote"),
						     /* Tooltip */
						     _(""
						       "Reject domestic trade"),
						     &close_button);
	frontend_gui_register(close_button, GUI_QUOTE_REJECT, "clicked");
	quote_page = quote_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 quote_page, hbox, QUOTE_PAGE);
	gtk_widget_hide(quote_page);

	/* Tab page name */
	lbl = gtk_label_new(_("Legend"));
	gtk_widget_show(lbl);
	legend_page = legend_create_content();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 legend_page, lbl, LEGEND_PAGE);
	if (!legend_page_enabled)
		gui_show_legend_page(FALSE);
	theme_register_callback(G_CALLBACK(gui_theme_changed));

	/* Tab page name, shown for the splash screen */
	lbl = gtk_label_new(_("Welcome to Pioneers"));
	gtk_widget_show(lbl);
	splash_page = splash_build_page();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 splash_page, lbl, SPLASH_PAGE);

	return map_notebook;
}

void gui_discard_show(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 1);
}

void gui_discard_hide(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 0);
}

void gui_gold_show(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 2);
}

void gui_gold_hide(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 0);
}

void gui_prompt_show(const gchar * message)
{
	gtk_label_set_text(GTK_LABEL(prompt_lbl), message);
	/* Force resize of the notebook, this is needed because
	 * GTK does not redraw when the text in a label changes.
	 */
	gtk_container_check_resize(GTK_CONTAINER(develop_notebook));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 3);
}

void gui_prompt_hide(void)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(develop_notebook), 0);
}

static GtkWidget *prompt_build_page(void)
{
	prompt_lbl = gtk_label_new("");
	gtk_widget_show(prompt_lbl);
	return prompt_lbl;
}

static GtkWidget *build_develop_panel(void)
{
	develop_notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(develop_notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(develop_notebook),
				     FALSE);
	gtk_widget_show(develop_notebook);

	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 develop_build_page(), NULL, 0);
	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 discard_build_page(), NULL, 1);
	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 gold_build_page(), NULL, 2);
	gtk_notebook_insert_page(GTK_NOTEBOOK(develop_notebook),
				 prompt_build_page(), NULL, 3);

	return develop_notebook;
}

static gboolean get_16_9_layout(void)
{
	GtkWidget *paned;

	g_return_val_if_fail(main_paned != NULL, FALSE);
	g_return_val_if_fail(chat_panel != NULL, FALSE);

	paned = gtk_paned_get_child1(GTK_PANED(main_paned));
	if (gtk_widget_get_parent(chat_panel) == paned)
		return FALSE;
	return TRUE;
}

static void set_16_9_layout(gboolean layout_16_9)
{
	GtkWidget *paned;
	gboolean can_remove;

	g_return_if_fail(main_paned != NULL);
	g_return_if_fail(chat_panel != NULL);

	paned = gtk_paned_get_child1(GTK_PANED(main_paned));

	/* Increase reference count, otherwise it will be destroyed */
	g_object_ref(chat_panel);

	/* Initially the widget has no parent, and cannot be removed */
	can_remove = gtk_widget_get_parent(chat_panel) != NULL;

	if (layout_16_9) {
		if (can_remove)
			gtk_container_remove(GTK_CONTAINER(paned),
					     chat_panel);
		gtk_container_add(GTK_CONTAINER(main_paned), chat_panel);
	} else {
		if (can_remove)
			gtk_container_remove(GTK_CONTAINER(main_paned),
					     chat_panel);
		gtk_container_add(GTK_CONTAINER(paned), chat_panel);
	}
	g_object_unref(chat_panel);
}

static GtkWidget *build_main_interface(void)
{
	GtkWidget *vbox;
	GtkWidget *hpaned;
	GtkWidget *vpaned;
	GtkWidget *panel;

	hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(hpaned);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_widget_show(vbox);
	gtk_paned_pack1(GTK_PANED(hpaned), vbox, FALSE, TRUE);

	gtk_box_pack_start(GTK_BOX(vbox),
			   identity_build_panel(), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),
			   resource_build_panel(), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),
			   build_develop_panel(), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),
			   player_build_summary(), TRUE, TRUE, 0);

	main_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(main_paned);

	vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_widget_show(vpaned);

	gtk_paned_pack1(GTK_PANED(main_paned), vpaned, TRUE, TRUE);

	gtk_paned_pack1(GTK_PANED(vpaned), build_map_panel(), TRUE, TRUE);

	chat_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gui_show_splash_page(TRUE, chat_panel);

	panel = chat_build_panel();
	frontend_gui_register(panel, GUI_DISCONNECT, NULL);
	gtk_box_pack_start(GTK_BOX(chat_panel), panel, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(chat_panel),
			   build_messages_panel(), TRUE, TRUE, 0);

	set_16_9_layout(config_get_int_with_default
			("settings/layout_16_9", FALSE));

	gtk_paned_pack2(GTK_PANED(hpaned), main_paned, TRUE, TRUE);
	return hpaned;
}

static void quit_cb(G_GNUC_UNUSED GtkWidget * widget,
		    G_GNUC_UNUSED void *data)
{
	guimap_delete(gmap);
	frontend_quit();
}

static void theme_change_cb(GtkWidget * widget, G_GNUC_UNUSED void *data)
{
	gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	MapTheme *theme = g_list_nth_data(theme_get_list(), idx);
	if (theme != theme_get_current()) {
		config_set_string("settings/theme", theme->name);
		theme_set_current(theme);
		if (gmap->surface != NULL) {
			cairo_surface_destroy(gmap->surface);
			gmap->surface = NULL;
		}
		theme_rescale(2 * gmap->x_point);
	}

}

static void show_legend_cb(GtkToggleButton * widget,
			   G_GNUC_UNUSED gpointer user_data)
{
	legend_page_enabled = gtk_toggle_button_get_active(widget);
	gui_show_legend_page(legend_page_enabled);
	config_set_int("settings/legend_page", legend_page_enabled);
}

static void message_color_cb(GtkToggleButton * widget,
			     G_GNUC_UNUSED gpointer user_data)
{
	color_messages_enabled = gtk_toggle_button_get_active(widget);
	config_set_int("settings/color_messages", color_messages_enabled);
	log_set_func_message_color_enable(color_messages_enabled);
}

static void chat_color_cb(GtkToggleButton * widget,
			  G_GNUC_UNUSED gpointer user_data)
{
	color_chat_enabled = gtk_toggle_button_get_active(widget);
	config_set_int("settings/color_chat", color_chat_enabled);
}

static void summary_color_cb(GtkToggleButton * widget,
			     G_GNUC_UNUSED gpointer user_data)
{
	gboolean color_summary = gtk_toggle_button_get_active(widget);
	config_set_int("settings/color_summary", color_summary);
	set_color_summary(color_summary);
}

static void silent_mode_cb(GtkToggleButton * widget,
			   G_GNUC_UNUSED gpointer user_data)
{
	GtkToggleButton *announce_button = user_data;

	gboolean silent_mode = gtk_toggle_button_get_active(widget);
	config_set_int("settings/silent_mode", silent_mode);
	set_silent_mode(silent_mode);
	gtk_toggle_button_set_inconsistent(announce_button, silent_mode);
	gtk_widget_set_sensitive(GTK_WIDGET(announce_button),
				 !silent_mode);
}

static void announce_player_cb(GtkToggleButton * widget,
			       G_GNUC_UNUSED gpointer user_data)
{
	gboolean announce_player = gtk_toggle_button_get_active(widget);
	config_set_int("settings/announce_player", announce_player);
	set_announce_player(announce_player);
}

static void toggle_16_9_cb(GtkToggleButton * widget,
			   G_GNUC_UNUSED gpointer user_data)
{
	gboolean layout_16_9 = gtk_toggle_button_get_active(widget);
	config_set_int("settings/layout_16_9", layout_16_9);
	set_16_9_layout(layout_16_9);
}

static void toggle_notifications_cb(GtkToggleButton * widget,
				    G_GNUC_UNUSED gpointer user_data)
{
	gboolean show_notifications = gtk_toggle_button_get_active(widget);
	config_set_int("settings/show_notifications", show_notifications);
	set_show_notifications(show_notifications);
}

static void showhide_toolbar_cb(void)
{
	gui_set_toolbar_visible();
}

static void toolbar_shortcuts_cb(void)
{
	gui_toolbar_show_accelerators(!toolbar_show_accelerators);
}

static void preferences_cb(void)
{
	GtkWidget *silent_mode_widget;
	GtkWidget *widget;
	GtkWidget *dlg_vbox;
	GtkWidget *theme_label;
	GtkWidget *theme_list;
	GtkWidget *layout;

	guint row;
	gint color_summary;
	GList *theme_elt;
	int i;

	if (preferences_dlg != NULL) {
		gtk_window_present(GTK_WINDOW(preferences_dlg));
		return;
	};

	/* Caption of preferences dialog */
	preferences_dlg = gtk_dialog_new_with_buttons(_(""
							"Pioneers Preferences"),
						      GTK_WINDOW
						      (app_window),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      /* Button text */
						      _("_Close"),
						      GTK_RESPONSE_CLOSE,
						      NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(preferences_dlg),
					GTK_RESPONSE_CLOSE);
	g_signal_connect(G_OBJECT(preferences_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed),
			 &preferences_dlg);
	g_signal_connect(G_OBJECT(preferences_dlg), "response",
			 G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_widget_show(preferences_dlg);

	dlg_vbox =
	    gtk_dialog_get_content_area(GTK_DIALOG(preferences_dlg));
	gtk_widget_show(dlg_vbox);

	layout = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(layout), 3);
	gtk_grid_set_column_spacing(GTK_GRID(layout), 5);
	gtk_widget_show(layout);
	gtk_box_pack_start(GTK_BOX(dlg_vbox), layout, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(layout), 5);

	row = 0;

	theme_list = gtk_combo_box_text_new();
	gtk_widget_set_hexpand(theme_list, TRUE);
	/* Label for changing the theme, in the preferences dialog */
	theme_label = gtk_label_new(_("Theme:"));
	gtk_label_set_xalign(GTK_LABEL(theme_label), 0.0);
	gtk_widget_show(theme_list);
	gtk_widget_show(theme_label);

	for (i = 0, theme_elt = theme_get_list();
	     theme_elt != NULL; ++i, theme_elt = g_list_next(theme_elt)) {
		MapTheme *theme = theme_elt->data;
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
					       (theme_list), theme->name);
		if (theme == theme_get_current())
			gtk_combo_box_set_active(GTK_COMBO_BOX(theme_list),
						 i);
	}
	g_signal_connect(G_OBJECT(theme_list), "changed",
			 G_CALLBACK(theme_change_cb), NULL);

	gtk_grid_attach(GTK_GRID(layout), theme_label, 0, row, 1, 1);
	gtk_grid_attach(GTK_GRID(layout), theme_list, 1, row, 1, 1);
	gtk_widget_set_tooltip_text(theme_list,
				    /* Tooltip for changing the theme in the preferences dialog */
				    _("Choose one of the themes"));
	row++;

	/* Label for the option to show the legend */
	widget = gtk_check_button_new_with_label(_("Show legend"));
	gtk_widget_show(widget);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     legend_page_enabled);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(show_legend_cb), NULL);
	gtk_grid_attach(GTK_GRID(layout), widget, 0, row, 2, 1);
	gtk_widget_set_tooltip_text(widget,
				    /* Tooltip for the option to show the legend */
				    _(""
				      "Show the legend as a page beside the map"));
	row++;

	/* Label for the option to display log messages in color */
	widget = gtk_check_button_new_with_label(_("Messages with color"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(layout), widget, 0, row, 2, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     color_messages_enabled);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(message_color_cb), NULL);
	gtk_widget_set_tooltip_text(widget,
				    /* Tooltip for the option to display log messages in color */
				    _("Show new messages with color"));
	row++;

	widget = gtk_check_button_new_with_label(
							/* Label for the option to display chat in color of player */
							_(""
							  "Chat in color of player"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(layout), widget, 0, row, 2, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     color_chat_enabled);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(chat_color_cb), NULL);
	gtk_widget_set_tooltip_text(widget,
				    /* Tooltip for the option to display chat in color of player */
				    _(""
				      "Show new chat messages in the color of the player"));
	row++;

	/* Label for the option to display the summary with colors */
	widget = gtk_check_button_new_with_label(_("Summary with color"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(layout), widget, 0, row, 2, 1);
	color_summary =
	    config_get_int_with_default("settings/color_summary", TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), color_summary);	/* @todo RC use correct variable */
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(summary_color_cb), NULL);
	gtk_widget_set_tooltip_text(widget,
				    /* Tooltip for the option to display the summary with colors */
				    _("Use colors in the player summary"));
	row++;

	widget =
	    /* Label for the option to display keyboard accelerators in the toolbar */
	    gtk_check_button_new_with_label(_("Toolbar with shortcuts"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(layout), widget, 0, row, 2, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     toolbar_show_accelerators);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(toolbar_shortcuts_cb), NULL);
	gtk_widget_set_tooltip_text(widget,
				    /* Tooltip for the option to display keyboard accelerators in the toolbar */
				    _(""
				      "Show keyboard shortcuts in the toolbar"));
	row++;

	silent_mode_widget =
	    /* Label for the option to disable all sounds */
	    gtk_check_button_new_with_label(_("Silent mode"));
	gtk_widget_show(silent_mode_widget);
	gtk_grid_attach(GTK_GRID(layout), silent_mode_widget, 0, row, 2,
			1);
	gtk_widget_set_tooltip_text(silent_mode_widget,
				    /* Tooltip for the option to disable all sounds */
				    _(""
				      "In silent mode no sounds are made"));
	row++;

	widget =
	    /* Label for the option to announce when players/spectators enter */
	    gtk_check_button_new_with_label(_("Announce new players"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(layout), widget, 0, row, 2, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     get_announce_player());
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(announce_player_cb), NULL);
	gtk_widget_set_tooltip_text(widget,
				    /* Tooltip for the option to use sound when players/spectators enter */
				    _(""
				      "Make a sound when a new player or spectator enters the game"));
	row++;

	/* Silent mode widget is connected an initialized after the announce button */
	g_signal_connect(G_OBJECT(silent_mode_widget), "toggled",
			 G_CALLBACK(silent_mode_cb), widget);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(silent_mode_widget),
				     get_silent_mode());

#ifdef HAVE_NOTIFY
	/* Label for the option to use the notifications. */
	widget = gtk_check_button_new_with_label(_("Show notifications"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(layout), widget, 0, row, 2, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     get_show_notifications());
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(toggle_notifications_cb), NULL);
	gtk_widget_set_tooltip_text(widget,
				    /* Tooltip for notifications option. */
				    _("Show notifications when it's your "
				      "turn or when new trade is available"));
	row++;
#endif

	/* Label for the option to use the 16:9 layout. */
	widget = gtk_check_button_new_with_label(_("Use 16:9 layout"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(layout), widget, 0, row, 2, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				     get_16_9_layout());
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(toggle_16_9_cb), NULL);
	gtk_widget_set_tooltip_text(widget,
				    /* Tooltip for 16:9 option. */
				    _(""
				      "Use a 16:9 friendly layout for the window"));
	row++;
}

static void help_about_cb(void)
{
	aboutbox_display(GTK_WINDOW(app_window),
			 /* Caption of about box */
			 _("About Pioneers"));
}

static void game_legend_cb(void)
{
	legend_create_dlg();
}

static void game_histogram_cb(void)
{
	histogram_create_dlg();
}

static void game_settings_cb(void)
{
	settings_create_dlg();
}

#ifdef HAVE_HELP
static void help_manual_cb(void)
{
	GError *error = NULL;
	gtk_show_uri_on_window(GTK_WINDOW(app_window), "help:pioneers",
			       GDK_CURRENT_TIME, &error);
	if (error) {
		log_message(MSG_ERROR, "%s: %s\n", _("Show the manual"),
			    error->message);
		g_error_free(error);
	}
}
#endif

static GtkAction *getAction(GuiEvent id)
{
	const gchar *path = NULL;
	gchar *full_path;
	GtkAction *action;
#ifdef ADMIN_GTK
	frontend_gui_register_action(gtk_ui_manager_get_action
				     (manager,
				      "ui/MainMenu/GameMenu/GameAdmin"),
				     GUI_CONNECT);
#endif

	switch (id) {
	case GUI_CONNECT:
		path = "GameMenu/GameNew";
		break;
	case GUI_DISCONNECT:
		path = "GameMenu/GameLeave";
		break;
	case GUI_CHANGE_NAME:
		path = "GameMenu/PlayerName";
		break;
	case GUI_ROLL:
		path = "ActionsMenu/RollDice";
		break;
	case GUI_TRADE:
		path = "ActionsMenu/Trade";
		break;
	case GUI_UNDO:
		path = "ActionsMenu/Undo";
		break;
	case GUI_FINISH:
		path = "ActionsMenu/Finish";
		break;
	case GUI_ROAD:
		path = "ActionsMenu/BuildRoad";
		break;
	case GUI_SHIP:
		path = "ActionsMenu/BuildShip";
		break;
	case GUI_MOVE_SHIP:
		path = "ActionsMenu/MoveShip";
		break;
	case GUI_BRIDGE:
		path = "ActionsMenu/BuildBridge";
		break;
	case GUI_SETTLEMENT:
		path = "ActionsMenu/BuildSettlement";
		break;
	case GUI_CITY:
		path = "ActionsMenu/BuildCity";
		break;
	case GUI_BUY_DEVELOP:
		path = "ActionsMenu/BuyDevelopment";
		break;
	case GUI_CITY_WALL:
		path = "ActionsMenu/BuildCityWall";
		break;
	default:
		break;
	};

	if (!path)
		return NULL;

	full_path = g_strdup_printf("ui/MainMenu/%s", path);
	action = gtk_ui_manager_get_action(ui_manager, full_path);
	g_free(full_path);
	return action;
}

/** Set the visibility of the toolbar */
static void gui_set_toolbar_visible(void)
{
	GSList *list;
	gboolean visible;

	list = gtk_ui_manager_get_toplevels(ui_manager,
					    GTK_UI_MANAGER_TOOLBAR);
	g_assert(g_slist_length(list) == 1);
	visible = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION
					       (gtk_ui_manager_get_action
						(ui_manager,
						 "ui/MainMenu/SettingsMenu/ShowHideToolbar")));
	if (visible)
		gtk_widget_show(GTK_WIDGET(list->data));
	else
		gtk_widget_hide(GTK_WIDGET(list->data));
	config_set_int("settings/show_toolbar", visible);
	g_slist_free(list);
}

/** Show the accelerators in the toolbar */
static void gui_toolbar_show_accelerators(gboolean show_accelerators)
{
	GtkToolbar *tb;
	gint n, i;

	toolbar_show_accelerators = show_accelerators;

	tb = GTK_TOOLBAR(toolbar);

	n = gtk_toolbar_get_n_items(tb);
	for (i = 0; i < n; i++) {
		GtkToolItem *ti;
		GtkToolButton *tbtn;
		gchar *text;
		guint j;

		ti = gtk_toolbar_get_nth_item(tb, i);
		tbtn = GTK_TOOL_BUTTON(ti);
		g_assert(tbtn != NULL);
		if (gtk_major_version == 2 && gtk_minor_version == 10) {
			/* Work around a gtk+ 2.10 bug (#434261) that
			 * mishandles strings like (Fn) in labels.
			 */
			/** @todo BW 2007-04-29 Remove this when gtk 2.10
			 * is no longer supported. */
			gtk_tool_button_set_use_underline(tbtn, FALSE);
		}
		text = g_strdup(gtk_tool_button_get_label(tbtn));
		if (strchr(text, '\n'))
			*strchr(text, '\n') = '\0';
		/* Find the matching entry */
		for (j = 0; j < G_N_ELEMENTS(entries); j++) {
			if (strcmp(text, _(entries[j].label)) == 0) {
				if (show_accelerators) {
					gchar *label;

					if (entries[j].accelerator == NULL
					    ||
					    strlen(entries[j].accelerator)
					    == 0)
						label =
						    g_strdup_printf("%s\n",
								    text);
					else {
						gchar *accelerator_text;
						guint accelerator_key;
						GdkModifierType
						    accelerator_mods;
						gtk_accelerator_parse
						    (entries
						     [j].accelerator,
						     &accelerator_key,
						     &accelerator_mods);
						accelerator_text =
						    gtk_accelerator_get_label
						    (accelerator_key,
						     accelerator_mods);
						label =
						    g_strdup_printf
						    ("%s\n(%s)", text,
						     accelerator_text);
						g_free(accelerator_text);
					}
					gtk_tool_button_set_label(tbtn,
								  label);
					g_free(label);
				} else {
					gtk_tool_button_set_label(tbtn,
								  _(entries
								    [j].
								    label));
				}
				break;
			}
		}
		g_free(text);
	}
	config_set_int("settings/toolbar_show_accelerators",
		       toolbar_show_accelerators);
}

/** Show or hide a button in the toolbar */
static void gui_toolbar_show_button(const gchar * path, gboolean visible)
{
	gchar *fullpath;
	GtkWidget *w;
	GtkToolItem *item;

	fullpath = g_strdup_printf("ui/MainToolbar/%s", path);
	w = gtk_ui_manager_get_widget(ui_manager, fullpath);
	if (w == NULL) {
		g_assert(!"Widget not found");
		return;
	}
	item = GTK_TOOL_ITEM(w);
	if (item == NULL) {
		g_assert(!"Widget is not a tool button");
		return;
	}
	gtk_tool_item_set_visible_horizontal(item, visible);

	g_free(fullpath);
}

void gui_rules_register_callback(GCallback callback)
{
	rules_callback_list = g_list_append(rules_callback_list, callback);
}

void gui_set_game_params(const GameParams * params)
{
	GList *list;
	GtkWidget *label;

	gmap->map = params->map;
	gmap->player_num = my_player_num();
	gtk_widget_queue_resize(gmap->area);

	gui_toolbar_show_button("BuildRoad",
				params->num_build_type[BUILD_ROAD] > 0);
	gui_toolbar_show_button("BuildShip",
				params->num_build_type[BUILD_SHIP] > 0);
	gui_toolbar_show_button("MoveShip",
				params->num_build_type[BUILD_SHIP] > 0);
	gui_toolbar_show_button("BuildBridge",
				params->num_build_type[BUILD_BRIDGE] > 0);
	/* In theory, it is possible to play a game without cities */
	gui_toolbar_show_button("BuildCity",
				params->num_build_type[BUILD_CITY] > 0);
	gui_toolbar_show_button("BuildCityWall",
				params->num_build_type[BUILD_CITY_WALL] >
				0);

	identity_draw();

	gui_set_vp_target_value(params->victory_points);

	list = rules_callback_list;
	while (list) {
		G_CALLBACK(list->data) ();
		list = g_list_next(list);
	}

	label =
	    gtk_notebook_get_tab_label(GTK_NOTEBOOK(map_notebook),
				       legend_page);
	g_object_ref(label);

	gtk_widget_destroy(legend_page);
	legend_page = legend_create_content();
	gtk_notebook_insert_page(GTK_NOTEBOOK(map_notebook),
				 legend_page, label, LEGEND_PAGE);
	if (!legend_page_enabled)
		gui_show_legend_page(FALSE);
	g_object_unref(label);
}

static GtkWidget *build_status_bar(void)
{
	GtkWidget *vsep;

	app_bar = gtk_statusbar_new();
	gtk_widget_show(app_bar);

	vp_target_status = gtk_label_new("");
	gtk_widget_show(vp_target_status);
	gtk_box_pack_start(GTK_BOX(app_bar), vp_target_status, FALSE, TRUE,
			   0);

	vsep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	gtk_widget_show(vsep);
	gtk_box_pack_start(GTK_BOX(app_bar), vsep, FALSE, TRUE, 0);

	/* Network status: offline */
	net_status = gtk_label_new(_("Offline"));
	gtk_widget_show(net_status);
	gtk_box_pack_start(GTK_BOX(app_bar), net_status, FALSE, TRUE, 0);

	vsep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	gtk_widget_show(vsep);
	gtk_box_pack_start(GTK_BOX(app_bar), vsep, FALSE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(app_bar),
			   player_build_turn_area(), FALSE, TRUE, 0);

	/* Initial text in status bar */
	gui_set_instructions(_("Welcome to Pioneers!"));

	return app_bar;
}

static void register_pixmaps(void)
{
	guint idx;

	GtkIconFactory *factory = gtk_icon_factory_new();

	for (idx = 0; idx < G_N_ELEMENTS(pioneers_pixmaps); idx++) {
		gchar *filename;
		GtkIconSet *icon;

		icon = NULL;
		/* determine full path to pixmap file */
		filename = g_build_filename(DATADIR, "pixmaps",
					    pioneers_pixmaps[idx], NULL);
		if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
			GdkPixbuf *pixbuf;
			GError *error = NULL;
			pixbuf =
			    gdk_pixbuf_new_from_file(filename, &error);
			if (error != NULL) {
				g_warning("Error loading pixmap %s\n",
					  filename);
				g_error_free(error);
			} else {
				icon =
				    gtk_icon_set_new_from_pixbuf(pixbuf);
			}
		} else {
			/* Missing pixmap */
			g_warning("Pixmap not found: %s", filename);
		}

		gtk_icon_factory_add(factory, pioneers_pixmaps[idx], icon);
		g_free(filename);
		gtk_icon_set_unref(icon);
	}

	gtk_icon_factory_add_default(factory);
	g_object_unref(factory);
}

GtkWidget *gui_build_interface(void)
{
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkActionGroup *action_group;
	GtkAccelGroup *accel_group;
	GError *error = NULL;
	gchar *icon_file;

	player_init();

	gmap = guimap_new();

	register_pixmaps();
	app_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	/* The name of the application */
	gtk_window_set_title(GTK_WINDOW(app_window), _("Pioneers"));

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(app_window), vbox);

	action_group = gtk_action_group_new("MenuActions");
	gtk_action_group_set_translation_domain(action_group, PACKAGE);
	gtk_action_group_add_actions(action_group, entries,
				     G_N_ELEMENTS(entries), app_window);
	gtk_action_group_add_toggle_actions(action_group, toggle_entries,
					    G_N_ELEMENTS(toggle_entries),
					    app_window);

	ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW(app_window), accel_group);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_string
	    (ui_manager, ui_description, -1, &error)) {
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	icon_file =
	    g_build_filename(DATADIR, "pixmaps", PIONEERS_ICON_FILE, NULL);
	if (g_file_test(icon_file, G_FILE_TEST_EXISTS)) {
		gtk_window_set_default_icon_from_file(icon_file, NULL);
	} else {
		/* Missing pixmap, main icon file */
		g_warning("Pixmap not found: %s", icon_file);
	}
	g_free(icon_file);

	color_chat_enabled =
	    config_get_int_with_default("settings/color_chat", TRUE);

	color_messages_enabled =
	    config_get_int_with_default("settings/color_messages", TRUE);
	log_set_func_message_color_enable(color_messages_enabled);

	set_color_summary(config_get_int_with_default
			  ("settings/color_summary", TRUE));

	set_silent_mode(config_get_int_with_default
			("settings/silent_mode", FALSE));
	set_announce_player(config_get_int_with_default
			    ("settings/announce_player", TRUE));
	set_show_notifications(config_get_int_with_default
			       ("settings/show_notifications", TRUE));

	legend_page_enabled =
	    config_get_int_with_default("settings/legend_page", FALSE);

	menubar = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	toolbar = gtk_ui_manager_get_widget(ui_manager, "/MainToolbar");
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), build_main_interface(), TRUE,
			   TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), build_status_bar(), FALSE, FALSE,
			   0);

	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION
				     (gtk_ui_manager_get_action
				      (ui_manager,
				       "ui/MainMenu/SettingsMenu/ShowHideToolbar")),
				     config_get_int_with_default
				     ("settings/show_toolbar", TRUE));

	g_signal_connect(G_OBJECT(app_window), "key_press_event",
			 G_CALLBACK(hotkeys_handler), NULL);

	gtk_widget_show(app_window);

	frontend_gui_register_action(getAction(GUI_CONNECT), GUI_CONNECT);
	frontend_gui_register_action(getAction(GUI_DISCONNECT),
				     GUI_DISCONNECT);
#ifdef ADMIN_GTK
	/** @todo RC 2005-05-26 Admin interface: Not tested */
	frontend_gui_register_action(gtk_ui_manager_get_action
				     (manager,
				      "ui/MainMenu/GameMenu/GameAdmin"),
				     GUI_ADMIN);
#endif
	frontend_gui_register_action(getAction(GUI_CHANGE_NAME),
				     GUI_CHANGE_NAME);
	frontend_gui_register_action(getAction(GUI_ROLL), GUI_ROLL);
	frontend_gui_register_action(getAction(GUI_TRADE), GUI_TRADE);
	frontend_gui_register_action(getAction(GUI_UNDO), GUI_UNDO);
	frontend_gui_register_action(getAction(GUI_FINISH), GUI_FINISH);
	frontend_gui_register_action(getAction(GUI_ROAD), GUI_ROAD);
	frontend_gui_register_action(getAction(GUI_SHIP), GUI_SHIP);
	frontend_gui_register_action(getAction(GUI_MOVE_SHIP),
				     GUI_MOVE_SHIP);
	frontend_gui_register_action(getAction(GUI_BRIDGE), GUI_BRIDGE);
	frontend_gui_register_action(getAction(GUI_SETTLEMENT),
				     GUI_SETTLEMENT);
	frontend_gui_register_action(getAction(GUI_CITY), GUI_CITY);
	frontend_gui_register_action(getAction(GUI_BUY_DEVELOP),
				     GUI_BUY_DEVELOP);
	frontend_gui_register_action(getAction(GUI_CITY_WALL),
				     GUI_CITY_WALL);
#if 0
	frontend_gui_register_destroy(gtk_ui_manager_get_action
				      (manager, "GameQuit"), GUI_QUIT);
#endif

	gui_toolbar_show_button("BuildShip", FALSE);
	gui_toolbar_show_button("MoveShip", FALSE);
	gui_toolbar_show_button("BuildBridge", FALSE);

	gui_toolbar_show_accelerators(config_get_int_with_default
				      ("settings/toolbar_show_accelerators",
				       TRUE));

	gtk_ui_manager_ensure_update(ui_manager);
	gtk_widget_show(app_window);
	g_signal_connect(G_OBJECT(app_window), "delete_event",
			 G_CALLBACK(quit_cb), NULL);

	return app_window;
}

void gui_set_show_no_setup_nodes(gboolean show)
{
	guimap_set_show_no_setup_nodes(gmap, show);
}

Map *frontend_get_map(void)
{
	g_return_val_if_fail(gmap != NULL, NULL);
	return gmap->map;
}

void frontend_set_map(Map * map)
{
	g_assert(gmap != NULL);
	gmap->map = map;
	ogreb_show_map(map);
}
