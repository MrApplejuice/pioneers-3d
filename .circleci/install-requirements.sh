#!/bin/bash

set -e

#( 
#  wget -O /tmp/gtk.tar.xz http://ftp.gnome.org/pub/gnome/sources/gtk+/3.24/gtk+-3.24.11.tar.xz ;
#  tar -x -C $HOME/deps -f /tmp/gtk.tar.xz 
#) &

#(
#  wget -O /tmp/glib.tar.xz http://ftp.gnome.org/pub/gnome/sources/glib/2.60/glib-2.60.7.tar.xz ;
#  tar -x -C $HOME/deps -f /tmp/glib.tar.xz 
#) &

# Build requirements


sudo -i -u build bash -c "cd /home/build/ogre-1.12.2/build && make -j6"
