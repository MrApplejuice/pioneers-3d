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

include client/gtk/data/themes/Makefile.am

icon_DATA += client/gtk/data/pioneers.png

icons += client/gtk/data/pioneers.svg

pixmap_DATA += \
	client/gtk/data/bridge.svg \
	client/gtk/data/city.svg \
	client/gtk/data/city_wall.svg \
	client/gtk/data/develop.svg \
	client/gtk/data/dice.svg \
	client/gtk/data/finish.svg \
	client/gtk/data/road.svg \
	client/gtk/data/settlement.svg \
	client/gtk/data/ship.svg \
	client/gtk/data/ship_move.svg \
	client/gtk/data/splash.png \
	client/gtk/data/trade.svg \
	client/gtk/data/brick.png \
	client/gtk/data/grain.png \
	client/gtk/data/lumber.png \
	client/gtk/data/ore.png \
	client/gtk/data/wool.png \
	client/gtk/data/style-human.png \
	client/gtk/data/style-human-1.png \
	client/gtk/data/style-human-2.png \
	client/gtk/data/style-human-3.png \
	client/gtk/data/style-human-4.png \
	client/gtk/data/style-human-5.png \
	client/gtk/data/style-human-6.png \
	client/gtk/data/style-human-7.png \
	client/gtk/data/style-ai.svg \
	client/gtk/data/pioneers.ico \
	client/gtk/data/pioneers.rc

EXTRA_DIST += client/gtk/data/splash.svg
MAINTAINERCLEANFILES += client/gtk/data/splash.png

client/gtk/data/splash.png: client/gtk/data/splash.svg
	$(MKDIR_P) $(dir $@)
	$(svg_renderer_path) $(svg_renderer_width)400$(svg_renderer_height)400 $< $(svg_renderer_output) $@

if USE_WINDOWS_ICON
pioneers_LDADD += client/gtk/data/pioneers.res
CLEANFILES += client/gtk/data/pioneers.res
endif
