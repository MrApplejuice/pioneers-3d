#!/bin/sh
#
# Kill the game with the given port number
#
# Usage: kill-game.sh port
#
# Note: you'll need to have the proper access rights
#
pkill -f pioneers-server-console.*-p.$1
