#!/bin/sh
sleep 1
~/local/bin/pioneers-server-console --file ~/local/share/games/pioneers/lobby.game -r -m localhost -n pioneers.game-host.org -p 5560 -a 5559 --debug | tee `date +%Y-%m-%dT%H:%M:%S.local.log`
