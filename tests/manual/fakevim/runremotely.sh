#!/bin/sh
account=berlin@hd
sourcedir=/data5/dev/creator/tests/manual/fakevim/
exename=fakevim
targetdir=/tmp/run-${exename}

executable=${sourcedir}/${exename}
qtlibs=`ldd ${executable} | grep libQt | sed -e 's/^.*=> \(.*\) (.*)$/\1/'`

ssh ${account} "mkdir -p ${targetdir}" || exit 1
scp ${executable} ${qtlibs} ${account}:${targetdir} || exit 1
ssh ${account} "chrpath -r ${targetdir} ${targetdir}/*" || exit 1
ssh -X ${account} "gdbserver localhost:5555 ${targetdir}/${exename}" || exit 1
ssh ${account} "rm ${targetdir}/* ; rmdir ${targetdir}" || exit 1
exit 0

