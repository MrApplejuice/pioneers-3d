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

# some settings
console_cflags = \
	-I$(top_srcdir)/common \
	-I$(top_builddir)/common \
	-I$(includedir) \
	$(GLIB2_CFLAGS) \
	$(GIO2_CFLAGS) \
	$(WARNINGS) \
	$(DEBUGGING) \
	$(GLIB_DEPRECATION) \
	-DDATADIR=\""$(pioneers_datadir)"\" \
	-DTHEMEDIR=\""$(pioneers_themedir_embed)"\" \
	-DLOCALEDIR=\""$(pioneers_localedir)"\" \
	-DPIONEERS_DIR_DEFAULT=\""$(pioneers_datadir)/games/pioneers"\" \
	-DPIONEERS_SERVER_CONSOLE_PROGRAM_NAME=\""pioneers-server-console$(EXEEXT)"\" \
	-DPIONEERS_SERVER_GTK_PROGRAM_NAME=\""pioneers-server-gtk$(EXEEXT)"\" \
	-DPIONEERS_CLIENT_GTK_PROGRAM_NAME=\""pioneers$(EXEEXT)"\" \
	-DPIONEERS_AI_PROGRAM_NAME=\""pioneersai$(EXEEXT)"\"

avahi_cflags = \
	$(AVAHI_CLIENT_CFLAGS) \
	$(AVAHI_GLIB_CFLAGS)

gtk_cflags = \
	$(console_cflags) \
	-I$(top_srcdir)/common/gtk \
	$(GTK_CFLAGS) \
	$(GTK_DEPRECATION)

# The Fink port needs an explicit reference to driver.o
console_libs = \
	libpioneers.a \
	$(top_builddir)/common/libpioneers_a-driver.o \
	$(GLIB2_LIBS) \
	$(GIO2_LIBS)

avahi_libs = \
	$(AVAHI_CLIENT_LIBS) \
	$(AVAHI_GLIB_LIBS)

gtk_libs = \
	$(console_libs) \
	libpioneers_gtk.a \
	$(GTK_LIBS)

configdir = $(datadir)/games/pioneers
icondir = $(datadir)/pixmaps
pixmapdir = $(datadir)/pixmaps/pioneers
actionsdir = $(pioneers_iconthemedir)/scalable/actions
desktopdir = $(datadir)/applications

# Let object files be generated in their own subdirectories
AUTOMAKE_OPTIONS = subdir-objects foreign

# set up these variables so the included Makefile.ams can use +=
SUBDIRS =
bin_PROGRAMS =
noinst_PROGRAMS =
noinst_LIBRARIES =
man_MANS =
config_DATA =
icon_DATA =
pixmap_DATA =
actions_DATA =
desktop_in_files =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
EXTRA_DIST = autogen.sh pioneers.spec README.Cygwin README.MinGW Doxyfile
BUILT_SOURCES =
icons = 

# Use GOB to create new classes
%.gob.stamp %.c %.h %-private.h: %.gob
	$(MKDIR_P) $(dir $@)
	$(GOB2) --output-dir $(dir $@) $<
	touch $@

# creating icons
%.png: %.svg
	$(MKDIR_P) $(dir $@)
	$(svg_renderer_path) $(svg_renderer_width)256$(svg_renderer_height)256 $< $(svg_renderer_output) $@

if USE_WINDOWS_ICON
# Will be used in Windows builds
%.res: %.rc %.ico
	$(MKDIR_P) $(dir $@)
	windres -I$(top_srcdir) -O coff -o $@ $<
endif

if BUILD_CLIENT
include client/Makefile.am
endif

include server/Makefile.am

if BUILD_METASERVER
include metaserver/Makefile.am
endif

if BUILD_EDITOR
include editor/Makefile.am
endif

if BUILD_HELP
# yelp-tools doesn't use automake, but creates its own Makefile
SUBDIRS += client/help
endif

include MinGW/Makefile.am
include common/Makefile.am
include docs/Makefile.am

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

# Make use of some of the variables that were filled in by the included
# Makefile.ams
MAINTAINERCLEANFILES += $(icon_DATA)
DISTCLEANFILES += $(desktop_in_files:.desktop.in=.desktop) intltool-extract intltool-merge intltool-update
EXTRA_DIST += \
	$(man_MANS) \
	$(desktop_in_files) \
	$(config_DATA) \
	$(pixmap_DATA) \
	$(icon_DATA) \
	$(subst png,svg,$(icon_DATA)) \
	$(actions_DATA)

# po doesn't use automake, but creates its own Makefile
SUBDIRS += po

clean-local:
	find $(top_srcdir) -name "*~" -type f -delete
	rm -rf html

distclean-local:
	rm -f *~
	rm -rf autom4te.cache

# Reformat the code.
reindent:
	find . -name '*.[ch]' -exec indent -kr -i8 '{}' ';'
	find . -name '*.[ch]' -exec indent -kr -i8 '{}' ';'

restorepo:
	svn revert po/*.po po/pioneers.pot

# Find and fix common misspellings in the code
codespell:
	find . -name '*.[ch]' -exec codespell -d -i3 -w '{}' ';'

# Remove ALL generated files
pristine: maintainer-clean
	rm -fr m4
	svn status --no-ignore | awk '$$1=="I" { print substr($$0, 9, 255) }' | tr '\n' '\0' | xargs -0 rm

# Application icons, in various sizes
BUILT_SOURCES += $(subst .svg,.48x48_apps.png,$(icons))
EXTRA_DIST += $(icons) $(subst .svg,.48x48_apps.png,$(icons))

%.48x48_apps.png: %.svg
	$(MKDIR_P) $(dir $@)
	$(svg_renderer_path) $(svg_renderer_width)48$(svg_renderer_height)48 $< $(svg_renderer_output) $@

install-icons:
	$(MKDIR_P) $(DESTDIR)$(pioneers_iconthemedir)/48x48/apps 
	$(MKDIR_P) $(DESTDIR)$(pioneers_iconthemedir)/scalable/apps
	for icon in $(icons); do \
		ICONNAME=`echo $$icon | $(AWK) '{ c = split($$0, a, "/"); print substr(a[c], 1, index(a[c], ".") - 1) }'`; \
		INPUTNAME=`echo $$icon | $(AWK) '{ gsub(".svg", ""); print }'`; \
		$(INSTALL_DATA) $(srcdir)/$$INPUTNAME.48x48_apps.png $(DESTDIR)$(pioneers_iconthemedir)/48x48/apps/$$ICONNAME.png; \
		$(INSTALL_DATA) $(srcdir)/$$INPUTNAME.svg $(DESTDIR)$(pioneers_iconthemedir)/scalable/apps/$$ICONNAME.svg; \
	done;

uninstall-icons:
	-for icon in $(icons); do \
		ICONNAME=`echo $$icon | $(AWK) '{ c = split($$0, a, "/"); print substr(a[c], 1, index(a[c], ".") - 1) }'`; \
		rm -f $(DESTDIR)$(pioneers_iconthemedir)/48x48/apps/$$ICONNAME.png; \
		rm -f $(DESTDIR)$(pioneers_iconthemedir)/scalable/apps/$$ICONNAME.svg; \
	done;

install-data-hook: install-icons

uninstall-hook: uninstall-icons

dist-hook:
	$(MAKE) -C "$(srcdir)/po" "${GETTEXT_PACKAGE}.pot"
	cp "$(srcdir)/po/${GETTEXT_PACKAGE}.pot" "$(distdir)/po/"
