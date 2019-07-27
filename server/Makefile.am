# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2006 Bas Wijnen <shevek@fmf.nl>
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

if BUILD_SERVER

if HAVE_GTK
include server/gtk/Makefile.am
endif

bin_PROGRAMS += pioneers-server-console
noinst_LIBRARIES += libpioneers_server.a

pioneers_server_console_CPPFLAGS = $(console_cflags)
libpioneers_server_a_CPPFLAGS = $(console_cflags) $(avahi_cflags)

libpioneers_server_a_SOURCES = \
	server/admin.c \
	server/admin.h \
	server/avahi.c \
	server/avahi.h \
	server/buildutil.c \
	server/develop.c \
	server/discard.c \
	server/gold.c \
	server/meta.c \
	server/player.c \
	server/pregame.c \
	server/resource.c \
	server/robber.c \
	server/server.c \
	server/server.h \
	server/trade.c \
	server/turn.c

pioneers_server_console_SOURCES = \
	server/main.c \
	server/glib-driver.c \
	server/glib-driver.h

pioneers_server_console_LDADD = libpioneers_server.a $(console_libs) $(avahi_libs)

endif # BUILD_SERVER

config_DATA += \
	server/default.game \
	server/5-6-player.game \
	server/four-islands.game \
	server/seafarers.game \
	server/seafarers-gold.game \
	server/small.game \
	server/archipel_gold.game \
	server/canyon.game \
	server/coeur.game \
	server/conquest.game \
	server/conquest+ports.game \
	server/crane_island.game \
	server/iles.game \
	server/pond.game \
	server/square.game \
	server/star.game \
	server/x.game \
	server/Cube.game \
	server/Another_swimming_pool_in_the_wall.game \
	server/Evil_square.game \
	server/GuerreDe100ans.game \
	server/Mini_another_swimming_pool_in_the_wall.game \
	server/henjes.game \
	server/lorindol.game \
	server/lobby.game \
	server/south_africa.game \
	server/ubuntuland.game \
	server/north_america.game
