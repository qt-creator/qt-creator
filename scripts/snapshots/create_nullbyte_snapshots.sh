#!/usr/bin/env bash

## Open script-dir-homed subshell
(
ABS_SCRIPT_DIR=`pwd`/`dirname "$0"`
cd "${ABS_SCRIPT_DIR}"


print_run() {
    echo "$@"
    "$@"
}

rand_range() {
    incMin=$1
    incMax=$2
    echo $((RANDOM*(incMax-incMin+1)/32768+incMin))
}

rand_range_list() {
    for ((i=0;i<$3;++i)); do
        rand_range $1 $2
    done
}


identifiers="\
linux-x86-setup.bin \
linux-x86_64-setup.bin \
linux-x86-gcc3.3-setup.bin \
mac-setup.dmg \
windows-setup.exe \
"

hour=23
minutes=59

for version in 0.9 ; do
for year in $(rand_range 2007 2008) ; do
for month in $(rand_range_list 1 12 3) ; do
for day in $(rand_range_list 1 28 10) ; do
    dir=`printf '%04d-%02d-%02d' ${year} ${month} ${day}`
    print_run mkdir -p ${dir}
    timestamp=`printf '%04d%02d%02d%02d%02d' ${year} ${month} ${day} ${hour} ${minutes}`
    shared="qtcreator-${version}-${timestamp}"
    for i in ${identifiers} ; do
        print_run touch "${dir}/${shared}-${i}"
    done
done
done
done
done
exit 0


## Properly close subshell
) 
exit $?
