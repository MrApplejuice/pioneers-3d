#!/bin/sh
#
# Lists all running games:
# PID Port Game
#
ps -edaf | awk '$8 ~ "pioneers-server-console" && $0 ~ "-g" { portstart = index($0, "-p ") + 3; portend = index(substr($0, portstart, 255), "-"); titlestart = index($0, "-g") + 3; titleend = index(substr($0, titlestart, 255), "-P"); printf "%5d %s %s\n", $2+0, substr($0, portstart, portend - 2), substr($0, titlestart, titleend - 2); }' | sort -k  2
