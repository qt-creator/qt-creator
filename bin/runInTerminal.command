#!/bin/bash
osascript >/dev/null 2>&1 <<EOF
    tell application "Terminal"
    activate
    do script with command "$@; exit"
    end tell
EOF
