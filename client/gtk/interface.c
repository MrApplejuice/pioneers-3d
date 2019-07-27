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

/* This file consists of gui state functions and callbacks to set them */
#include "config.h"
#include "frontend.h"
#include "cost.h"
#include "histogram.h"
#include "audio.h"
#include "notification.h"
#include "client.h"

/* local functions */
static void frontend_state_robber(GuiEvent event);
static void frontend_state_turn(GuiEvent event);
static void build_road_cb(MapElement edge, MapElement extra);
static void build_ship_cb(MapElement edge, MapElement extra);
static void build_bridge_cb(MapElement edge, MapElement extra);
static void move_ship_cb(MapElement edge, MapElement extra);
static void build_settlement_cb(MapElement node, MapElement extra);
static void build_city_cb(MapElement node, MapElement extra);
static void build_city_wall_cb(MapElement node, MapElement extra);

static void dummy_state(G_GNUC_UNUSED GuiEvent event)
{
}

/* for gold and discard, remember the previous gui state */
static GuiState previous_state = dummy_state;

static gboolean discard_busy = FALSE, robber_busy = FALSE;

static GtkWidget *game_over_dlg = NULL;

void frontend_init_game(void)
{
	notification_close();
	player_clear_summary();
	chat_clear_names();
	develop_reset();
	histogram_reset();
	gui_reset();
	if (game_over_dlg != NULL) {
		gtk_widget_destroy(game_over_dlg);
	}
}

static void frontend_state_idle(G_GNUC_UNUSED GuiEvent event)
{
	/* don't react on any event when idle. */
	/* (except of course chat and name change events, but they are
	 * handled in route_event) */
}

void build_road_cb(MapElement edge, G_GNUC_UNUSED MapElement extra)
{
	cb_build_road(edge.edge);
}

void build_ship_cb(MapElement edge, G_GNUC_UNUSED MapElement extra)
{
	cb_build_ship(edge.edge);
}

static void do_move_ship_cb(MapElement edge, MapElement ship_from)
{
	cb_move_ship(ship_from.edge, edge.edge);
	gui_prompt_hide();
}

/** Edge cursor check function.
 *
 * Determine whether or not a ship can be moved to this edge by the
 * specified player.  Perform the following checks:
 * 1 - Ship cannot be moved to where it comes from
 * 2 - A ship must be buildable at the destination if the ship is moved away
 *     from its current location.
 */
static gboolean can_ship_be_moved_to(MapElement ship_to,
				     G_GNUC_UNUSED gint owner,
				     MapElement ship_from)
{
	return can_move_ship(ship_from.edge, ship_to.edge);
}

void build_bridge_cb(MapElement edge, G_GNUC_UNUSED MapElement extra)
{
	cb_build_bridge(edge.edge);
}

static void cancel_move_ship_cb(void)
{
	callbacks.instructions(_("Ship movement canceled."));
	gui_prompt_hide();
}

void move_ship_cb(MapElement edge, G_GNUC_UNUSED MapElement extra)
{
	MapElement ship_from;
	ship_from.edge = edge.edge;
	callbacks.instructions(_("Select a new location for the ship."));
	gui_prompt_show(_("Select a new location for the ship."));
	gui_cursor_set(SHIP_CURSOR, can_ship_be_moved_to, do_move_ship_cb,
		       cancel_move_ship_cb, &ship_from);
}

void build_settlement_cb(MapElement node, G_GNUC_UNUSED MapElement extra)
{
	cb_build_settlement(node.node);
}

void build_city_cb(MapElement node, G_GNUC_UNUSED MapElement extra)
{
	cb_build_city(node.node);
}

void build_city_wall_cb(MapElement node, G_GNUC_UNUSED MapElement extra)
{
	cb_build_city_wall(node.node);
}

/* trade */
static void frontend_state_trade(GuiEvent event)
{
	static gboolean trading = FALSE;
	const QuoteInfo *quote;
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_TRADE_CALL, can_call_for_quotes());
		frontend_gui_check(GUI_TRADE_ACCEPT,
				   trade_valid_selection());
		frontend_gui_check(GUI_TRADE_FINISH, TRUE);
		frontend_gui_check(GUI_TRADE, TRUE);
		gui_cursor_none();	/* Finish single click build */
		break;
	case GUI_TRADE_CALL:
		trading = TRUE;
		trade_new_trade();
		cb_domestic(trade_we_supply(), trade_we_receive());
		return;
	case GUI_TRADE_ACCEPT:
		quote = trade_current_quote();
		g_assert(quote != NULL);
		if (quote->is_domestic) {
			trade_perform_domestic(my_player_num(),
					       quote->var.d.player_num,
					       quote->var.d.quote_num,
					       quote->var.d.supply,
					       quote->var.d.receive);
		} else {
			trade_perform_maritime(quote->var.m.ratio,
					       quote->var.m.supply,
					       quote->var.m.receive);
		}
		return;
	case GUI_TRADE_FINISH:
	case GUI_TRADE:
		/* stop trading.  Only let the network know about it if it
		 * knew we were trading in the first place. */
		if (trading)
			cb_end_trade();
		trading = FALSE;
		trade_finish();
		set_gui_state(frontend_state_turn);
		return;
	default:
		break;
	}
}

void frontend_trade_add_quote(int player_num, int quote_num,
			      const gint * they_supply,
			      const gint * they_receive)
{
	trade_add_quote(player_num, quote_num, they_supply, they_receive);
	frontend_gui_update();
}

void frontend_trade_remove_quote(int player_num, int quote_num)
{
	trade_delete_quote(player_num, quote_num);
	frontend_gui_update();
}

void frontend_trade_player_end(gint player_num)
{
	trade_player_finish(player_num);
	frontend_gui_update();
}

static void frontend_state_quote(GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_QUOTE_SUBMIT, can_submit_quote());
		frontend_gui_check(GUI_QUOTE_DELETE, can_delete_quote());
		frontend_gui_check(GUI_QUOTE_REJECT, can_reject_quote());
		break;
	case GUI_QUOTE_SUBMIT:
		cb_quote(quote_next_num(), quote_we_supply(),
			 quote_we_receive());
		return;
	case GUI_QUOTE_DELETE:
		cb_delete_quote(quote_current_quote()->var.d.quote_num);
		return;
	case GUI_QUOTE_REJECT:
		quote_player_finish(my_player_num());
		cb_end_quote();
		return;
	default:
		break;
	}
}

void frontend_quote(gint player_num, gint * they_supply,
		    gint * they_receive)
{
	if (get_gui_state() == frontend_state_quote) {
		quote_begin_again(player_num, they_supply, they_receive);
	} else {
		quote_begin(player_num, they_supply, they_receive);
		set_gui_state(frontend_state_quote);
	}
	frontend_gui_update();
}

void frontend_quote_add(int player_num, int quote_num,
			const gint * they_supply,
			const gint * they_receive)
{
	quote_add_quote(player_num, quote_num, they_supply, they_receive);
	frontend_gui_update();
}

void frontend_quote_remove(int player_num, int quote_num)
{
	quote_delete_quote(player_num, quote_num);
	frontend_gui_update();
}

void frontend_quote_player_end(gint player_num)
{
	quote_player_finish(player_num);
	frontend_gui_update();
}

void frontend_quote_end(void)
{
	if (get_gui_state() == frontend_state_quote) {
		quote_finish();
		set_gui_state(frontend_state_idle);
	}
}

void frontend_quote_start(void)
{
	/*set_gui_state (frontend_state_quote); */
}

void frontend_quote_monitor(void)
{
}

static gboolean check_road(MapElement element, gint owner,
			   G_GNUC_UNUSED MapElement extra)
{
	return can_road_be_built(element.edge, owner);
}

static gboolean check_ship(MapElement element, gint owner,
			   G_GNUC_UNUSED MapElement extra)
{
	return can_ship_be_built(element.edge, owner);
}

static gboolean check_ship_move(MapElement element, gint owner,
				G_GNUC_UNUSED MapElement extra)
{
	return can_ship_be_moved(element.edge, owner);
}

static gboolean check_bridge(MapElement element, gint owner,
			     G_GNUC_UNUSED MapElement extra)
{
	return can_bridge_be_built(element.edge, owner);
}

static gboolean check_settlement(MapElement element, gint owner,
				 G_GNUC_UNUSED MapElement extra)
{
	return can_settlement_be_built(element.node, owner);
}

static gboolean check_city(MapElement element, gint owner,
			   G_GNUC_UNUSED MapElement extra)
{
	return can_settlement_be_upgraded(element.node, owner);
}

static gboolean check_city_wall(MapElement element, gint owner,
				G_GNUC_UNUSED MapElement extra)
{
	return can_city_wall_be_built(element.node, owner);
}

/* turn */
static void frontend_state_turn(GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_ROLL, !have_rolled_dice());
		frontend_gui_check(GUI_UNDO, can_undo());
		frontend_gui_check(GUI_ROAD, turn_can_build_road());
		frontend_gui_check(GUI_SHIP, turn_can_build_ship());
		frontend_gui_check(GUI_MOVE_SHIP, turn_can_move_ship());
		frontend_gui_check(GUI_BRIDGE, turn_can_build_bridge());
		frontend_gui_check(GUI_SETTLEMENT,
				   turn_can_build_settlement());
		frontend_gui_check(GUI_CITY, turn_can_build_city());
		frontend_gui_check(GUI_CITY_WALL,
				   turn_can_build_city_wall());
		frontend_gui_check(GUI_TRADE, turn_can_trade());
		frontend_gui_check(GUI_PLAY_DEVELOP,
				   can_play_develop(develop_current_idx
						    ()));
		frontend_gui_check(GUI_BUY_DEVELOP, can_buy_develop());
		frontend_gui_check(GUI_FINISH, have_rolled_dice());

		guimap_single_click_set_functions(check_road,
						  build_road_cb,
						  check_ship,
						  build_ship_cb,
						  check_bridge,
						  build_bridge_cb,
						  check_settlement,
						  build_settlement_cb,
						  check_city,
						  build_city_cb,
						  check_city_wall,
						  build_city_wall_cb,
						  check_ship_move,
						  move_ship_cb,
						  cancel_move_ship_cb);
		break;
	case GUI_ROLL:
		cb_roll();
		break;
	case GUI_UNDO:
		cb_undo();
		return;
	case GUI_ROAD:
		gui_cursor_set(ROAD_CURSOR, check_road, build_road_cb,
			       NULL, NULL);
		return;
	case GUI_SHIP:
		gui_cursor_set(SHIP_CURSOR, check_ship, build_ship_cb,
			       NULL, NULL);
		return;
	case GUI_MOVE_SHIP:
		gui_cursor_set(SHIP_CURSOR, check_ship_move, move_ship_cb,
			       NULL, NULL);
		return;
	case GUI_BRIDGE:
		gui_cursor_set(BRIDGE_CURSOR, check_bridge,
			       build_bridge_cb, NULL, NULL);
		return;
	case GUI_SETTLEMENT:
		gui_cursor_set(SETTLEMENT_CURSOR, check_settlement,
			       build_settlement_cb, NULL, NULL);
		return;
	case GUI_CITY:
		gui_cursor_set(CITY_CURSOR, check_city, build_city_cb,
			       NULL, NULL);
		return;
	case GUI_CITY_WALL:
		gui_cursor_set(CITY_WALL_CURSOR, check_city_wall,
			       build_city_wall_cb, NULL, NULL);
		return;
	case GUI_TRADE:
		trade_begin();
		set_gui_state(frontend_state_trade);
		return;
	case GUI_PLAY_DEVELOP:
		cb_play_develop(develop_current_idx());
		return;
	case GUI_BUY_DEVELOP:
		cb_buy_develop();
		return;
	case GUI_FINISH:
		cb_end_turn();
		gui_cursor_none();	/* Finish single click build */
		set_gui_state(frontend_state_idle);
		return;
	default:
		break;
	}
}

void frontend_turn(void)
{
	/* if it already is our turn, just update the gui (maybe something
	 * happened), but don't beep */
	if (get_gui_state() == frontend_state_turn
	    || get_gui_state() == frontend_state_trade
	    || get_gui_state() == frontend_state_robber) {
		/* this is in the if, because it gets called from set_gui_state
		 * anyway. */
		frontend_gui_update();
		return;
	}
	set_gui_state(frontend_state_turn);
	play_sound(SOUND_TURN);
	/* Notification */
	notification_send(_("It is your turn."), PIONEERS_PIXMAP_DICE);
}

/* development card actions */
/* road building */
static void frontend_state_roadbuilding(GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_UNDO, can_undo());
		frontend_gui_check(GUI_ROAD,
				   road_building_can_build_road());
		frontend_gui_check(GUI_SHIP,
				   road_building_can_build_ship());
		frontend_gui_check(GUI_BRIDGE,
				   road_building_can_build_bridge());
		frontend_gui_check(GUI_FINISH, road_building_can_finish());
		guimap_single_click_set_functions(check_road,
						  build_road_cb,
						  check_ship,
						  build_ship_cb,
						  check_bridge,
						  build_bridge_cb,
						  NULL, NULL, NULL,
						  NULL, NULL, NULL,
						  NULL, NULL, NULL);
		break;
	case GUI_UNDO:
		cb_undo();
		return;
	case GUI_ROAD:
		gui_cursor_set(ROAD_CURSOR, check_road, build_road_cb,
			       NULL, NULL);
		return;
	case GUI_SHIP:
		gui_cursor_set(SHIP_CURSOR, check_ship, build_ship_cb,
			       NULL, NULL);
		return;
	case GUI_BRIDGE:
		gui_cursor_set(BRIDGE_CURSOR, check_bridge,
			       build_bridge_cb, NULL, NULL);
		return;
	case GUI_FINISH:
		cb_end_turn();
		gui_cursor_none();	/* Finish single click build */
		/** @todo RC 2018-03-14 The line below allows trade after the
		 * road building development card is played. There is still a
		 * loophole: build a road, play the development card. Now trade
		 * is enabled, but should not have been.
		 * Even the server allows trade in this scenario. */
		build_new_turn();	/* Re-enable trade */
		set_gui_state(frontend_state_turn);
		gui_prompt_hide();
		return;
	default:
		break;
	}
}

void frontend_roadbuilding(gint num_roads)
{
	gui_prompt_show(road_building_message(num_roads));
	if (get_gui_state() == frontend_state_roadbuilding)
		return;
	set_gui_state(frontend_state_roadbuilding);
}

/* monopoly */
static void frontend_state_monopoly(GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_MONOPOLY, TRUE);
		break;
	case GUI_MONOPOLY:
		cb_choose_monopoly(monopoly_type());
		monopoly_destroy_dlg();
		set_gui_state(frontend_state_turn);
		return;
	default:
		break;
	}
}

void frontend_monopoly(void)
{
	monopoly_create_dlg();
	set_gui_state(frontend_state_monopoly);
}

/* year of plenty */
static void frontend_state_plenty(GuiEvent event)
{
	gint plenty[NO_RESOURCE];
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_PLENTY, plenty_can_activate());
		break;
	case GUI_PLENTY:
		plenty_resources(plenty);
		cb_choose_plenty(plenty);
		plenty_destroy_dlg();
		set_gui_state(frontend_state_turn);
		return;
	default:
		break;
	}
}

void frontend_plenty(const gint * bank)
{
	plenty_create_dlg(bank);
	set_gui_state(frontend_state_plenty);
}

/* general actions */
/* discard */
static void frontend_state_discard(GuiEvent event)
{
	gint discards[NO_RESOURCE];

	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_DISCARD, can_discard());
		break;
	case GUI_DISCARD:
		discard_get_list(discards);
		cb_discard(discards);
		return;
	default:
		break;
	}
}

void frontend_discard(void)
{
	/* set state to idle until we must discard (or discard ends) */
	if (!discard_busy) {
		discard_busy = TRUE;
		discard_begin();
		g_assert(previous_state == dummy_state);
		previous_state = get_gui_state();
		set_gui_state(frontend_state_idle);
		gui_cursor_none();	/* Clear possible cursor */
	}
}

void frontend_discard_add(gint player_num, gint discard_num)
{
	if (player_num == my_player_num())
		g_assert(callback_mode == MODE_DISCARD);
	discard_player_must(player_num, discard_num);
	if (player_num == my_player_num())
		set_gui_state(frontend_state_discard);
	frontend_gui_update();
}

void frontend_discard_remove(gint player_num)
{
	if (discard_busy) {
		discard_player_did(player_num);
		if (player_num == my_player_num())
			set_gui_state(frontend_state_idle);
	}
	frontend_gui_update();
}

void frontend_discard_done(void)
{
	discard_busy = FALSE;
	discard_end();
	if (previous_state != dummy_state) {
		set_gui_state(previous_state);
		previous_state = dummy_state;
	}
}

/* gold */
static void frontend_state_gold(GuiEvent event)
{
	gint gold[NO_RESOURCE];
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_CHOOSE_GOLD, can_choose_gold());
		break;
	case GUI_CHOOSE_GOLD:
		choose_gold_get_list(gold);
		cb_choose_gold(gold);
		return;
	default:
		break;
	}
}

void frontend_gold(void)
{
	if (get_gui_state() != frontend_state_gold) {
		gold_choose_begin();
		g_assert(previous_state == dummy_state);
		previous_state = get_gui_state();
		set_gui_state(frontend_state_gold);
		gui_cursor_none();	/* Clear possible cursor */
	}
}

void frontend_gold_add(gint player_num, gint gold_num)
{
	gold_choose_player_prepare(player_num, gold_num);
	frontend_gui_update();
}

void frontend_gold_choose(gint gold_num, const gint * bank)
{
	gold_choose_player_must(gold_num, bank);
	frontend_gui_update();
}

void frontend_gold_remove(gint player_num, gint * resources)
{
	gold_choose_player_did(player_num, resources);
	frontend_gui_update();
}

void frontend_gold_done(void)
{
	gold_choose_end();
	if (previous_state != dummy_state) {
		set_gui_state(previous_state);
		previous_state = dummy_state;
	}
}

void frontend_game_over(gint player, gint points)
{
	gui_cursor_none();	/* Clear possible (robber) cursor */
	if (robber_busy) {
		robber_busy = FALSE;
		gui_prompt_hide();
	}
	game_over_dlg = gameover_create_dlg(player, points);
	g_signal_connect(G_OBJECT(game_over_dlg), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &game_over_dlg);
	notification_close();
	set_gui_state(frontend_state_idle);
}

void frontend_rolled_dice(gint die1, gint die2, gint player_num)
{
	histogram_dice_rolled(die1 + die2, player_num);
	identity_set_dice(die1, die2);
	gui_highlight_chits(die1 + die2);
	notification_close();
	frontend_gui_update();
}

static void frontend_state_robber(GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_UNDO, callback_mode == MODE_ROB);
		break;
	case GUI_UNDO:
		cb_undo();
		/* restart robber placement.  */
		frontend_robber();
		break;
	default:
		break;
	}
}

static void rob_building(MapElement node, G_GNUC_UNUSED MapElement extra)
{
	cb_rob(node.node->owner);
}

static void rob_edge(MapElement edge, G_GNUC_UNUSED MapElement extra)
{
	cb_rob(edge.edge->owner);
}

/* Return TRUE if the node can be robbed. */
static gboolean can_building_be_robbed(MapElement node,
				       G_GNUC_UNUSED int owner,
				       MapElement robber)
{
	guint idx;

	/* Can only steal from buildings that are not owned by me */
	if (node.node->type == BUILD_NONE
	    || node.node->owner == my_player_num())
		return FALSE;

	/* Can only steal if the owner has some resources */
	if (player_get(node.node->owner)->statistics[STAT_RESOURCES] == 0)
		return FALSE;

	/* Can only steal from buildings adjacent to hex with robber */
	for (idx = 0; idx < G_N_ELEMENTS(node.node->hexes); idx++)
		if (node.node->hexes[idx] == robber.hex)
			return TRUE;
	return FALSE;
}

/** Returns TRUE if the edge can be robbed. */
static gboolean can_edge_be_robbed(MapElement edge,
				   G_GNUC_UNUSED int owner,
				   MapElement pirate)
{
	guint idx;

	/* Can only steal from ships that are not owned by me */
	if (edge.edge->type != BUILD_SHIP
	    || edge.edge->owner == my_player_num())
		return FALSE;

	/* Can only steal if the owner has some resources */
	if (player_get(edge.edge->owner)->statistics[STAT_RESOURCES] == 0)
		return FALSE;

	/* Can only steal from edges adjacent to hex with pirate */
	for (idx = 0; idx < G_N_ELEMENTS(edge.edge->hexes); idx++)
		if (edge.edge->hexes[idx] == pirate.hex)
			return TRUE;
	return FALSE;
}

/* User just placed the robber
 */
static void place_robber_or_pirate_cb(MapElement hex,
				      G_GNUC_UNUSED MapElement extra)
{
	cb_place_robber(hex.hex);
}

void frontend_steal_ship(void)
{
	MapElement hex;
	hex.hex = map_pirate_hex(callbacks.get_map());

	gui_cursor_set(STEAL_SHIP_CURSOR, can_edge_be_robbed, rob_edge,
		       NULL, &hex);
	gui_prompt_show(_("Select the ship to steal from."));
}

void frontend_steal_building(void)
{
	MapElement hex;
	hex.hex = map_robber_hex(callbacks.get_map());

	gui_cursor_set(STEAL_BUILDING_CURSOR, can_building_be_robbed,
		       rob_building, NULL, &hex);
	gui_prompt_show(_("Select the building to steal from."));
}

void frontend_robber_done(void)
{
	robber_busy = FALSE;
	if (previous_state != dummy_state) {
		set_gui_state(previous_state);
		previous_state = dummy_state;
	}
	gui_prompt_hide();
}

static gboolean check_move_robber_or_pirate(MapElement element,
					    G_GNUC_UNUSED int owner,
					    G_GNUC_UNUSED MapElement extra)
{
	return can_robber_or_pirate_be_moved(element.hex);
}

void frontend_robber(void)
{
	if (!robber_busy) {
		/* Do this only once.  */
		robber_busy = TRUE;
		g_assert(previous_state == dummy_state);
		previous_state = get_gui_state();
	}
	/* These things are redone at undo.  */
	set_gui_state(frontend_state_robber);
	gui_cursor_set(ROBBER_CURSOR, check_move_robber_or_pirate,
		       place_robber_or_pirate_cb, NULL, NULL);
	gui_prompt_show(_("Place the robber."));
	frontend_gui_update();
}

static gboolean check_road_setup(MapElement element,
				 G_GNUC_UNUSED gint owner,
				 G_GNUC_UNUSED MapElement extra)
{
	return setup_check_road(element.edge);
}

static gboolean check_ship_setup(MapElement element,
				 G_GNUC_UNUSED gint owner,
				 G_GNUC_UNUSED MapElement extra)
{
	return setup_check_ship(element.edge);
}

static gboolean check_bridge_setup(MapElement element,
				   G_GNUC_UNUSED gint owner,
				   G_GNUC_UNUSED MapElement extra)
{
	return setup_check_bridge(element.edge);
}

static gboolean check_settlement_setup(MapElement element,
				       G_GNUC_UNUSED gint owner,
				       G_GNUC_UNUSED MapElement extra)
{
	return setup_check_settlement(element.node);
}

static void frontend_mode_setup(GuiEvent event)
{
	switch (event) {
	case GUI_UPDATE:
		frontend_gui_check(GUI_UNDO, can_undo());
		frontend_gui_check(GUI_ROAD, setup_can_build_road());
		frontend_gui_check(GUI_BRIDGE, setup_can_build_bridge());
		frontend_gui_check(GUI_SHIP, setup_can_build_ship());
		frontend_gui_check(GUI_SETTLEMENT,
				   setup_can_build_settlement());
		frontend_gui_check(GUI_FINISH, setup_can_finish());
		guimap_single_click_set_functions(check_road_setup,
						  build_road_cb,
						  check_ship_setup,
						  build_ship_cb,
						  check_bridge_setup,
						  build_bridge_cb,
						  check_settlement_setup,
						  build_settlement_cb,
						  NULL, NULL, NULL,
						  NULL, NULL, NULL, NULL);
		break;
	case GUI_UNDO:
		/* The user has pressed the "Undo" button.  Send a
		 * command to the server to attempt the undo.  The
		 * server will respond telling us whether the undo was
		 * successful or not.
		 */
		cb_undo();
		return;
	case GUI_ROAD:
		gui_cursor_set(ROAD_CURSOR,
			       check_road_setup, build_road_cb, NULL,
			       NULL);
		return;
	case GUI_SHIP:
		gui_cursor_set(SHIP_CURSOR,
			       check_ship_setup, build_ship_cb, NULL,
			       NULL);
		return;
	case GUI_BRIDGE:
		gui_cursor_set(BRIDGE_CURSOR,
			       check_bridge_setup, build_bridge_cb, NULL,
			       NULL);
		return;
	case GUI_SETTLEMENT:
		gui_cursor_set(SETTLEMENT_CURSOR,
			       check_settlement_setup,
			       build_settlement_cb, NULL, NULL);
		return;
	case GUI_FINISH:
		cb_end_turn();
		gui_cursor_none();	/* Finish single click build */
		set_gui_state(frontend_state_idle);
		notification_close();
		return;
	default:
		break;
	}
}

void frontend_setup(G_GNUC_UNUSED gint num_settlements,
		    G_GNUC_UNUSED gint num_roads)
{
	if (get_gui_state() == frontend_mode_setup) {
		frontend_gui_update();
		return;
	}
	set_gui_state(frontend_mode_setup);
	play_sound(SOUND_TURN);
	/* Notification */
	notification_send(_("It is your turn to setup."),
			  PIONEERS_PIXMAP_SETTLEMENT);
}
