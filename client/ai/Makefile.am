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

bin_PROGRAMS += pioneersai

pioneersai_CPPFLAGS = -I$(top_srcdir)/client -I$(top_srcdir)/client/common $(console_cflags) $(GOBJECT2_CFLAGS) -DINTEGRATE_GENETIC_ALGORITHM

pioneersai_SOURCES = \
	client/callback.h \
	client/ai/ai.h \
	client/ai/ai.c \
	client/ai/genetic.c \
	client/ai/genetic_core.h \
	client/ai/genetic_core.c \
	client/ai/greedy.c \
	client/ai/lobbybot.c

pioneersai_LDADD = libpioneersclient.a $(console_libs) $(GOBJECT2_LIBS)

config_DATA += \
	client/ai/computer_names
