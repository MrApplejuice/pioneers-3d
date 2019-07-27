/* Pioneers - Implementation of the excellent Settlers of Catan board game.
 *   Go buy a copy.
 *
 * Copyright (C) 2008 Roland Clobus <rclobus@bigfoot.com>
 * Copyright (C) 2009 Andreas Steinel <lnxbil@users.sourceforge.net>
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
#include <gtk/gtk.h>

#include "log.h"
#include "avahi.h"

#ifdef HAVE_AVAHI

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#include <avahi-glib/glib-watch.h>

static AvahiGLibPoll *glib_poll = NULL;
static AvahiClient *client = NULL;
static AvahiServiceBrowser *sb = NULL;

static AvahiBrowser *zcsptr = NULL;

static void resolve_callback(AvahiServiceResolver * r,
			     AVAHI_GCC_UNUSED AvahiIfIndex interface,
			     AVAHI_GCC_UNUSED AvahiProtocol protocol,
			     AvahiResolverEvent event,
			     const char *name,
			     const char *type,
			     const char *domain,
			     const char *host_name,
			     AVAHI_GCC_UNUSED const AvahiAddress * address,
			     uint16_t port,
			     AvahiStringList * txt,
			     AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
			     AVAHI_GCC_UNUSED void *userdata)
{
	g_assert(r);

	/* Called whenever a service has been resolved successfully or timed out */
	switch (event) {
	case AVAHI_RESOLVER_FAILURE:
		debug
		    ("Avahi: Failed to resolve service '%s' of type '%s' in domain '%s': %s",
		     name, type, domain,
		     avahi_strerror(avahi_client_errno
				    (avahi_service_resolver_get_client
				     (r))));
		break;

	case AVAHI_RESOLVER_FOUND:{
			gchar *version = NULL;
			gchar *title = NULL;

			/* Parse the text part */
			AvahiStringList *iter = txt;
			while (iter != NULL) {
				gchar *text = g_strdup((gchar *)
						       avahi_string_list_get_text
						       (iter));
				if (g_str_has_prefix(text, "version=")) {
					version = g_strdup(text + 8);
				} else
				    if (g_str_has_prefix(text, "title=")) {
					title = g_strdup(text + 6);
				}
				g_free(text);
				iter = avahi_string_list_get_next(iter);
			}

			if (zcsptr != NULL) {
				gchar *sport =
				    g_strdup_printf("%" G_GUINT16_FORMAT,
						    port);
				char resolved_hostname
				    [AVAHI_ADDRESS_STR_MAX];
				avahi_address_snprint(resolved_hostname,
						      sizeof
						      (resolved_hostname),
						      address);
				avahibrowser_add(zcsptr, name,
						 resolved_hostname,
						 host_name, sport, version,
						 title);
				g_free(sport);
			}
			g_free(version);
			g_free(title);
		}
	}

	avahi_service_resolver_free(r);
}

static void browse_callback(AvahiServiceBrowser * b,
			    AvahiIfIndex interface,
			    AvahiProtocol protocol,
			    AvahiBrowserEvent event,
			    const char *name,
			    const char *type,
			    const char *domain,
			    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
			    void *userdata)
{

	AvahiClient *c = userdata;
	g_assert(b);

	/* Called whenever a new service becomes available on the LAN or is removed from the LAN */
	switch (event) {
	case AVAHI_BROWSER_FAILURE:
		debug("Avahi browser: failure %s",
		      avahi_strerror(avahi_client_errno
				     (avahi_service_browser_get_client
				      (b))));
		return;

	case AVAHI_BROWSER_NEW:
		/* We ignore the returned resolver object. In the callback
		   function we free it. If the server is terminated before
		   the callback function is called the server will free
		   the resolver for us. */

		if (!
		    (avahi_service_resolver_new
		     (c, interface, protocol, name, type, domain,
		      AVAHI_PROTO_UNSPEC, 0, resolve_callback, c)))
			debug
			    ("Avahi browser: Failed to resolve service '%s': %s",
			     name, avahi_strerror(avahi_client_errno(c)));
		break;

	case AVAHI_BROWSER_REMOVE:
		avahibrowser_del(zcsptr, name);
		break;

	case AVAHI_BROWSER_ALL_FOR_NOW:
	case AVAHI_BROWSER_CACHE_EXHAUSTED:
		break;
	}
}

static void client_callback(AvahiClient * c, AvahiClientState state,
			    AVAHI_GCC_UNUSED void *userdata)
{
	g_assert(c);

	/* Called whenever the client or server state changes */
	if (state == AVAHI_CLIENT_FAILURE) {
		debug("Avahi server connection failure: %s",
		      avahi_strerror(avahi_client_errno(c)));
	}
}

void avahi_register(AvahiBrowser * widget)
{
	const AvahiPoll *poll_api;

	zcsptr = widget;
	int error;

	glib_poll = avahi_glib_poll_new(NULL, G_PRIORITY_DEFAULT);
	poll_api = avahi_glib_poll_get(glib_poll);

	/* Allocate main loop object */
	if (!poll_api) {
		debug("Avahi: Failed to create glib poll object.");
		avahi_unregister();
		return;
	}


	client =
	    avahi_client_new(poll_api, 0, client_callback, NULL, &error);


	/* Check wether creating the client object succeeded */
	if (!client) {
		debug("Avahi: Failed to create client: %s",
		      avahi_strerror(error));
		avahi_unregister();
		return;
	}

	/* Create the service browser */
	if (!
	    (sb =
	     avahi_service_browser_new(client, AVAHI_IF_UNSPEC,
				       AVAHI_NETWORK_PROTOCOL,
				       AVAHI_ANNOUNCE_NAME, NULL, 0,
				       browse_callback, client))) {
		debug("Failed to create service browser: %s"),
		    avahi_strerror(avahi_client_errno(client));
		avahi_unregister();
	}
}

void avahi_unregister(void)
{
	/* Cleanup things */
	if (sb) {
		avahi_service_browser_free(sb);
		sb = NULL;
	}

	if (client) {
		avahi_client_free(client);
		client = NULL;
	}

	if (glib_poll) {
		avahi_glib_poll_free(glib_poll);
		glib_poll = NULL;
	}

}
#endif
