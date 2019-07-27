/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 1999 Dave Cole
 * Copyright (C) 2003-2006 Bas Wijnen <shevek@fmf.nl>
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
#include <ctype.h>
#include <string.h>
#include "game.h"
#include "cards.h"
#include "map.h"
#include "network.h"
#include "log.h"
#include "cost.h"
#include "client.h"
#include "state.h"
#include "callback.h"
#include "buildrec.h"
#include "quoteinfo.h"
#include "notifying-string.h"

static enum callback_mode previous_mode;
GameParams *game_params;
static struct recovery_info_t {
	gchar *prevstate;
	gint turnnum;
	gint playerturn;
	gint numdiscards;
	gboolean rolled_dice;
	gint die1, die2;
	gboolean bought_develop;
	GList *build_list;
	gboolean ship_moved;
} recovery_info;

NotifyingString *requested_name = NULL;
NotifyingString *requested_style = NULL;
gboolean requested_spectator;

static gboolean global_unhandled(StateMachine * sm, gint event);
static gboolean global_filter(StateMachine * sm, gint event);
static gboolean mode_offline(StateMachine * sm, gint event);
static gboolean mode_players(StateMachine * sm, gint event);
static gboolean mode_player_list(StateMachine * sm, gint event);
static gboolean mode_load_game(StateMachine * sm, gint event);
static gboolean mode_load_gameinfo(StateMachine * sm, gint event);
static gboolean mode_setup(StateMachine * sm, gint event);
static gboolean mode_idle(StateMachine * sm, gint event);
static gboolean mode_wait_for_robber(StateMachine * sm, gint event);
static gboolean mode_road_building(StateMachine * sm, gint event);
static gboolean mode_monopoly(StateMachine * sm, gint event);
static gboolean mode_year_of_plenty(StateMachine * sm, gint event);
static gboolean mode_robber(StateMachine * sm, gint event);
static gboolean mode_discard(StateMachine * sm, gint event);
static gboolean mode_turn(StateMachine * sm, gint event);
static gboolean mode_turn_rolled(StateMachine * sm, gint event);
static gboolean mode_domestic_trade(StateMachine * sm, gint event);
static gboolean mode_domestic_quote(StateMachine * sm, gint event);
static gboolean mode_domestic_monitor(StateMachine * sm, gint event);
static gboolean mode_game_over(StateMachine * sm, gint event);
static gboolean mode_wait_resources(StateMachine * sm, gint event);
static gboolean mode_recovery_wait_start_response(StateMachine * sm,
						  gint event);
static void recover_from_disconnect(StateMachine * sm,
				    struct recovery_info_t *rinfo);

/* Create and/or return the client state machine.
 */
StateMachine *SM(void)
{
	static StateMachine *state_machine;
	if (state_machine == NULL) {
		state_machine = sm_new(NULL);
		sm_global_set(state_machine, global_filter);
		sm_unhandled_set(state_machine, global_unhandled);
	}
	return state_machine;
}

/* When commands are sent to the server, front ends may want to update
 * the status bar or something to indicate the the game is currently
 * waiting for server respose.
 * Since the GUI may get disabled while waiting, it is good to let the
 * user know why all controls are unresponsive.
 */
static void waiting_for_network(gboolean is_waiting)
{
	if (is_waiting) {
		callbacks.network_status(_("Waiting"));
	} else {
		callbacks.network_status(_("Idle"));
	}
	callbacks.network_wait(is_waiting);
}

/* Dummy callback functions. They do nothing */
static void dummy_init_glib_et_al(G_GNUC_UNUSED int argc,
				  G_GNUC_UNUSED char **argv)
{;
}

static void dummy_init(void)
{;
}

static void dummy_network_status(G_GNUC_UNUSED const gchar * description)
{;
}

static void dummy_instructions(G_GNUC_UNUSED const gchar * message)
{;
}

static void dummy_network_wait(G_GNUC_UNUSED gboolean is_waiting)
{;
}

static void dummy_offline(void)
{;
}

static void dummy_discard(void)
{;
}

static void dummy_discard_add(G_GNUC_UNUSED gint player_num,
			      G_GNUC_UNUSED gint discard_num)
{;
}

static void dummy_discard_remove(G_GNUC_UNUSED gint player_num)
{;
}

static void dummy_discard_done(void)
{;
}

static void dummy_gold(void)
{;
}

static void dummy_gold_add(G_GNUC_UNUSED gint player_num,
			   G_GNUC_UNUSED gint gold_num)
{;
}

static void dummy_gold_remove(G_GNUC_UNUSED gint player_num,
			      G_GNUC_UNUSED gint * resources)
{;
}

static void dummy_gold_choose(G_GNUC_UNUSED gint gold_num,
			      G_GNUC_UNUSED const gint * bank)
{;
}

static void dummy_gold_done(void)
{;
}

static void dummy_game_over(G_GNUC_UNUSED gint player_num,
			    G_GNUC_UNUSED gint points)
{;
}

static void dummy_init_game(void)
{;
}

static void dummy_start_game(void)
{;
}

static void dummy_setup(G_GNUC_UNUSED gint num_settlements,
			G_GNUC_UNUSED gint num_roads)
{;
}

static void dummy_quote(G_GNUC_UNUSED gint player_num,
			G_GNUC_UNUSED gint * they_supply,
			G_GNUC_UNUSED gint * they_receive)
{;
}

static void dummy_roadbuilding(G_GNUC_UNUSED gint num_roads)
{;
}

static void dummy_monopoly(void)
{;
}

static void dummy_plenty(G_GNUC_UNUSED const gint * bank)
{;
}

static void dummy_turn(void)
{;
}

static void dummy_player_turn(G_GNUC_UNUSED gint player_num)
{;
}

static void dummy_trade(void)
{;
}

static void dummy_trade_player_end(G_GNUC_UNUSED gint player_num)
{;
}

static void dummy_trade_add_quote(G_GNUC_UNUSED gint player_num,
				  G_GNUC_UNUSED gint quote_num,
				  G_GNUC_UNUSED const gint * they_supply,
				  G_GNUC_UNUSED const gint * they_receive)
{;
}

static void dummy_trade_remove_quote(G_GNUC_UNUSED gint player_num,
				     G_GNUC_UNUSED gint quote_num)
{;
}

static void dummy_trade_domestic(G_GNUC_UNUSED gint partner_num,
				 G_GNUC_UNUSED gint quote_num,
				 G_GNUC_UNUSED const gint * we_supply,
				 G_GNUC_UNUSED const gint * we_receive)
{;
}

static void dummy_trade_maritime(G_GNUC_UNUSED gint ratio,
				 G_GNUC_UNUSED Resource we_supply,
				 G_GNUC_UNUSED Resource we_receive)
{;
}

static void dummy_quote_player_end(G_GNUC_UNUSED gint player_num)
{;
}

static void dummy_quote_add(G_GNUC_UNUSED gint player_num,
			    G_GNUC_UNUSED gint quote_num,
			    G_GNUC_UNUSED const gint * they_supply,
			    G_GNUC_UNUSED const gint * they_receive)
{;
}

static void dummy_quote_remove(G_GNUC_UNUSED gint player_num,
			       G_GNUC_UNUSED gint quote_num)
{;
}

static void dummy_quote_start(void)
{;
}

static void dummy_quote_end(void)
{;
}

static void dummy_quote_monitor(void)
{;
}

static void dummy_quote_trade(G_GNUC_UNUSED gint player_num,
			      G_GNUC_UNUSED gint partner_num,
			      G_GNUC_UNUSED gint quote_num,
			      G_GNUC_UNUSED const gint * they_supply,
			      G_GNUC_UNUSED const gint * they_receive)
{;
}

static void dummy_rolled_dice(G_GNUC_UNUSED gint die1,
			      G_GNUC_UNUSED gint die2,
			      G_GNUC_UNUSED gint player_num)
{;
}

static void dummy_draw_edge(G_GNUC_UNUSED Edge * edge)
{;
}

static void dummy_draw_node(G_GNUC_UNUSED Node * node)
{;
}

static void dummy_bought_develop(G_GNUC_UNUSED DevelType type)
{;
}

static void dummy_played_develop(G_GNUC_UNUSED gint player_num,
				 G_GNUC_UNUSED guint card_idx,
				 G_GNUC_UNUSED DevelType type)
{;
}

static void dummy_resource_change(G_GNUC_UNUSED Resource type,
				  G_GNUC_UNUSED gint num)
{;
}

static void dummy_draw_hex(G_GNUC_UNUSED Hex * hex)
{;
}

static void dummy_update_stock(void)
{;
}

static void dummy_robber(void)
{;
}

static void dummy_robber_moved(G_GNUC_UNUSED Hex * old,
			       G_GNUC_UNUSED Hex * new)
{;
}

static void dummy_steal_building(void)
{;
}

static void dummy_steal_ship(void)
{;
}

static void dummy_robber_done(void)
{;
}

static void dummy_player_robbed(G_GNUC_UNUSED gint robber_num,
				G_GNUC_UNUSED gint victim_num,
				G_GNUC_UNUSED Resource resource)
{;
}

static void dummy_get_rolled_resources(G_GNUC_UNUSED gint player_num,
				       G_GNUC_UNUSED const gint *
				       resources,
				       G_GNUC_UNUSED const gint * wanted)
{;
}

static void dummy_new_statistics(G_GNUC_UNUSED gint player_num,
				 G_GNUC_UNUSED StatisticType type,
				 G_GNUC_UNUSED gint num)
{;
}

static void dummy_new_points(G_GNUC_UNUSED gint player_num,
			     G_GNUC_UNUSED Points * points,
			     G_GNUC_UNUSED gboolean added)
{
}

static void dummy_spectator_name(G_GNUC_UNUSED gint spectator_num,
				 G_GNUC_UNUSED const gchar * name)
{;
}

static void dummy_player_name(G_GNUC_UNUSED gint player_num,
			      G_GNUC_UNUSED const gchar * name)
{;
}

static void dummy_player_style(G_GNUC_UNUSED gint player_num,
			       G_GNUC_UNUSED const gchar * style)
{;
}

static void dummy_player_quit(G_GNUC_UNUSED gint player_num)
{;
}

static void dummy_spectator_quit(G_GNUC_UNUSED gint player_num)
{;
}

static void dummy_incoming_chat(G_GNUC_UNUSED gint player_num,
				G_GNUC_UNUSED const gchar * chat)
{;
}

static void dummy_new_bank(G_GNUC_UNUSED const gint * new_bank)
{;
}

static void dummy_error(G_GNUC_UNUSED const gchar * message)
{;
}

static Map *dummy_get_map(void)
{
	return NULL;
}

static void dummy_set_map(G_GNUC_UNUSED Map * map)
{;
}

/*----------------------------------------------------------------------
 * Entry point for the client state machine
 */
void client_init(void)
{
	/* first set everything to 0, so we are sure it segfaults if
	 * someone forgets to update this when adding a new callback */
	memset(&callbacks, 0, sizeof(callbacks));
	/* set all callbacks to their default value: doing nothing */
	callbacks.init_glib_et_al = &dummy_init_glib_et_al;
	callbacks.init = &dummy_init;
	callbacks.network_status = &dummy_network_status;
	callbacks.instructions = &dummy_instructions;
	callbacks.network_wait = &dummy_network_wait;
	callbacks.offline = &dummy_offline;
	callbacks.discard = &dummy_discard;
	callbacks.discard_add = &dummy_discard_add;
	callbacks.discard_remove = &dummy_discard_remove;
	callbacks.discard_done = &dummy_discard_done;
	callbacks.gold = &dummy_gold;
	callbacks.gold_add = &dummy_gold_add;
	callbacks.gold_remove = &dummy_gold_remove;
	callbacks.gold_choose = &dummy_gold_choose;
	callbacks.gold_done = &dummy_gold_done;
	callbacks.game_over = &dummy_game_over;
	callbacks.init_game = &dummy_init_game;
	callbacks.start_game = &dummy_start_game;
	callbacks.setup = &dummy_setup;
	callbacks.quote = &dummy_quote;
	callbacks.roadbuilding = &dummy_roadbuilding;
	callbacks.monopoly = &dummy_monopoly;
	callbacks.plenty = &dummy_plenty;
	callbacks.turn = &dummy_turn;
	callbacks.player_turn = &dummy_player_turn;
	callbacks.trade = &dummy_trade;
	callbacks.trade_player_end = &dummy_trade_player_end;
	callbacks.trade_add_quote = &dummy_trade_add_quote;
	callbacks.trade_remove_quote = &dummy_trade_remove_quote;
	callbacks.trade_domestic = &dummy_trade_domestic;
	callbacks.trade_maritime = &dummy_trade_maritime;
	callbacks.quote_player_end = &dummy_quote_player_end;
	callbacks.quote_add = &dummy_quote_add;
	callbacks.quote_remove = &dummy_quote_remove;
	callbacks.quote_start = &dummy_quote_start;
	callbacks.quote_end = &dummy_quote_end;
	callbacks.quote_monitor = &dummy_quote_monitor;
	callbacks.quote_trade = &dummy_quote_trade;
	callbacks.rolled_dice = &dummy_rolled_dice;
	callbacks.draw_edge = &dummy_draw_edge;
	callbacks.draw_node = &dummy_draw_node;
	callbacks.bought_develop = &dummy_bought_develop;
	callbacks.played_develop = &dummy_played_develop;
	callbacks.resource_change = &dummy_resource_change;
	callbacks.draw_hex = &dummy_draw_hex;
	callbacks.update_stock = &dummy_update_stock;
	callbacks.robber = &dummy_robber;
	callbacks.robber_moved = &dummy_robber_moved;
	callbacks.steal_building = &dummy_steal_building;
	callbacks.steal_ship = &dummy_steal_ship;
	callbacks.robber_done = &dummy_robber_done;
	callbacks.player_robbed = &dummy_player_robbed;
	callbacks.get_rolled_resources = &dummy_get_rolled_resources;
	callbacks.new_statistics = &dummy_new_statistics;
	callbacks.new_points = &dummy_new_points;
	callbacks.spectator_name = &dummy_spectator_name;
	callbacks.player_name = &dummy_player_name;
	callbacks.player_style = &dummy_player_style;
	callbacks.player_quit = &dummy_player_quit;
	callbacks.spectator_quit = &dummy_spectator_quit;
	callbacks.incoming_chat = &dummy_incoming_chat;
	callbacks.new_bank = &dummy_new_bank;
	callbacks.error = &dummy_error;
	callbacks.get_map = &dummy_get_map;
	callbacks.set_map = &dummy_set_map;
	/* mainloop and quit are not set here */
	resource_init();
}

void client_start(int argc, char **argv)
{
	callbacks.init_glib_et_al(argc, argv);

	requested_name = NOTIFYING_STRING(notifying_string_new());
	requested_style = NOTIFYING_STRING(notifying_string_new());

	callbacks.init();
	sm_goto(SM(), mode_offline);
}

/*----------------------------------------------------------------------
 * The state machine API supports two global event handling callbacks.
 *
 * All events are sent to the global event handler before they are
 * sent to the current state function.  If the global event handler
 * handles the event and returns TRUE, the event will not be sent to
 * the current state function.  is which allow unhandled events to be
 * processed via a callback.
 *
 * If an event is not handled by either the global event handler, or
 * the current state function, then it will be sent to the unhandled
 * event handler.  Using this, the client code implements some of the
 * error handling globally.
 */

/* Global event handler - this get first crack at events.  If we
 * return TRUE, the event will not be passed to the current state
 * function.
 */
static gboolean global_filter(StateMachine * sm, gint event)
{
	switch (event) {
	case SM_NET_CLOSE:
		log_message(MSG_ERROR,
			    _("We have been kicked out of the game.\n"));
		waiting_for_network(FALSE);
		sm_pop_all_and_goto(sm, mode_offline);
		callbacks.network_status(_("Offline"));
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/* Global unhandled event handler - this get called for events that
 * fall through the state machine without being handled.
 */
static gboolean global_unhandled(StateMachine * sm, gint event)
{
	gchar *str;

	switch (event) {
	case SM_NET_CLOSE:
		g_error("SM_NET_CLOSE not caught by global_filter");
	case SM_RECV:
		/* all errors start with ERR */
		if (sm_recv(sm, "ERR %S", &str)) {
			log_message(MSG_ERROR, _("Error (%s): %s\n"),
				    sm_current_name(sm), str);
			callbacks.error(str);
			g_free(str);
			return TRUE;
		}
		/* notices which are not errors should appear in the message
		 * window */
		if (sm_recv(sm, "NOTE %S", &str)) {
			log_message(MSG_ERROR, _("Notice: %s\n"), _(str));
			g_free(str);
			return TRUE;
		}
		/* A notice with 1 argument */
		if (sm_recv(sm, "NOTE1 %S", &str)) {
			gchar *message;
			gchar **parts;

			parts = g_strsplit(str, "|", 2);
			message = g_strdup_printf(_(parts[1]), parts[0]);
			log_message(MSG_ERROR, _("Notice: %s\n"), message);
			g_strfreev(parts);
			g_free(message);
			g_free(str);
			return TRUE;
		}
		/* protocol extensions which may be ignored have this prefix
		 * before the next protocol changing version of the game is
		 * released.  Notify the client about it anyway. */
		if (sm_recv(sm, "extension %S", &str)) {
			log_message(MSG_INFO,
				    "Ignoring extension used by server: %s\n",
				    str);
			g_free(str);
			return TRUE;
		}
		/* we're receiving strange things */
		if (sm_recv(sm, "%S", &str)) {
			log_message(MSG_ERROR,
				    "Unknown message in %s: %s\n",
				    sm_current_name(sm), str);
			g_free(str);
			return TRUE;
		}
		/* this is never reached: everything matches "%S" */
		g_error
		    ("This should not be possible, please report this bug.\n");
	default:
		break;
	}
	/* this may happen, for example when a hotkey is used for a function
	 * which cannot be activated */
	return FALSE;
}

/*----------------------------------------------------------------------
 * Server notifcations about player name changes and chat messages.
 * These can happen in any state (maybe this should be moved to
 * global_filter()?).
 */
static gboolean check_chat_or_name(StateMachine * sm)
{
	gint player_num;
	gchar *str;

	if (sm_recv(sm, "player %d chat %S", &player_num, &str)) {
		callbacks.incoming_chat(player_num, str);
		g_free(str);
		return TRUE;
	}
	if (sm_recv(sm, "player %d is %S", &player_num, &str)) {
		player_change_name(player_num, str);
		g_free(str);
		return TRUE;
	}
	if (sm_recv(sm, "player %d style %S", &player_num, &str)) {
		player_change_style(player_num, str);
		g_free(str);
		return TRUE;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Server notifcations about other players name changes and chat
 * messages.  These can happen in almost any state in which the game
 * is running.
 */
static gboolean check_other_players(StateMachine * sm)
{
	BuildType build_type;
	DevelType devel_type;
	Resource resource_type, supply_type, receive_type;
	gint player_num, victim_num;
	guint card_idx;
	gint backwards;
	gint discard_num, num, ratio, die1, die2, x, y, pos;
	gint id;
	gint resource_list[NO_RESOURCE], wanted_list[NO_RESOURCE];
	gint sx, sy, spos, dx, dy, dpos;
	gchar *str;

	if (check_chat_or_name(sm))
		return TRUE;

	if (!sm_recv_prefix(sm, "player %d ", &player_num))
		return FALSE;

	if (sm_recv(sm, "built %B %d %d %d", &build_type, &x, &y, &pos)) {
		player_build_add(player_num, build_type, x, y, pos, TRUE);
		return TRUE;
	}
	if (sm_recv
	    (sm, "move %d %d %d %d %d %d", &sx, &sy, &spos, &dx, &dy,
	     &dpos)) {
		player_build_move(player_num, sx, sy, spos, dx, dy, dpos,
				  FALSE);
		return TRUE;
	}
	if (sm_recv
	    (sm, "move-back %d %d %d %d %d %d", &sx, &sy, &spos, &dx, &dy,
	     &dpos)) {
		player_build_move(player_num, sx, sy, spos, dx, dy, dpos,
				  TRUE);
		return TRUE;
	}
	if (sm_recv(sm, "remove %B %d %d %d", &build_type, &x, &y, &pos)) {
		player_build_remove(player_num, build_type, x, y, pos);
		return TRUE;
	}
	if (sm_recv(sm, "receives %R %R", resource_list, wanted_list)) {
		gint i;
		for (i = 0; i < NO_RESOURCE; ++i) {
			if (resource_list[i] == wanted_list[i])
				continue;
			if (resource_list[i] == 0) {
				log_message(MSG_RESOURCE,
					    _(""
					      "%s does not receive any %s, because the bank is empty.\n"),
					    player_name(player_num, TRUE),
					    resource_name(i, FALSE));
			} else {
				gint j, list[NO_RESOURCE];
				gchar *buff;
				for (j = 0; j < NO_RESOURCE; ++j)
					list[j] = 0;
				list[i] = resource_list[i];
				resource_list[i] = 0;
				buff = resource_format_num(list);
				log_message(MSG_RESOURCE,
					    _(""
					      "%s only receives %s, because the bank didn't have any more.\n"),
					    player_name(player_num, TRUE),
					    buff);
				g_free(buff);
				resource_apply_list(player_num, list, 1);
			}
		}
		if (resource_count(resource_list) != 0)
			player_resource_action(player_num,
					       _("%s receives %s.\n"),
					       resource_list, 1);
		callbacks.get_rolled_resources(player_num, resource_list,
					       wanted_list);
		return TRUE;
	}
	if (sm_recv(sm, "plenty %R", resource_list)) {
		/* Year of Plenty */
		player_resource_action(player_num, _("%s takes %s.\n"),
				       resource_list, 1);
		return TRUE;
	}
	if (sm_recv(sm, "spent %R", resource_list)) {
		player_resource_action(player_num, _("%s spent %s.\n"),
				       resource_list, -1);
		return TRUE;
	}
	if (sm_recv(sm, "refund %R", resource_list)) {
		player_resource_action(player_num,
				       _("%s is refunded %s.\n"),
				       resource_list, 1);
		return TRUE;
	}
	if (sm_recv(sm, "bought-develop")) {
		develop_bought(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "play-develop %u %D", &card_idx, &devel_type)) {
		develop_played(player_num, card_idx, devel_type);
		return TRUE;
	}
	if (sm_recv(sm, "turn %d", &num)) {
		turn_begin(player_num, num);
		return TRUE;
	}
	if (sm_recv(sm, "shuffled-dice-deck")) {
		/* %s = Player name */
		log_message(MSG_DICE, _("%s shuffled the dice deck.\n"),
			    player_name(player_num, TRUE));
		return TRUE;
	}
	if (sm_recv(sm, "rolled %d %d", &die1, &die2)) {
		turn_rolled_dice(player_num, die1, die2);
		if (die1 + die2 != 7)
			sm_push(sm, mode_wait_resources);
		return TRUE;
	}
	if (sm_recv(sm, "must-discard %d", &discard_num)) {
		waiting_for_network(FALSE);
		sm_push(sm, mode_discard);
		if (player_num == my_player_num())
			callback_mode = MODE_DISCARD;
		callbacks.discard_add(player_num, discard_num);
		return TRUE;
	}
	if (sm_recv(sm, "discarded %R", resource_list)) {
		player_resource_action(player_num, _("%s discarded %s.\n"),
				       resource_list, -1);
		callbacks.discard_remove(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "is-robber")) {
		robber_begin_move(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "moved-robber %d %d", &x, &y)) {
		robber_moved(player_num, x, y, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "moved-pirate %d %d", &x, &y)) {
		pirate_moved(player_num, x, y, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "unmoved-robber %d %d", &x, &y)) {
		robber_moved(player_num, x, y, TRUE);
		return TRUE;
	}
	if (sm_recv(sm, "unmoved-pirate %d %d", &x, &y)) {
		pirate_moved(player_num, x, y, TRUE);
		return TRUE;
	}
	if (sm_recv(sm, "stole from %d", &victim_num)) {
		player_stole_from(player_num, victim_num, NO_RESOURCE);
		return TRUE;
	}
	if (sm_recv(sm, "stole %r from %d", &resource_type, &victim_num)) {
		player_stole_from(player_num, victim_num, resource_type);
		return TRUE;
	}
	if (sm_recv
	    (sm, "monopoly %d %r from %d", &num, &resource_type,
	     &victim_num)) {
		monopoly_player(player_num, victim_num, num,
				resource_type);
		return TRUE;
	}
	if (sm_recv(sm, "largest-army")) {
		player_largest_army(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "longest-road")) {
		player_longest_road(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "get-point %d %d %S", &id, &num, &str)) {
		player_get_point(player_num, id, str, num);
		g_free(str);
		return TRUE;
	}
	if (sm_recv(sm, "lose-point %d", &id)) {
		player_lose_point(player_num, id);
		return TRUE;
	}
	if (sm_recv(sm, "take-point %d %d", &id, &victim_num)) {
		player_take_point(player_num, id, victim_num);
		return TRUE;
	}
	if (sm_recv(sm, "setup %d", &backwards)) {
		setup_begin(player_num);
		if (backwards)
			sm_push(sm, mode_wait_resources);
		return TRUE;
	}
	if (sm_recv(sm, "setup-double")) {
		setup_begin_double(player_num);
		sm_push(sm, mode_wait_resources);
		return TRUE;
	}
	if (sm_recv(sm, "won with %d", &num)) {
		callbacks.game_over(player_num, num);
		log_message(MSG_DICE, _("%s has won the game with %d "
					"victory points!\n"),
			    player_name(player_num, TRUE), num);
		sm_pop_all_and_goto(sm, mode_game_over);
		return TRUE;
	}
	if (sm_recv(sm, "has quit")) {
		player_has_quit(player_num);
		return TRUE;
	}

	if (sm_recv(sm, "maritime-trade %d supply %r receive %r",
		    &ratio, &supply_type, &receive_type)) {
		player_maritime_trade(player_num, ratio, supply_type,
				      receive_type);
		return TRUE;
	}

	sm_cancel_prefix(sm);
	return FALSE;
}

/*----------------------------------------------------------------------
 * State machine state functions.
 *
 * The state machine API works like this:
 *
 * SM_ENTER:
 *   When a state is entered the new state function is called with the
 *   SM_ENTER event.  This allows the client to perform state
 *   initialisation.
 *
 * SM_INIT:
 *   When a state is entered, and every time an event is handled, the
 *   state machine code calls the current state function with an
 *   SM_INIT event.
 *
 * SM_RECV:
 *   Indicates that a message has been received from the server.
 *
 * SM_NET_*:
 *   These are network connection related events.
 *
 * To change current state function, use sm_goto().
 *
 * The state machine API also implements a state stack.  This allows
 * us to reuse parts of the state machine by pushing the current
 * state, and then returning to it when the nested processing is
 * complete.
 *
 * The state machine nesting can be used via sm_push() and sm_pop().
 */

/*----------------------------------------------------------------------
 * Game startup and offline handling
 */

static gboolean mode_offline(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_offline");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_INIT;
		callbacks.offline();
		break;
	default:
		break;
	}
	return FALSE;
}

/* Waiting for connect to complete
 */
gboolean mode_connecting(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_connecting");
	switch (event) {
	case SM_NET_CONNECT:
		sm_goto(sm, mode_start);
		return TRUE;
	case SM_NET_CONNECT_FAIL:
		sm_goto(sm, mode_offline);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/* Handle initial signon message
 */
gboolean mode_start(StateMachine * sm, gint event)
{
	gint player_num, total_num;
	gchar *version;

	sm_state_name(sm, "mode_start");

	if (event == SM_ENTER) {
		callbacks.network_status(_("Loading"));
		player_reset();
		callbacks.init_game();
	}

	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "version report")) {
		sm_send(sm, "version %s\n",
			client_version_type_to_string(LATEST_VERSION));
		return TRUE;
	}
	if (sm_recv(sm, "status report")) {
		gchar *name = notifying_string_get(requested_name);
		if (requested_spectator) {
			if (name && name[0] != '\0') {
				sm_send(sm, "status viewer %s\n", name);
			} else {
				sm_send(sm, "status newviewer\n");
			}
		} else {
			if (name && name[0] != '\0') {
				sm_send(sm, "status reconnect %s\n", name);
			} else {
				sm_send(sm, "status newplayer\n");
			}
		}
		g_free(name);
		return TRUE;
	}
	if (sm_recv(sm, "player %d of %d, welcome to pioneers server %S",
		    &player_num, &total_num, &version)) {
		gchar *style = notifying_string_get(requested_style);

		g_free(version);
		player_set_my_num(player_num);
		player_set_total_num(total_num);
		sm_send(sm, "style %s\n", style);
		sm_send(sm, "players\n");
		sm_goto(sm, mode_players);
		g_free(style);
		return TRUE;
	}
	if (sm_recv(sm, "ERR sorry, version conflict")) {
		sm_pop_all_and_goto(sm, mode_offline);
		callbacks.network_status(_("Offline"));
		callbacks.instructions(_("Version mismatch."));
		log_message(MSG_ERROR,
			    _("Version mismatch. Please make sure client "
			      "and server are up to date.\n"));
		return TRUE;
	}
	return check_chat_or_name(sm);
}

/* Response to "players" command
 */
static gboolean mode_players(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_players");
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "players follow")) {
		sm_goto(sm, mode_player_list);
		return TRUE;
	}
	return check_other_players(sm);
}

/* Handle list of players
 */
static gboolean mode_player_list(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_player_list");
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, ".")) {
		sm_send(sm, "game\n");
		sm_goto(sm, mode_load_game);
		return TRUE;
	}
	return check_other_players(sm);
}

/* Response to "game" command
 */
static gboolean mode_load_game(StateMachine * sm, gint event)
{
	gchar *str;

	sm_state_name(sm, "mode_load_game");
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "game")) {
		if (game_params != NULL) {
			callbacks.set_map(NULL);
			params_free(game_params);
		}
		game_params = params_new();
		return TRUE;
	}
	if (sm_recv(sm, "end")) {
		params_load_finish(game_params);
		callbacks.set_map(game_params->map);
		stock_init();
		develop_init();
		/* initialize global recovery info struct */
		recovery_info.prevstate = NULL;
		recovery_info.turnnum = -1;
		recovery_info.playerturn = -1;
		recovery_info.numdiscards = -1;
		recovery_info.rolled_dice = FALSE;
		recovery_info.die1 = -1;
		recovery_info.die2 = -1;
		recovery_info.bought_develop = FALSE;
		recovery_info.build_list = NULL;
		recovery_info.ship_moved = FALSE;

		sm_send(sm, "gameinfo\n");
		sm_goto(sm, mode_load_gameinfo);
		return TRUE;
	}
	if (check_other_players(sm))
		return TRUE;
	if (sm_recv(sm, "%S", &str)) {
		params_load_line(game_params, str);
		g_free(str);
		return TRUE;
	}
	return FALSE;
}

/* Response to "gameinfo" command
 */
static gboolean mode_load_gameinfo(StateMachine * sm, gint event)
{
	gint x, y, pos, owner;
	static gboolean have_bank = FALSE;
	static gint devcardidx = -1;
	static gint numdevcards = -1;
	gint num_roads, num_bridges, num_ships, num_settlements,
	    num_cities, num_soldiers;
	gint opnum, opnassets, opncards, opnsoldiers;
	gboolean pchapel, puniv, pgov, plibr, pmarket, plongestroad,
	    plargestarmy;
	gint point_id, point_points;
	gchar *point_name;
	DevelType devcard;
	gint devcardturnbought;
	BuildType btype;
	gint resources[NO_RESOURCE];
	gint tmp_bank[NO_RESOURCE];
	gint devbought;

	sm_state_name(sm, "mode_load_gameinfo");
	if (event == SM_ENTER) {
		gint idx;

		have_bank = FALSE;
		for (idx = 0; idx < NO_RESOURCE; ++idx)
			tmp_bank[idx] = game_params->resource_count;
		set_bank(tmp_bank);
	}
	if (event != SM_RECV)
		return FALSE;
	if (sm_recv(sm, "gameinfo")) {
		return TRUE;
	}
	if (sm_recv(sm, ".")) {
		return TRUE;
	}
	if (sm_recv(sm, "end")) {
		callback_mode = MODE_WAIT_TURN;	/* allow chatting */
		callbacks.start_game();
		sm_goto(sm, mode_recovery_wait_start_response);
		return TRUE;
	}
	if (sm_recv(sm, "bank %R", tmp_bank)) {
		set_bank(tmp_bank);
		have_bank = TRUE;
		return TRUE;
	}
	if (sm_recv(sm, "development-bought %d", &devbought)) {
		gint i;
		for (i = 0; i < devbought; i++)
			stock_use_develop();
		return TRUE;
	}
	if (sm_recv(sm, "turn num %d", &recovery_info.turnnum)) {
		return TRUE;
	}
	if (sm_recv(sm, "player turn: %d", &recovery_info.playerturn)) {
		return TRUE;
	}
	if (sm_recv
	    (sm, "dice rolled: %d %d", &recovery_info.die1,
	     &recovery_info.die2)) {
		recovery_info.rolled_dice = TRUE;
		return TRUE;
	}
	if (sm_recv
	    (sm, "dice value: %d %d", &recovery_info.die1,
	     &recovery_info.die2)) {
		return TRUE;
	}
	if (sm_recv(sm, "moved ship")) {
		recovery_info.ship_moved = TRUE;
		return TRUE;
	}
	if (sm_recv(sm, "bought develop")) {
		recovery_info.bought_develop = TRUE;
		return TRUE;
	}
	if (sm_recv(sm, "state %S", &recovery_info.prevstate)) {
		return TRUE;
	}
	if (sm_recv(sm, "playerinfo: resources: %R", resources)) {
		resource_init();
		resource_apply_list(my_player_num(), resources, 1);
		/* If the bank was copied from the server, it should not be
		 * compensated for my own resources, because it was already
		 * correct.  So we compensate it back. */
		if (have_bank)
			modify_bank(resources);
		return TRUE;
	}
	if (sm_recv(sm, "playerinfo: numdevcards: %d", &numdevcards)) {
		devcardidx = 0;
		return TRUE;
	}
	if (sm_recv
	    (sm, "playerinfo: devcard: %d %d", &devcard,
	     &devcardturnbought)) {
		if (devcardidx >= numdevcards) {
			return FALSE;
		}

		stock_dont_count_develop();
		develop_bought_card_turn(devcard, devcardturnbought != 0);

		devcardidx++;
		if (devcardidx >= numdevcards) {
			devcardidx = numdevcards = -1;
		}
		return TRUE;
	}
	if (sm_recv
	    (sm, "playerinfo: %d %d %d %d %d %d %d %d %d %d %d %d %d",
	     &num_roads, &num_bridges, &num_ships, &num_settlements,
	     &num_cities, &num_soldiers, &pchapel, &puniv, &pgov, &plibr,
	     &pmarket, &plongestroad, &plargestarmy)) {
		if (num_soldiers) {
			player_modify_statistic(my_player_num(),
						STAT_SOLDIERS,
						num_soldiers);
		}
		if (pchapel) {
			player_modify_statistic(my_player_num(),
						STAT_CHAPEL, 1);
		}
		if (puniv) {
			player_modify_statistic(my_player_num(),
						STAT_UNIVERSITY, 1);
		}
		if (pgov) {
			player_modify_statistic(my_player_num(),
						STAT_GOVERNORS_HOUSE, 1);
		}
		if (plibr) {
			player_modify_statistic(my_player_num(),
						STAT_LIBRARY, 1);
		}
		if (pmarket) {
			player_modify_statistic(my_player_num(),
						STAT_MARKET, 1);
		}
		if (plongestroad) {
			player_modify_statistic(my_player_num(),
						STAT_LONGEST_ROAD, 1);
		}
		if (plargestarmy) {
			player_modify_statistic(my_player_num(),
						STAT_LARGEST_ARMY, 1);
		}
		return TRUE;
	}
	if (sm_recv
	    (sm, "get-point %d %d %d %S", &opnum, &point_id, &point_points,
	     &point_name)) {
		Points *points =
		    points_new(point_id, point_name, point_points);
		player_modify_points(opnum, points, TRUE);	/* Added */
		return TRUE;
	}

	if (sm_recv
	    (sm, "otherplayerinfo: %d %d %d %d %d %d %d %d %d %d %d",
	     &opnum, &opnassets, &opncards, &opnsoldiers, &pchapel, &puniv,
	     &pgov, &plibr, &pmarket, &plongestroad, &plargestarmy)) {
		player_modify_statistic(opnum, STAT_RESOURCES, opnassets);
		player_modify_statistic(opnum, STAT_DEVELOPMENT, opncards);
		player_modify_statistic(opnum, STAT_SOLDIERS, opnsoldiers);
		if (opnassets != 0)
			g_assert(have_bank);
		if (pchapel) {
			player_modify_statistic(opnum, STAT_CHAPEL, 1);
		}
		if (puniv) {
			player_modify_statistic(opnum, STAT_UNIVERSITY, 1);
		}
		if (pgov) {
			player_modify_statistic(opnum,
						STAT_GOVERNORS_HOUSE, 1);
		}
		if (plibr) {
			player_modify_statistic(opnum, STAT_LIBRARY, 1);
		}
		if (pmarket) {
			player_modify_statistic(opnum, STAT_MARKET, 1);
		}
		if (plongestroad) {
			player_modify_statistic(opnum, STAT_LONGEST_ROAD,
						1);
		}
		if (plargestarmy) {
			player_modify_statistic(opnum, STAT_LARGEST_ARMY,
						1);
		}
		return TRUE;
	}
	if (sm_recv(sm, "buildinfo: %B %d %d %d", &btype, &x, &y, &pos)) {
		BuildRec *rec;
		rec = g_malloc0(sizeof(*rec));
		rec->type = btype;
		rec->x = x;
		rec->y = y;
		rec->pos = pos;
		recovery_info.build_list =
		    g_list_append(recovery_info.build_list, rec);
		return TRUE;
	}
	if (sm_recv(sm, "RO%d,%d", &x, &y)) {
		robber_move_on_map(x, y);
		return TRUE;
	}
	if (sm_recv(sm, "P%d,%d", &x, &y)) {
		pirate_move_on_map(x, y);
		return TRUE;
	}
	if (sm_recv(sm, "S%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_SETTLEMENT, x, y, pos,
				 FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "C%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_CITY, x, y, pos, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "W%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_CITY_WALL, x, y, pos, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "R%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_ROAD, x, y, pos, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "SH%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_SHIP, x, y, pos, FALSE);
		return TRUE;
	}
	if (sm_recv(sm, "B%d,%d,%d,%d", &x, &y, &pos, &owner)) {
		player_build_add(owner, BUILD_BRIDGE, x, y, pos, FALSE);
		return TRUE;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Build command processing
 */

/* Handle response to build command
 */
gboolean mode_build_response(StateMachine * sm, gint event)
{
	BuildType build_type;
	gint x, y, pos;

	sm_state_name(sm, "mode_build_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "built %B %d %d %d",
			    &build_type, &x, &y, &pos)) {
			build_add(build_type, x, y, pos, TRUE);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Handle response to move ship command
 */
gboolean mode_move_response(StateMachine * sm, gint event)
{
	gint sx, sy, spos, dx, dy, dpos;

	sm_state_name(sm, "mode_move_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "move %d %d %d %d %d %d",
			    &sx, &sy, &spos, &dx, &dy, &dpos)) {
			build_move(sx, sy, spos, dx, dy, dpos, FALSE);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Setup phase handling
 */

/* Response to a "done"
 */
gboolean mode_done_response(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_done_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			build_clear();
			waiting_for_network(FALSE);
			/* pop back to parent's parent if "done" worked */
			sm_multipop(sm, 2);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

static char *setup_msg(void)
{
	gchar *msg;
	gchar *old;
	const gchar *parts[3];
	int num_parts;
	int idx;

	if (is_setup_double())
		msg = g_strdup(_("Build two settlements, "
				 "each with a connecting"));
	else
		msg = g_strdup(_("Build a settlement with a connecting"));
	num_parts = 0;
	if (setup_can_build_road())
		parts[num_parts++] = _("road");
	if (setup_can_build_bridge())
		parts[num_parts++] = _("bridge");
	if (setup_can_build_ship())
		parts[num_parts++] = _("ship");

	for (idx = 0; idx < num_parts; idx++) {
		if (idx > 0) {
			if (idx == num_parts - 1) {
				old = msg;
				msg =
				    g_strdup_printf("%s%s", msg, _(" or"));
				g_free(old);
			} else {
				old = msg;
				msg = g_strdup_printf("%s,", msg);
				g_free(old);
			}
		}
		old = msg;
		msg = g_strdup_printf("%s %s", msg, parts[idx]);
		g_free(old);
	}
	old = msg;
	msg = g_strdup_printf("%s.", msg);
	g_free(old);

	return msg;
}

static gboolean mode_setup(StateMachine * sm, gint event)
{
	gint total;
	sm_state_name(sm, "mode_setup");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_SETUP;
		callbacks.instructions(setup_msg());
		total = is_setup_double()? 2 : 1;
		callbacks.setup(total - build_count_settlements(),
				total - build_count_edges());
		break;
	case SM_RECV:
		/* When a line of text comes in from the network, the
		 * state machine will call us with SM_RECV.
		 */
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Game is up and running - waiting for our turn
 */

/* Waiting for your turn to come around
 */
static gboolean mode_idle(StateMachine * sm, gint event)
{
	gint num, player_num, backwards;
	gint they_supply[NO_RESOURCE];
	gint they_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_idle");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_WAIT_TURN;
		if (player_is_spectator(my_player_num()))
			callbacks.instructions("");
		else
			callbacks.instructions(_
					       ("Waiting for your turn."));
		break;
	case SM_RECV:
		if (sm_recv(sm, "setup %d", &backwards)) {
			setup_begin(my_player_num());
			if (backwards)
				sm_push_noenter(sm, mode_wait_resources);
			sm_push(sm, mode_setup);
			return TRUE;
		}
		if (sm_recv(sm, "setup-double")) {
			setup_begin_double(my_player_num());
			sm_push_noenter(sm, mode_wait_resources);
			sm_push(sm, mode_setup);
			return TRUE;
		}
		if (sm_recv(sm, "turn %d", &num)) {
			turn_begin(my_player_num(), num);
			sm_push(sm, mode_turn);
			return TRUE;
		}
		if (sm_recv
		    (sm,
		     "player %d domestic-trade call supply %R receive %R",
		     &player_num, they_supply, they_receive)) {
			sm_push(sm, mode_domestic_quote);
			callbacks.quote(player_num, they_supply,
					they_receive);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Nested state machine for robber handling
 */

/* Get user to steal from a building
 */
static gboolean mode_robber_steal_building(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_robber_steal_building");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_ROB;
		callbacks.instructions(_
				       ("Select the building to steal from."));
		callbacks.steal_building();
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/* Get user to steal from a ship
 */
static gboolean mode_robber_steal_ship(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_robber_steal_ship");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_ROB;
		callbacks.instructions(_
				       ("Select the ship to steal from."));
		callbacks.steal_ship();
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/* Handle response to move robber
 */
gboolean mode_robber_move_response(StateMachine * sm, gint event)
{
	gint x, y;

	sm_state_name(sm, "mode_robber_move_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "robber-done")) {
			waiting_for_network(FALSE);
			sm_multipop(sm, 2);
			callbacks.robber_done();
			return TRUE;
		}
		if (sm_recv(sm, "rob %d %d", &x, &y)) {
			const Hex *hex;
			waiting_for_network(FALSE);
			hex = map_hex_const(callbacks.get_map(), x, y);
			if (hex->terrain == SEA_TERRAIN)
				sm_push(sm, mode_robber_steal_ship);
			else
				sm_push(sm, mode_robber_steal_building);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Wait for server to say robber-done */
gboolean mode_robber_response(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_robber_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "robber-done")) {
			waiting_for_network(FALSE);
			/* current state is response
			 * parent is steal
			 * parent is move_response
			 * parent is mode_robber
			 * all four must be popped.
			 */
			sm_multipop(sm, 4);
			callbacks.robber_done();
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Get user to place robber
 */
static gboolean mode_robber(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_robber");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_ROBBER;
		callbacks.instructions(_("Place the robber."));
		robber_begin_move(my_player_num());
		callbacks.robber();
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/* We rolled a 7, or played a soldier card - any time now the server
 * is going to tell us to place the robber.  Going into this state as
 * soon as we roll a 7 stops a race condition where the user presses a
 * GUI control in the window between receiving the die roll result and
 * the command to enter robber mode.
 */
static gboolean mode_wait_for_robber(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_wait_for_robber");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "you-are-robber")) {
			waiting_for_network(FALSE);
			sm_goto(sm, mode_robber);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Road building
 */

const gchar *road_building_message(gint build_amount)
{
	switch (build_amount) {
	case 0:
		return _("Finish the road building action.");
	case 1:
		return _("Build one road segment.");
	case 2:
		return _("Build two road segments.");
	default:
		g_error("Unknown road building amount");
		return "";
	};
}

static gboolean mode_road_building(StateMachine * sm, gint event)
{
	gint build_amount;	/* The amount of available 'roads' */
	sm_state_name(sm, "mode_road_building");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_ROAD_BUILD;
		/* Determine the possible amount of road segments */
		build_amount = 0;
		if (road_building_can_build_road())
			build_amount += stock_num_roads();
		if (road_building_can_build_ship())
			build_amount += stock_num_ships();
		if (road_building_can_build_bridge())
			build_amount += stock_num_bridges();
		/* Now determine the amount of segments left to play */
		build_amount = MIN(build_amount, 2 - build_count_edges());
		callbacks.roadbuilding(build_amount);
		callbacks.instructions(road_building_message
				       (build_amount));
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Monopoly development card
 */

/* Response to "monopoly"
 */
gboolean mode_monopoly_response(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_monopoly_response");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_MONOPOLY_RESPONSE;
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			waiting_for_network(FALSE);
			/* pop to parent's parent if it worked */
			sm_multipop(sm, 2);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

static gboolean mode_monopoly(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_monopoly");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_MONOPOLY;
		callbacks.monopoly();
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Year of Plenty development card
 */

/* Response to "plenty"
 */
gboolean mode_year_of_plenty_response(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_year_of_plenty_response");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_PLENTY_RESPONSE;
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			waiting_for_network(FALSE);
			/* action is done, go to parent's parent */
			sm_multipop(sm, 2);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

static gboolean mode_year_of_plenty(StateMachine * sm, gint event)
{
	gint plenty[NO_RESOURCE];

	sm_state_name(sm, "mode_year_of_plenty");
	switch (event) {
	case SM_RECV:
		if (sm_recv(sm, "plenty %R", plenty)) {
			callback_mode = MODE_PLENTY;
			callbacks.plenty(plenty);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Nested state machine for handling development card play response
 */

/* Handle response to play develop card
 */
gboolean mode_play_develop_response(StateMachine * sm, gint event)
{
	guint card_idx;
	DevelType card_type;

	sm_state_name(sm, "mode_play_develop_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv
		    (sm, "play-develop %u %D", &card_idx, &card_type)) {
			build_clear();
			waiting_for_network(FALSE);
			develop_played(my_player_num(), card_idx,
				       card_type);
			/* This mode should be popped off after the response
			 * has been handled.  However, for the development
			 * card, a new mode must be pushed immediately.  Due
			 * to the lack of sm_pop_noenter this is combined as
			 * sm_goto */
			switch (card_type) {
			case DEVEL_ROAD_BUILDING:
				sm_goto(sm, mode_road_building);
				break;
			case DEVEL_MONOPOLY:
				sm_goto(sm, mode_monopoly);
				break;
			case DEVEL_YEAR_OF_PLENTY:
				sm_goto(sm, mode_year_of_plenty);
				break;
			case DEVEL_SOLDIER:
				sm_goto(sm, mode_wait_for_robber);
				break;
			default:
				sm_pop(sm);
				break;
			}
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Nested state machine for handling resource card discards.  We enter
 * discard mode whenever any player has to discard resources.
 *
 * When in discard mode, a section of the GUI changes to list all
 * players who must discard resources.  This is important because if
 * during our turn we roll 7, but have less than 7 resources, we do
 * not have to discard.  The list tells us which players have still
 * not discarded resources.
 */
static gboolean mode_discard(StateMachine * sm, gint event)
{
	gint player_num, discard_num;

	sm_state_name(sm, "mode_discard");
	switch (event) {
	case SM_ENTER:
		if (callback_mode != MODE_DISCARD
		    && callback_mode != MODE_DISCARD_WAIT) {
			previous_mode = callback_mode;
			callback_mode = MODE_DISCARD_WAIT;
		}
		callbacks.discard();
		break;
	case SM_RECV:
		if (sm_recv(sm, "player %d must-discard %d",
			    &player_num, &discard_num)) {
			if (player_num == my_player_num())
				callback_mode = MODE_DISCARD;
			callbacks.discard_add(player_num, discard_num);
			return TRUE;
		}
		if (sm_recv(sm, "discard-done")) {
			callback_mode = previous_mode;
			callbacks.discard_done();
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Turn mode processing - before dice have been rolled
 */

/* Handle response to "roll dice"
 */
gboolean mode_roll_response(StateMachine * sm, gint event)
{
	gint die1, die2;

	sm_state_name(sm, "mode_roll_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "rolled %d %d", &die1, &die2)) {
			turn_rolled_dice(my_player_num(), die1, die2);
			waiting_for_network(FALSE);
			sm_goto_noenter(sm, mode_turn_rolled);
			if (die1 + die2 == 7) {
				sm_push(sm, mode_wait_for_robber);
			} else
				sm_push(sm, mode_wait_resources);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

static gboolean mode_turn(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_turn");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_TURN;
		callbacks.instructions(_("It is your turn."));
		callbacks.turn();
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Turn mode processing - after dice have been rolled
 */

/* Handle response to buy development card
 */
gboolean mode_buy_develop_response(StateMachine * sm, gint event)
{
	DevelType card_type;

	sm_state_name(sm, "mode_buy_develop_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "bought-develop %D", &card_type)) {
			develop_bought_card(card_type);
			sm_pop(sm);
			waiting_for_network(FALSE);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Response to "undo"
 */
gboolean mode_undo_response(StateMachine * sm, gint event)
{
	BuildType build_type;
	gint x, y, pos;
	gint sx, sy, spos, dx, dy, dpos;

	sm_state_name(sm, "mode_undo_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "remove %B %d %d %d",
			    &build_type, &x, &y, &pos)) {
			build_remove(build_type, x, y, pos);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (sm_recv(sm, "move-back %d %d %d %d %d %d",
			    &sx, &sy, &spos, &dx, &dy, &dpos)) {
			build_move(sx, sy, spos, dx, dy, dpos, TRUE);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (sm_recv(sm, "undo-robber")) {
			waiting_for_network(FALSE);
			/* current state is undo-response
			 * parent is steal
			 * parent is move_response
			 * parent is mode_robber
			 * the first three must be popped.
			 */
			sm_multipop(sm, 3);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

static gboolean mode_turn_rolled(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_turn_rolled");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_TURN;
		callbacks.instructions(_("It is your turn."));
		callbacks.turn();
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Trade processing - all trading done inside a nested state machine
 * to allow trading to be invoked from multiple states.
 */

static gboolean check_trading(StateMachine * sm)
{
	gint player_num, quote_num;
	gint they_supply[NO_RESOURCE];
	gint they_receive[NO_RESOURCE];

	if (!sm_recv_prefix(sm, "player %d ", &player_num))
		return FALSE;

	if (sm_recv(sm, "domestic-quote finish")) {
		callbacks.trade_player_end(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-quote quote %d supply %R receive %R",
		    &quote_num, they_supply, they_receive)) {
		callbacks.trade_add_quote(player_num, quote_num,
					  they_supply, they_receive);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-quote delete %d", &quote_num)) {
		callbacks.trade_remove_quote(player_num, quote_num);
		return TRUE;
	}

	sm_cancel_prefix(sm);
	return FALSE;
}

/* Handle response to call for domestic trade quotes
 */
gboolean mode_trade_call_response(StateMachine * sm, gint event)
{
	gint we_supply[NO_RESOURCE];
	gint we_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_trade_call_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-trade call supply %R receive %R",
			    we_supply, we_receive)) {
			waiting_for_network(FALSE);
			/* pop response state + push trade state == goto */
			sm_goto(sm, mode_domestic_trade);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Handle response to maritime trade
 */
gboolean mode_trade_maritime_response(StateMachine * sm, gint event)
{
	gint ratio;
	Resource we_supply;
	Resource we_receive;

	Resource no_receive;

	sm_state_name(sm, "mode_trade_maritime_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		/* Handle out-of-resource-cards */
		if (sm_recv(sm, "ERR no-cards %r", &no_receive)) {
			gchar *buf_receive;

			buf_receive = resource_cards(0, no_receive);
			log_message(MSG_TRADE, _("Sorry, %s available.\n"),
				    buf_receive);
			g_free(buf_receive);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (sm_recv(sm, "maritime-trade %d supply %r receive %r",
			    &ratio, &we_supply, &we_receive)) {
			player_maritime_trade(my_player_num(), ratio,
					      we_supply, we_receive);
			waiting_for_network(FALSE);
			sm_pop(sm);
			callbacks.trade_maritime(ratio, we_supply,
						 we_receive);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Handle response to call for quotes during domestic trade
 */
gboolean mode_trade_call_again_response(StateMachine * sm, gint event)
{
	gint we_supply[NO_RESOURCE];
	gint we_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_trade_call_again_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-trade call supply %R receive %R",
			    we_supply, we_receive)) {
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Handle response to domestic trade
 */
gboolean mode_trade_domestic_response(StateMachine * sm, gint event)
{
	gint partner_num;
	gint quote_num;
	gint they_supply[NO_RESOURCE];
	gint they_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_trade_domestic_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv
		    (sm,
		     "domestic-trade accept player %d quote %d supply %R receive %R",
		     &partner_num, &quote_num, &they_supply,
		     &they_receive)) {
			player_domestic_trade(my_player_num(), partner_num,
					      they_supply, they_receive);
			waiting_for_network(FALSE);
			callbacks.trade_domestic(partner_num, quote_num,
						 they_receive,
						 they_supply);
			sm_pop(sm);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Handle response to domestic trade finish
 */
gboolean mode_domestic_finish_response(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_domestic_finish_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-trade finish")) {
			callback_mode = MODE_TURN;
			waiting_for_network(FALSE);
			/* pop to parent's parent on finish */
			sm_multipop(sm, 2);
			return TRUE;
		}
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

static gboolean mode_domestic_trade(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_domestic_trade");
	switch (event) {
	case SM_ENTER:
		if (callback_mode != MODE_DOMESTIC) {
			callback_mode = MODE_DOMESTIC;
		}
		callbacks.trade();
		break;
	case SM_RECV:
		if (check_trading(sm) || check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * Quote processing - all quoting done inside a nested state machine.
 */

static gboolean check_quoting(StateMachine * sm, gint exitdepth,
			      gboolean monitor)
{
	gint player_num, partner_num, quote_num;
	gint they_supply[NO_RESOURCE];
	gint they_receive[NO_RESOURCE];

	if (!sm_recv_prefix(sm, "player %d ", &player_num))
		return FALSE;

	if (sm_recv(sm, "domestic-quote finish")) {
		callbacks.quote_player_end(player_num);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-quote quote %d supply %R receive %R",
		    &quote_num, they_supply, they_receive)) {
		callbacks.quote_add(player_num, quote_num, they_supply,
				    they_receive);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-quote delete %d", &quote_num)) {
		callbacks.quote_remove(player_num, quote_num);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-trade call supply %R receive %R",
		    they_supply, they_receive)) {
		if (monitor)
			sm_pop(sm);
		callbacks.quote(player_num, they_supply, they_receive);
		return TRUE;
	}
	if (sm_recv
	    (sm,
	     "domestic-trade accept player %d quote %d supply %R receive %R",
	     &partner_num, &quote_num, they_supply, they_receive)) {
		player_domestic_trade(player_num, partner_num, they_supply,
				      they_receive);
		callbacks.quote_trade(player_num, partner_num, quote_num,
				      they_supply, they_receive);
		return TRUE;
	}
	if (sm_recv(sm, "domestic-trade finish")) {
		callback_mode = previous_mode;
		callbacks.quote_end();
		sm_multipop(sm, exitdepth);
		return TRUE;
	}

	sm_cancel_prefix(sm);
	return FALSE;
}

/* Handle response to domestic quote finish
 */
gboolean mode_quote_finish_response(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_quote_finish_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-quote finish")) {
			waiting_for_network(FALSE);
			/* pop response + push monitor == goto */
			sm_goto(sm, mode_domestic_monitor);
			callbacks.quote_monitor();
			return TRUE;
		}
		if (check_quoting(sm, 2, FALSE) || check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Handle response to domestic quote submit
 */
gboolean mode_quote_submit_response(StateMachine * sm, gint event)
{
	gint quote_num;
	gint we_supply[NO_RESOURCE];
	gint we_receive[NO_RESOURCE];

	sm_state_name(sm, "mode_quote_submit_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv
		    (sm, "domestic-quote quote %d supply %R receive %R",
		     &quote_num, we_supply, we_receive)) {
			callbacks.quote_add(my_player_num(), quote_num,
					    we_supply, we_receive);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_quoting(sm, 2, FALSE) || check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Handle response to domestic quote delete
 */
gboolean mode_quote_delete_response(StateMachine * sm, gint event)
{
	gint quote_num;

	sm_state_name(sm, "mode_quote_delete_response");
	switch (event) {
	case SM_ENTER:
		waiting_for_network(TRUE);
		break;
	case SM_RECV:
		if (sm_recv(sm, "domestic-quote delete %d", &quote_num)) {
			callbacks.quote_remove(my_player_num(), quote_num);
			waiting_for_network(FALSE);
			sm_pop(sm);
			return TRUE;
		}
		if (check_quoting(sm, 2, FALSE) || check_other_players(sm))
			return TRUE;
		break;
	}
	return FALSE;
}

/* Another player has called for quotes for domestic trade
 */
static gboolean mode_domestic_quote(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_domestic_quote");
	switch (event) {
	case SM_ENTER:
		if (callback_mode != MODE_QUOTE) {
			previous_mode = callback_mode;
			callback_mode = MODE_QUOTE;
			callbacks.quote_start();
		}
		break;
	case SM_RECV:
		if (check_quoting(sm, 1, FALSE) || check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/* We have rejected domestic trade, now just monitor
 */
static gboolean mode_domestic_monitor(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_domestic_monitor");
	switch (event) {
	case SM_RECV:
		if (check_quoting(sm, 2, TRUE) || check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

/*----------------------------------------------------------------------
 * The game is over
 */

static gboolean mode_game_over(StateMachine * sm, gint event)
{
	sm_state_name(sm, "mode_game_over");
	switch (event) {
	case SM_ENTER:
		callback_mode = MODE_GAME_OVER;
		callbacks.instructions(_("The game is over."));
		break;
	case SM_RECV:
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

static gboolean mode_recovery_wait_start_response(StateMachine * sm,
						  gint event)
{
	sm_state_name(sm, "mode_recovery_wait_start_response");
	switch (event) {
	case SM_ENTER:
		sm_send(sm, "start\n");
		break;
	case SM_RECV:
		if (sm_recv(sm, "OK")) {
			recover_from_disconnect(sm, &recovery_info);
			return TRUE;
		}
		return check_other_players(sm);
	default:
		break;
	}
	return FALSE;
}

static void recover_from_disconnect(StateMachine * sm,
				    struct recovery_info_t *rinfo)
{
	StateFunc modeturn;
	GList *next;
	guint num_playable_cards = get_num_playable_cards();

	callbacks.start_game();
	if (rinfo->turnnum > 0)
		turn_begin(rinfo->playerturn, rinfo->turnnum);
	if (rinfo->rolled_dice) {
		turn_rolled_dice(rinfo->playerturn, rinfo->die1,
				 rinfo->die2);
	} else if (rinfo->die1 + rinfo->die2 > 1) {
		callbacks.rolled_dice(rinfo->die1, rinfo->die2,
				      rinfo->playerturn);
	}

	if (rinfo->rolled_dice)
		modeturn = mode_turn_rolled;
	else
		modeturn = mode_turn;

	if (rinfo->bought_develop) {
		develop_reset_have_played_bought(rinfo->bought_develop,
						 num_playable_cards);
	}

	/* setup_begin must be called before the build list is created,
	 * because it contains a call to build_clear() */
	if (strcmp(rinfo->prevstate, "SETUP") == 0 ||
	    strcmp(rinfo->prevstate, "RSETUP") == 0 ||
	    strcmp(rinfo->prevstate, "SETUPDOUBLE") == 0) {
		if (strcmp(rinfo->prevstate, "SETUPDOUBLE") == 0) {
			setup_begin_double(my_player_num());
		} else {
			setup_begin(my_player_num());
		}
	}

	/* The build list must be created before the state is entered,
	 * because when the state is entered the frontend is called and
	 * it will want to have the build list present. */
	if (rinfo->build_list) {
		for (next = rinfo->build_list; next != NULL;
		     next = g_list_next(next)) {
			BuildRec *build = (BuildRec *) next->data;
			build_add(build->type, build->x, build->y,
				  build->pos, FALSE);
		}
		rinfo->build_list = buildrec_free(rinfo->build_list);
	}

	if (strcmp(rinfo->prevstate, "PREGAME") == 0) {
		sm_goto(sm, mode_idle);
	} else if (strcmp(rinfo->prevstate, "IDLE") == 0) {
		sm_goto(sm, mode_idle);
	} else if (strcmp(rinfo->prevstate, "SETUP") == 0 ||
		   strcmp(rinfo->prevstate, "RSETUP") == 0 ||
		   strcmp(rinfo->prevstate, "SETUPDOUBLE") == 0) {
		sm_goto_noenter(sm, mode_idle);
		if (strcmp(rinfo->prevstate, "SETUP") != 0) {
			sm_push_noenter(sm, mode_wait_resources);
		}
		sm_push(sm, mode_setup);
	} else if (strcmp(rinfo->prevstate, "TURN") == 0) {
		sm_goto_noenter(sm, mode_idle);
		sm_push(sm, modeturn);
	} else if (strcmp(rinfo->prevstate, "YOUAREROBBER") == 0) {
		sm_goto_noenter(sm, mode_idle);
		sm_push_noenter(sm, modeturn);
		sm_push(sm, mode_robber);
	} else if (strcmp(rinfo->prevstate, "DISCARD") == 0) {
		sm_goto_noenter(sm, mode_idle);
		if (my_player_num() == rinfo->playerturn) {
			sm_push_noenter(sm, mode_turn_rolled);
			sm_push_noenter(sm, mode_wait_for_robber);
		}
		/* Allow gui to fill previous_state when entering
		 * mode_discard.  */
		callbacks.turn();
		sm_push(sm, mode_discard);
	} else if (strcmp(rinfo->prevstate, "MONOPOLY") == 0) {
		sm_goto_noenter(sm, mode_idle);
		sm_push_noenter(sm, modeturn);
		callback_mode = MODE_TURN;
		sm_push(sm, mode_monopoly);
	} else if (strcmp(rinfo->prevstate, "PLENTY") == 0) {
		sm_goto_noenter(sm, mode_idle);
		sm_push_noenter(sm, modeturn);
		callback_mode = MODE_TURN;
		sm_push(sm, mode_year_of_plenty);
	} else if (strcmp(rinfo->prevstate, "GOLD") == 0) {
		sm_goto_noenter(sm, mode_idle);
		sm_push_noenter(sm, modeturn);
		callback_mode = MODE_TURN;
		sm_push(sm, mode_wait_resources);
	} else if (strcmp(rinfo->prevstate, "ROADBUILDING") == 0) {
		sm_goto_noenter(sm, mode_idle);
		sm_push_noenter(sm, modeturn);
		callback_mode = MODE_TURN;
		sm_push(sm, mode_road_building);
	} else
		g_warning("Not entering any state after reconnect, "
			  "please report this as a bug.  "
			  "Should enter state \"%s\"", rinfo->prevstate);
	g_free(rinfo->prevstate);
}

/*----------------------------------------------------------------------
 * Nested state machine for handling resource card distribution.  We enter
 * here whenever resources might be distributed.
 *
 * This also includes choosing gold.  Gold-choose mode is entered the first
 * time prepare-gold is received.
 *
 * When in gold-choose mode, as in discard mode, a section of the GUI
 * changes to list all players who must choose resources.  This is
 * important because if during our turn we do not receive gold, but others
 * do, the list tells us which players have still not chosen resources.
 * Only the top player in the list can actually choose, the rest is waiting
 * for their turn.
 */
static gboolean mode_wait_resources(StateMachine * sm, gint event)
{
	gint resource_list[NO_RESOURCE], bank[NO_RESOURCE];
	gint player_num, gold_num;

	sm_state_name(sm, "mode_wait_resources");
	switch (event) {
	case SM_RECV:
		if (sm_recv(sm, "player %d prepare-gold %d", &player_num,
			    &gold_num)) {
			if (callback_mode != MODE_GOLD
			    && callback_mode != MODE_GOLD_WAIT) {
				previous_mode = callback_mode;
				callback_mode = MODE_GOLD_WAIT;
				callbacks.gold();
			}
			callbacks.gold_add(player_num, gold_num);
			return TRUE;
		}
		if (sm_recv(sm, "choose-gold %d %R", &gold_num, &bank)) {
			callback_mode = MODE_GOLD;
			callbacks.gold_choose(gold_num, bank);
			return TRUE;
		}
		if (sm_recv(sm, "player %d receive-gold %R",
			    &player_num, resource_list)) {
			player_resource_action(player_num,
					       _("%s takes %s.\n"),
					       resource_list, 1);
			callbacks.gold_remove(player_num, resource_list);
			return TRUE;
		}
		if (sm_recv(sm, "done-resources")) {
			if (callback_mode == MODE_GOLD
			    || callback_mode == MODE_GOLD_WAIT) {
				callback_mode = previous_mode;
				callbacks.gold_done();
			}
			sm_pop(sm);
			return TRUE;
		}
		if (check_other_players(sm))
			return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

gboolean can_trade_domestic(void)
{
	return game_params->domestic_trade;
}

gboolean can_trade_maritime(void)
{
	MaritimeInfo info;
	gint idx;
	gboolean can_trade;
	/* We are not allowed to trade before we have rolled the dice,
	 * or after we have done built a settlement / city, or after
	 * buying a development card.  */
	if (!have_rolled_dice()
	    || (game_params->strict_trade
		&& (have_built() || have_bought_develop())))
		return FALSE;
	can_trade = FALSE;
	/* Check if we can do a maritime trade */
	map_maritime_info(callbacks.get_map(), &info, my_player_num());
	for (idx = 0; idx < NO_RESOURCE; idx++)
		if (info.specific_resource[idx]
		    && resource_asset(idx) >= 2) {
			can_trade = TRUE;
			break;
		} else if (info.any_resource && resource_asset(idx) >= 3) {
			can_trade = TRUE;
			break;
		} else if (resource_asset(idx) >= 4) {
			can_trade = TRUE;
			break;
		}
	return can_trade;
}
