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

noinst_LIBRARIES += libpioneers_gtk.a

libpioneers_gtk_a_CPPFLAGS = $(gtk_cflags)

libpioneers_gtk_a_SOURCES = \
	common/gtk/aboutbox.c \
	common/gtk/aboutbox.h \
	common/gtk/colors.c \
	common/gtk/colors.h \
	common/gtk/common_gtk.c \
	common/gtk/common_gtk.h \
	common/gtk/config-gnome.c \
	common/gtk/config-gnome.h \
	common/gtk/game-rules.c \
	common/gtk/game-rules.h \
	common/gtk/game-settings.c \
	common/gtk/game-settings.h \
	common/gtk/guimap.c \
	common/gtk/guimap.h \
	common/gtk/map-icons.c \
	common/gtk/map-icons.h \
	common/gtk/metaserver.c \
	common/gtk/metaserver.h \
	common/gtk/player-icon.c \
	common/gtk/player-icon.h \
	common/gtk/polygon.c \
	common/gtk/polygon.h \
	common/gtk/scrollable-text-view.gob \
	common/gtk/scrollable-text-view.gob.stamp \
	common/gtk/scrollable-text-view.c \
	common/gtk/scrollable-text-view.h \
	common/gtk/select-game.c \
	common/gtk/select-game.h \
	common/gtk/theme.c \
	common/gtk/theme.h

BUILT_SOURCES += \
	common/gtk/scrollable-text-view.gob.stamp \
	common/gtk/scrollable-text-view.c \
	common/gtk/scrollable-text-view.h

MAINTAINERCLEANFILES += \
	common/gtk/scrollable-text-view.gob.stamp \
	common/gtk/scrollable-text-view.c \
	common/gtk/scrollable-text-view.h

actions_DATA += \
	common/gtk/pioneers-checkmark.svg \
	common/gtk/pioneers-cross.svg
