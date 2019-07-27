# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003, 2006 Bas Wijnen <shevek@fmf.nl>
# Copyright (C) 2004, 2010 Roland Clobus <rclobus@rclobus.nl>
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

noinst_LIBRARIES += libpioneers.a

if HAVE_GTK
include common/gtk/Makefile.am
endif

libpioneers_a_CPPFLAGS = $(console_cflags)

libpioneers_a_SOURCES = \
	common/authors.h \
	common/buildrec.c \
	common/buildrec.h \
	common/cards.c \
	common/cards.h \
	common/common_glib.c \
	common/common_glib.h \
	common/cost.c \
	common/cost.h \
	common/deck.c \
	common/deck.h \
	common/driver.c \
	common/driver.h \
	common/game.c \
	common/game.h \
	common/game-list.c \
	common/game-list.h \
	common/log.c \
	common/log.h \
	common/map.c \
	common/map.h \
	common/map_query.c \
	common/network.c \
	common/network.h \
	common/notifying-string.gob \
	common/notifying-string.gob.stamp \
	common/notifying-string.c \
	common/notifying-string.h \
	common/notifying-string-private.h \
	common/quoteinfo.c \
	common/quoteinfo.h \
	common/random.c \
	common/random.h \
	common/set.c \
	common/set.h \
	common/state.c \
	common/state.h \
	common/version.h

common/authors.h: AUTHORS
	$(MKDIR_P) common
	printf '#define AUTHORLIST ' > $@
	$(SED) -e's/ <.*//; s/$$/", \\/; s/^/"/; /^"[[:space:]]*", \\$$/d' $< >> $@
	printf 'NULL\n' >> $@

common/version.h: build_version


# This target is not called common/version.h (although it builds that file),
# because it must be PHONY, but should only be rebuilt once.
build_version:
	$(MKDIR_P) common
	printf '#define FULL_VERSION "$(VERSION)' > common/version.new
	if svn info > /dev/null 2>&1; then				\
		svn info |                                              \
		$(AWK) '$$1 == "Revision:" { printf ".r%s", $$2 }'      \
			>> common/version.new                           ;\
		if svn status | $(GREP) -vq ^\? ; then			\
			printf '.M' >> common/version.new		;\
		fi							;\
	fi
	printf '"\n' >> common/version.new
	if diff common/version.h common/version.new > /dev/null 2>&1; then \
		rm common/version.new					;\
	else								\
		mv common/version.new common/version.h			;\
	fi

BUILT_SOURCES += \
	common/authors.h \
	common/notifying-string.gob.stamp \
	common/notifying-string.c \
	common/notifying-string.h \
	common/notifying-string-private.h \
	common/version.h

CLEANFILES += common/authors.h common/version.h

MAINTAINERCLEANFILES += \
	common/notifying-string.gob.stamp \
	common/notifying-string.c \
	common/notifying-string.h \
	common/notifying-string-private.h

# always try to rebuild version.h
.PHONY: build_version
