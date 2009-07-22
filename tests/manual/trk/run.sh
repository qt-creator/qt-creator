#!/bin/sh

make || exit 1

killall -s USR1 adapter trkserver > /dev/null 2>&1
killall adapter trkserver > /dev/null 2>&1

trkservername="TRKSERVER-4";
gdbserverip=127.0.0.1
gdbserverport=2226
memorydump=TrkDump-78-6a-40-00.bin

fuser -n tcp -k ${gdbserverport} 
rm /tmp/${trkservername}

./trkserver ${trkservername} ${memorydump} &
trkserverpid=$!

sleep 1

./adapter ${trkservername} ${gdbserverip}:${gdbserverport} &
adapterpid=$!

echo "
# This is generated. Changes will be lost.
set remote noack-packet on
set endian big
target remote ${gdbserverip}:${gdbserverport}
file filebrowseapp.sym
" > .gdbinit

#kill -s USR1 ${adapterpid}
#kill -s USR1 ${trkserverpid}
#killall arm-gdb
