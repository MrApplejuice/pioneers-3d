#!/bin/bash

set -e

apt-get update

apt-get install -y wget tar

wget -O /tmp/cmake.tgz https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.tar.gz
tar -x -C /opt/ -f /tmp/cmake.tgz
mv /opt/cmake-* /opt/cmake

#mkdir $HOME/deps

#( 
#  wget -O /tmp/gtk.tar.xz http://ftp.gnome.org/pub/gnome/sources/gtk+/3.24/gtk+-3.24.11.tar.xz ;
#  tar -x -C $HOME/deps -f /tmp/gtk.tar.xz 
#) &

#(
#  wget -O /tmp/glib.tar.xz http://ftp.gnome.org/pub/gnome/sources/glib/2.60/glib-2.60.7.tar.xz ;
#  tar -x -C $HOME/deps -f /tmp/glib.tar.xz 
#) &

# Build requirements
apt-get install -y intltool libgtk2.0-dev libgtk-3-dev libnotify-dev yelp-tools
apt-get install -y libavahi-client-dev libavahi-glib-dev gob2 librsvg2-bin
