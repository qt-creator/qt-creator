#!/bin/sh
account=berlin@hd
sourcedir=/data5/dev/creator/tests/manual/fakevim/
exename=fakevim
targetdir=/tmp/run-${exename}

executable=${sourcedir}/${exename}
qtlibs=`ldd ${executable} | grep libQt | sed -e 's/^.*=> \(.*\) (.*)$/\1/'`

ssh ${account} "mkdir -p ${targetdir}"
scp ${executable} ${qtlibs} ${account}:${targetdir}
ssh ${account} "chrpath -r ${targetdir} ${targetdir}/*"
ssh -X ${account} "gdbserver localhost:5555 ${targetdir}/${exename}"
ssh ${account} "rm ${targetdir}/* ; rmdir ${targetdir}"

