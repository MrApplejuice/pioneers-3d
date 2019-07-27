# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 2011 Roland Clobus <rclobus@rclobus.nl>
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

if IS_MINGW_PORT

install-exec-hook: install-MinGW

install-MinGW:
	cp /mingw/bin/libatk-1.0-0.dll /usr/local
	cp /mingw/bin/libcairo-2.dll /usr/local
	cp /mingw/bin/libcairo-gobject-2.dll /usr/local
	cp /mingw/bin/libcroco-0.6-3.dll /usr/local
	cp /mingw/bin/libffi-6.dll /usr/local
	cp /mingw/bin/libfontconfig-1.dll /usr/local
	cp /mingw/bin/libfreetype-6.dll /usr/local
	cp /mingw/bin/libgcc_s_dw2-1.dll /usr/local
	cp /mingw/bin/libgdk-3-0.dll /usr/local
	cp /mingw/bin/libgdk_pixbuf-2.0-0.dll /usr/local
	cp /mingw/bin/libgio-2.0-0.dll /usr/local
	cp /mingw/bin/libglib-2.0-0.dll /usr/local
	cp /mingw/bin/libgmodule-2.0-0.dll /usr/local
	cp /mingw/bin/libgobject-2.0-0.dll /usr/local
	cp /mingw/bin/libgtk-3-0.dll /usr/local
	cp /mingw/bin/libiconv-2.dll /usr/local
	cp /mingw/bin/libintl-8.dll /usr/local
	cp /mingw/bin/liblzma-5.dll /usr/local
	cp /mingw/bin/libpango-1.0-0.dll /usr/local
	cp /mingw/bin/libpangocairo-1.0-0.dll /usr/local
	cp /mingw/bin/libpangoft2-1.0-0.dll /usr/local
	cp /mingw/bin/libpangowin32-1.0-0.dll /usr/local
	cp /mingw/lib/gdk-pixbuf-2.0/2.10.0/loaders/libpixbufloader-svg.dll /usr/local
	cp /mingw/bin/libpixman-1-0.dll /usr/local
	cp /mingw/bin/libpng15-15.dll /usr/local
	cp /mingw/bin/librsvg-2-2.dll /usr/local
	cp /mingw/bin/libssp-0.dll /usr/local
	cp /mingw/bin/libxml2-2.dll /usr/local
	cp /mingw/bin/pthreadGC2.dll /usr/local
	cp /mingw/bin/zlib1.dll /usr/local
	cp /mingw/bin/gspawn-win32-helper.exe /usr/local
	cp /mingw/bin/gspawn-win32-helper-console.exe /usr/local
	mkdir -p /usr/local/lib/gdk-pixbuf-2.0/2.10.0
	cp $(srcdir)/MinGW/loaders.cache /usr/local/lib/gdk-pixbuf-2.0/2.10.0
	mkdir -p /usr/local/share/icons/hicolor
	cp /mingw/share/icons/hicolor/index.theme /usr/local/share/icons/hicolor
	mkdir -p /usr/local/share/icons/hicolor/24x24/actions
	cp /mingw/share/gtk-doc/html/gtk3/edit-undo-ltr.png /usr/local/share/icons/hicolor/24x24/actions/edit-undo.png
	mkdir -p /usr/local/share/glib-2.0/schemas
	cp /mingw/share/glib-2.0/schemas/gschemas.compiled /usr/local/share/glib-2.0/schemas

endif # IS_MINGW_PORT

EXTRA_DIST += \
	$(srcdir)/MinGW/loaders.cache \
	$(srcdir)/MinGW/README.txt
