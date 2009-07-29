#!/bin/sh

ADAPTER_OPTIONS=""
TRKSERVEROPTIONS=""
DUMP_POSTFIX='-BigEndian.bin'
ENDIANESS='big'

while expr " $1" : " -.*" >/dev/null
do
  if [ " $1" = " -av" ]
  then
    ADAPTER_OPTIONS="$ADAPTER_OPTIONS -v"
  elif [ " $1" = " -aq" ]
  then
     ADAPTER_OPTIONS="$ADAPTER_OPTIONS -q"
  elif [ " $1" = " -tv" ]
  then
     TRKSERVEROPTIONS="$TRKSERVEROPTIONS -v"
  elif [ " $1" = " -tq" ]
  then
     TRKSERVEROPTIONS="$TRKSERVEROPTIONS -q"
  elif [ " $1" = " -l" ]
  then
     DUMP_POSTFIX='.bin'
     ENDIANESS='little'
     ADAPTER_OPTIONS="$ADAPTER_OPTIONS -l"
  fi
  shift 1
done

make || exit 1

killall -s USR1 adapter trkserver > /dev/null 2>&1
killall adapter trkserver > /dev/null 2>&1

userid=`id -u`
trkservername="TRKSERVER-${userid}";
gdbserverip=127.0.0.1
gdbserverport=$[2222 + ${userid}]

fuser -n tcp -k ${gdbserverport} 
rm /tmp/${trkservername}

MEMORYDUMP="TrkDump-78-6a-40-00$DUMP_POSTFIX"
ADDITIONAL_DUMPS="0x00402000$DUMP_POSTFIX 0x786a4000$DUMP_POSTFIX 0x00600000$DUMP_POSTFIX"
./trkserver $TRKSERVEROPTIONS ${trkservername} ${MEMORYDUMP} ${ADDITIONAL_DUMPS}&
trkserverpid=$!

sleep 1
./adapter $ADAPTER_OPTIONS ${trkservername} ${gdbserverip}:${gdbserverport} &
adapterpid=$!

echo "# This is generated. Changes will be lost.
#set remote noack-packet on
set confirm off
set endian $ENDIANESS
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
