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

echo "# This is generated. Changes will be lost.
#set remote noack-packet on
set confirm off
set endian big
#set debug remote 1
#target remote ${gdbserverip}:${gdbserverport}
target extended-remote ${gdbserverip}:${gdbserverport}
#file filebrowseapp.sym
add-symbol-file filebrowseapp.sym 0x786A4000
symbol-file filebrowseapp.sym
p E32Main
#continue
#info files
#file filebrowseapp.sym -readnow
#add-symbol-file filebrowseapp.sym 0x786A4000
" > .gdbinit

#kill -s USR1 ${adapterpid}
#kill -s USR1 ${trkserverpid}
#killall arm-gdb
