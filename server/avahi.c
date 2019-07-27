/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.

 * Copyright (C) 2004-2010 Avahi http://avahi.org
 * Copyright (C) 2010 Andreas Steinel <lnxbil@users.sourceforge.net>
 * Copyright (C) 2010 Roland Clobus <rclobus@rclobus.nl>
 *
 * This file is originally based on client-publish-service.c last committed on
 * 2006-01-27 20:34:22Z by lennart.
 * It got adapted to glib instead from pure avahi
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

/* Translation note: the Avahi error messages are not translated,
 *                   because the strings returned by Avahi aren't either
 */

#include "config.h"
#include "avahi.h"
#include "network.h"
#include "log.h"

#include <stdlib.h>

#ifdef HAVE_AVAHI
#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

#include <avahi-glib/glib-watch.h>

static AvahiEntryGroup *group = NULL;
static AvahiGLibPoll *glib_poll = NULL;
static AvahiClient *client = NULL;
static char *name = NULL;

static void create_services(AvahiClient * c, Game * game);

static void entry_group_callback(AvahiEntryGroup * g,
				 AvahiEntryGroupState state,
				 void *userdata)
{
	Game *game = (Game *) userdata;
	/* Called whenever the entry group state changes */

	switch (state) {
	case AVAHI_ENTRY_GROUP_ESTABLISHED:
		/* The entry group has been established successfully */
		log_message(MSG_INFO,
			    _("Avahi registration successful.\n"));
		break;
	case AVAHI_ENTRY_GROUP_COLLISION:{
			/* A service name collision happened. Let's pick a new name */
			gchar *n = avahi_alternative_service_name(name);
			avahi_free(name);
			name = n;

			log_message(MSG_INFO,
				    _
				    ("Avahi service name collision, renaming service to '%s'.\n"),
				    name);

			/* And recreate the services */
			create_services(avahi_entry_group_get_client(g),
					game);
			break;
		}

	case AVAHI_ENTRY_GROUP_FAILURE:
		/* Some kind of failure happened while we were registering */
		log_message(MSG_ERROR, _("Avahi error: %s\n"),
			    "Some kind of failure happened while we were registering");
		break;
	case AVAHI_ENTRY_GROUP_UNCOMMITED:
	case AVAHI_ENTRY_GROUP_REGISTERING:
		break;
	}
}

static void create_services(AvahiClient * c, Game * game)
{
	const gchar *hostname;
	gchar *servicename;
	AvahiStringList *sl;
	int ret;

	g_assert(c != NULL);

	/* If this is the first time we're called, let's create a new entry group */
	if (!group) {
		if (!
		    (group =
		     avahi_entry_group_new(c, entry_group_callback,
					   NULL))) {
			log_message(MSG_ERROR, _("Avahi error: %s, %s\n"),
				    "avahi_entry_group_new() failed",
				    avahi_strerror(avahi_client_errno(c)));
			avahi_glib_poll_free(glib_poll);
			return;
		}
	}

	sl = avahi_string_list_new(NULL, NULL);
	sl = avahi_string_list_add_printf(sl, "version=%s",
					  client_version_type_to_string
					  (LATEST_VERSION));
	sl = avahi_string_list_add_printf(sl, "title=%s",
					  game->params->title);

	/* Add the service for IPP */
	hostname = game->hostname ? game->hostname : g_get_host_name();
	servicename =
	    g_strdup_printf("%s [%s]", hostname, game->server_port);
	ret = avahi_entry_group_add_service_strlst(group,
						   AVAHI_IF_UNSPEC,
						   AVAHI_NETWORK_PROTOCOL,
						   0,
						   servicename,
						   AVAHI_ANNOUNCE_NAME,
						   NULL, NULL, (uint16_t)
						   atoi(game->server_port),
						   sl);
	g_free(servicename);
	if (ret < 0) {
		gchar *msg = g_strdup_printf("Failed to add '%s' service",
					     AVAHI_ANNOUNCE_NAME);
		log_message(MSG_ERROR, _("Avahi error: %s, %s\n"), msg,
			    avahi_strerror(ret));
		g_free(msg);
		avahi_string_list_free(sl);
		avahi_glib_poll_free(glib_poll);
		return;
	}

	/* Tell the server to register the service */
	if ((ret = avahi_entry_group_commit(group)) < 0) {
		log_message(MSG_ERROR, _("Avahi error: %s, %s\n"),
			    "Failed to commit entry_group",
			    avahi_strerror(ret));
		avahi_string_list_free(sl);
		avahi_glib_poll_free(glib_poll);
		return;
	}

	avahi_string_list_free(sl);
	return;
}


static void client_callback(AvahiClient * c,
			    AvahiClientState state, AVAHI_GCC_UNUSED void
			    *userdata)
{
	Game *game = (Game *) userdata;
	g_assert(c != NULL);
	/* Called whenever the client or server state changes */
	switch (state) {
	case AVAHI_CLIENT_S_RUNNING:
		/* The server has startup successfully and registered its host
		 * name on the network, so it's time to create our services */
		if (!group)
			create_services(c, game);
		break;
	case AVAHI_CLIENT_S_COLLISION:
		/* Let's drop our registered services. When the server is back
		 * in AVAHI_SERVER_RUNNING state we will register them
		 * again with the new host name. */
		if (group)
			avahi_entry_group_reset(group);
		break;
	case AVAHI_CLIENT_FAILURE:
		log_message(MSG_ERROR,
			    _("Avahi error: %s, %s\n"), "Client failure",
			    avahi_strerror(avahi_client_errno(c)));
		avahi_glib_poll_free(glib_poll);
		break;
	case AVAHI_CLIENT_CONNECTING:
	case AVAHI_CLIENT_S_REGISTERING:
		;
	}
}
#endif				/* HAVE_AVAHI */

void avahi_register_game(Game * game)
{
#ifdef HAVE_AVAHI
	const AvahiPoll *poll_api;
	int error;
	glib_poll = avahi_glib_poll_new(NULL, G_PRIORITY_DEFAULT);
	poll_api = avahi_glib_poll_get(glib_poll);
	/* Allocate main loop object */
	if (!poll_api) {
		log_message(MSG_ERROR,
			    _("Avahi error: %s, %s\n"),
			    _("Unable to register Avahi server"),
			    "Failed to create glib poll object");
		avahi_unregister_game();
		return;
	}

	name = avahi_strdup(game->params->title);
	/* Allocate a new client */
	client =
	    avahi_client_new(poll_api, 0, client_callback, game, &error);
	/* Check whether creating the client object succeeded */
	if (!client) {
		log_message(MSG_ERROR,
			    _("Avahi error: %s, %s\n"),
			    _("Unable to register Avahi server"),
			    avahi_strerror(error));
		avahi_unregister_game();
	}
#endif				/* HAVE_AVAHI */
}

void avahi_unregister_game(void)
{
#ifdef HAVE_AVAHI

	/* Cleanup things */

	if (group) {
		avahi_entry_group_free(group);
		group = NULL;
	}

	if (client) {
		avahi_client_free(client);
		client = NULL;
	}

	if (glib_poll) {
		avahi_glib_poll_free(glib_poll);
		glib_poll = NULL;
	}

	if (name) {
		avahi_free(name);
		name = NULL;
	}

	log_message(MSG_INFO, _("Unregistering Avahi.\n"));
#endif				/* HAVE_AVAHI */
}
