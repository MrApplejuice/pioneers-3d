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

icon_DATA += server/gtk/pioneers-server.png
desktop_in_files += server/gtk/pioneers-server-gtk.desktop.in
bin_PROGRAMS += pioneers-server-gtk

icons += server/gtk/pioneers-server.svg

pioneers_server_gtk_CPPFLAGS = $(gtk_cflags) $(avahi_cflags) -I $(srcdir)/server
pioneers_server_gtk_LDADD = libpioneers_server.a $(gtk_libs) $(avahi_libs)

pioneers_server_gtk_SOURCES = \
	server/gtk/main.c \
	server/gtk/pioneers-server.ico \
	server/gtk/pioneers-server.rc

if USE_WINDOWS_ICON
pioneers_server_gtk_LDADD += server/gtk/pioneers-server.res
CLEANFILES += server/gtk/pioneers-server.res
endif
