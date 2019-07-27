#!/bin/sh
#
# This script will list the most current version of Pioneers in various
# distributions.
#
# The output is comma delimited:
#  Column 1: the name of the distribution
#  Column 2: the most recent version
#
# When adding new distributions, keep the alphabetic order
# (but source code first)
#
# Copyright 2006, 2010 Roland Clobus <rclobus@rclobus.nl>
#
# This script is distributed with the same license as the Pioneers package

# Source code
wget -o /dev/null -O - "http://sourceforge.net/projects/pio/files/Source" | awk '$0 ~ /title=..Source.pioneers-.*.tar.gz/ { a = match($0, "pioneers-(.*).tar.gz"); b = substr($0, a+9, RLENGTH-9-7); split(b, c, "."); printf "%06d %s\n", c[1]*10000+c[2]*100+c[3]+0, b }' | sort -u -r | awk '{ print "Source," $2 }' | head -1
# Debian
wget -o /dev/null -O - "http://packages.qa.debian.org/p/pioneers.html" | awk '$0 ~ "latest_version" { gsub("<[^>]+>", ""); print "Debian," $1 }'

# Fink
wget -o /dev/null -O - "http://pdb.finkproject.org/pdb/package.php/pioneers" | awk '$0 ~ "Package pioneers" { a = match($0, "pioneers-(.*)\<"); b = substr($0, a+9, RLENGTH-9-1); print "Fink," b }' | tail -1

# FreeBSD
wget -o /dev/null -O - --user-agent="" "http://www.freebsd.org/cgi/ports.cgi?query=pioneers&stype=all" | awk '$0 ~ "a name" { gsub("<[^>]+>", ""); gsub("pioneers-", ""); print "FreeBSD," $1 }' | tail -1

# Gentoo
#wget -o /dev/null -O - --user-agent="" "http://packages.gentoo.org/search/?sstring=pioneers" | awk '$0 ~ "<th class=\"releases\">" { gsub("<[^>]+>", ""); print $1 }' | head -1 | awk '{ print "Gentoo," $1 }'

# Mandriva
#wget -o /dev/null -O - "http://rpms.mandrivaclub.com/search.php?query=pioneers&submit=Search+...&arch=i586" | awk ' $0 ~ "<tr bgcolor" { gsub("<[^>]+>", " "); a = $1; gsub("pioneers-", "", a); gsub(".i586.html", "", a); print "Mandriva," a }'

# Microsoft Windows
wget -o /dev/null -O - "http://sourceforge.net/projects/pio/files/Windows installer/" | awk '$0 ~ /title=..Windows.installer.Pioneers-.*.setup.exe/ { a = match($0, "Pioneers-(.*)-setup.exe"); b = substr($0, a+9, RLENGTH-9-10); split(b, c, "."); printf "%06d %s\n", c[1]*10000+c[2]*100+c[3]+0, b }' | sort -u -r | awk '{ print "Microsoft Windows," $2 }' | head -1

# NetBSD
#wget -o /dev/null -O - "ftp://ftp.netbsd.org/pub/pkgsrc/current/pkgsrc/games/pioneers/README.html" | awk '$0 ~ "The current source" { getline; gsub("\"pioneers-", ""); gsub("\".", ""); print "NetBSD," $1 }'

# OpenBSD
wget -o /dev/null -O - "http://www.openbsd.org/cgi-bin/cvsweb/~checkout~/ports/games/pioneers/distinfo?content-type=text%2Fplain" | awk '$0 ~ "MD5" { a = $2; gsub(".pioneers-", "", a); gsub(".tar.gz.", "", a); print "OpenBSD," a }'

# Redhat FC4/openSUSE 10.2
#wget -o /dev/null -O - "http://sourceforge.net/project/showfiles.php?group_id=5095&package_id=181682" | awk '$0 ~ /pioneers-[0-9].*.rpm/ { a = match($0, ".*pioneers-(.*).rpm", b); if (a==1) { split(b[1], c, "."); printf "%06d %s\n", c[1]*10000+c[2]*100+c[3], b[1] } }' | sort -u -r | awk '{ print "RedHat/openSUSE RPM," $2 }' | head -1

# Ubuntu
wget -o /dev/null -O - "https://launchpad.net/distros/ubuntu/+source/pioneers" | awk '$0 ~ "Latest upload" { getline; gsub("<[^>]+>", ""); print "Ubuntu," $1 }'
