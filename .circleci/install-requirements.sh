#!/bin/bash

set -e

apt-get update > /dev/null &

wget -O /tmp/cmake.tgz https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.tar.gz
tar -x -C /opt/ -f /tmp/cmake.tgz
mv /opt/cmake-* /opt/cmake

wget -O /tmp/gtk.tar.xz http://ftp.gnome.org/pub/gnome/sources/gtk+/3.24/gtk+-3.24.11.tar.xz
mkdir deps
tar -x -C deps -f /tmp/gtk.tar.xz

# cmake in: /opt/cmake/bin/

wait

# Build
apt-get install -y shtool

# Build requirements
apt-get install -y intltool libgtk2.0-dev libgtk-3-dev libnotify-dev yelp-tools
apt-get install -y libavahi-client-dev libavahi-glib-dev gob2 librsvg2-bin
