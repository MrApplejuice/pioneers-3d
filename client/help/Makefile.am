# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 2018 Roland Clobus <rclobus@rclobus.nl>
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

@YELP_HELP_RULES@

HELP_ID=pioneers
HELP_FILES = \
	index.docbook \
	legal.xml

# Keep this list in alphabetical order
HELP_MEDIA = \
	images/actions.png \
	images/brick.png \
	images/chat.png \
	images/client.png \
	images/connect-dialog.png \
	images/desert.png \
	images/develop-cards.png \
	images/discard-dialog.png \
	images/discards.png \
	images/field.png \
	images/forest.png \
	images/gameover-dialog.png \
	images/gold.png \
	images/grain.png \
	images/hill.png \
	images/identity.png \
	images/join-private-dialog.png \
	images/legend-dialog.png \
	images/lumber.png \
	images/map.png \
	images/messages.png \
	images/monopoly-dialog.png \
	images/mountain.png \
	images/ore.png \
	images/pasture.png \
	images/place-robber.png \
	images/player-summary.png \
	images/plenty-dialog.png \
	images/quote.png \
	images/resources.png \
	images/sea.png \
	images/server-create.png \
	images/servers-dialog.png \
	images/status.png \
	images/steal-from.png \
	images/trade.png \
	images/wool.png

HELP_LINGUAS=

# Various snippets
# HTMLHelp is the compressed help format for Microsoft Windows
htmlhelp: pioneers.xml
	xmlto -m custom.xsl htmlhelp pioneers.xml
	-/cygdrive/c/Program\ Files/HTML\ Help\ Workshop/hhc.exe htmlhelp.hhp
	rm htmlhelp.hhp
	rm toc.hhc
	rm *.html

simplehtml: C/pioneers.xml
	cd C
	xmlto -m custom.xsl html pioneers.xml
	cd ..

yelphtml: C/pioneers.xml
	cd C
	xsltproc /usr/share/yelp-xsl/xslt/docbook/html/db2html.xsl pioneers.xml
	cp /usr/share/yelp-xsl/icons/hicolor/24x24/status/yelp-note*.png .
	cd ..

xmllint: C/pioneers.xml
	xmllint --format C/pioneers.xml -o pioneers.xml.good
	mv pioneers.xml.good C/pioneers.xml
