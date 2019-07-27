# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
# Copyright (C) 2006 Roland Clobus <rclobus@bigfoot.com>
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

icon_DATA += editor/gtk/pioneers-editor.png
desktop_in_files += editor/gtk/pioneers-editor.desktop.in
bin_PROGRAMS += pioneers-editor

icons += editor/gtk/pioneers-editor.svg

pioneers_editor_CPPFLAGS = $(gtk_cflags)

pioneers_editor_SOURCES = \
	editor/gtk/editor.c \
	editor/gtk/game-devcards.c \
	editor/gtk/game-devcards.h \
	editor/gtk/game-buildings.c \
	editor/gtk/game-buildings.h \
	editor/gtk/game-resources.c \
	editor/gtk/game-resources.h \
	editor/gtk/pioneers-editor.ico \
	editor/gtk/pioneers-editor.rc

pioneers_editor_LDADD = $(gtk_libs)

if USE_WINDOWS_ICON
pioneers_editor_LDADD += editor/gtk/pioneers-editor.res
CLEANFILES += editor/gtk/pioneers-editor.res
endif
