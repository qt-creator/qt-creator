#! /bin/bash

### FIXME:
# - currentTab and geometry stuff does not work with macX 10.4 (tiger)
# - -async is always in effect, i.e., synchronous execution is not implemented

geom=
async=
while test -n "$1"; do
    case $1 in
    -async)
        async=1
        shift;;
    -geom)
        shift
        w=${1%%x*}
        y=${1#*x}
        h=${y%%+*}
        y=${y#*+}
        x=${y%%+*}
        y=${y#*+}
        geom="\
        set number of columns of currentTab to $w
        set number of rows of currentTab to $h
        set position of windows whose tabs contains currentTab to {$x, $y}"
        shift;;
    -e)
        shift
        break;;
    *)
        echo "Invalid call" >&2
        exit 1;;
    esac
done
args=
for i in "$@"; do
    i=${i//\\/\\\\\\\\}
    i=${i//\"/\\\\\\\"}
    i=${i//\$/\\\\\\\$}
    i=${i//\`/\\\\\\\`}
    args="$args \\\"$i\\\""
done
osascript <<EOF
    tell application "Terminal"
        do script "$args; exit"
        set currentTab to the result
$geom
        activate
    end tell
EOF
