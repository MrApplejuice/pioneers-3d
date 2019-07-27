#!/bin/sh
~/local/bin/pioneers-server-console --file ~/local/share/games/pioneers/lobby.game -r -n pioneers.game-host.org -p 5558 --debug | tee `date +%Y-%m-%dT%H:%M:%S.remote.log`
