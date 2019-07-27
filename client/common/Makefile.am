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

noinst_LIBRARIES += libpioneersclient.a

libpioneersclient_a_CPPFLAGS = -I$(top_srcdir)/client $(console_cflags)

libpioneersclient_a_SOURCES = \
	client/callback.h \
	client/common/build.c \
	client/common/callback.c \
	client/common/client.c \
	client/common/client.h \
	client/common/develop.c \
	client/common/main.c \
	client/common/player.c \
	client/common/resource.c \
	client/common/robber.c \
	client/common/setup.c \
	client/common/stock.c \
	client/common/turn.c
