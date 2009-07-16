#!/bin/sh

make || exit 1

killall -s USR1 adapter trkserver > /dev/null 2>&1
killall adapter trkserver > /dev/null 2>&1

trkservername="TRKSERVER-3";
gdbserverip=127.0.0.1
gdbserverport=2225
replaysource=dump.txt

./trkserver ${trkservername} ${replaysource} &
trkserverpid=$!

sleep 1

./adapter ${trkservername} ${gdbserverip}:${gdbserverport} &
adapterpid=$!

echo "
set remote noack-packet on
target remote ${gdbserverip}:${gdbserverport}
file filebrowseapp.sym
quit
" > gdb.txt

./arm-gdb -x gdb.txt

#sleep 4

kill -s USR1 ${adapterpid}
kill -s USR1 ${trkserverpid}

echo

#killall arm-gdb
