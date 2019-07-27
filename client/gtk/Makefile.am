# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

bin_PROGRAMS += pioneers
desktop_in_files += client/gtk/pioneers.desktop.in

# if anyone knows a cleaner way to do this, be my guest.  Automake screamed
#   at me when I tried to do it more directly.
if ADMIN_GTK_SUPPORT
ADMIN_GTK = -DADMIN_GTK
ADMIN_GTK_FILES_ACTIVE = client/gtk/admin-gtk.c
ADMIN_GTK_FILES_INACTIVE =
else
ADMIN_GTK =
ADMIN_GTK_FILES_ACTIVE =
ADMIN_GTK_FILES_INACTIVE = client/gtk/admin-gtk.c
endif

pioneers_CPPFLAGS = -I$(top_srcdir)/client -I$(top_srcdir)/client/common $(LIBNOTIFY_CFLAGS) $(gtk_cflags) $(ADMIN_GTK) $(avahi_cflags)

EXTRA_pioneers_SOURCES = $(ADMIN_GTK_FILES_INACTIVE)

pioneers_SOURCES = \
	$(ADMIN_GTK_FILES_ACTIVE) \
	client/callback.h \
	client/gtk/audio.h \
	client/gtk/avahi.h \
	client/gtk/avahi-browser.h \
	client/gtk/frontend.h \
	client/gtk/gui.h \
	client/gtk/histogram.h \
	client/gtk/audio.c \
	client/gtk/avahi.c \
	client/gtk/avahi-browser.c \
	client/gtk/callbacks.c \
	client/gtk/chat.c \
	client/gtk/connect.c \
	client/gtk/develop.c \
	client/gtk/discard.c \
	client/gtk/frontend.c \
	client/gtk/gameover.c \
	client/gtk/gold.c \
	client/gtk/gui.c \
	client/gtk/histogram.c \
	client/gtk/identity.c \
	client/gtk/interface.c \
	client/gtk/legend.c \
	client/gtk/monopoly.c \
	client/gtk/name.c \
	client/gtk/notification.c \
	client/gtk/notification.h \
	client/gtk/offline.c \
	client/gtk/plenty.c \
	client/gtk/player.c \
	client/gtk/quote.c \
	client/gtk/quote-view.c \
	client/gtk/quote-view.h \
	client/gtk/resource.c \
	client/gtk/resource-view.gob \
	client/gtk/resource-view.gob.stamp \
	client/gtk/resource-view.c \
	client/gtk/resource-view.h \
	client/gtk/resource-view-private.h \
	client/gtk/resource-table.c \
	client/gtk/resource-table.h \
	client/gtk/settingscreen.c \
	client/gtk/state.c \
	client/gtk/trade.c

pioneers_LDADD = libpioneersclient.a $(gtk_libs) $(avahi_libs) $(LIBNOTIFY_LIBS)

# Include the data here, not at the top,
# it can add extra resources to the executable
include client/gtk/data/Makefile.am

BUILT_SOURCES += \
	client/gtk/resource-view.gob.stamp \
	client/gtk/resource-view.c \
	client/gtk/resource-view.h \
	client/gtk/resource-view-private.h

MAINTAINERCLEANFILES += \
	client/gtk/resource-view.gob.stamp \
	client/gtk/resource-view.c \
	client/gtk/resource-view.h \
	client/gtk/resource-view-private.h
