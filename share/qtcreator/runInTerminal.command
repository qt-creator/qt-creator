#!/bin/bash
osascript >/dev/null 2>&1 <<EOF
    tell application "Terminal"
        do script "$1 $2 +$3 +\"normal $4|\"; exit"
        set currentTab to the result
        set number of columns of currentTab to $5
        set number of rows of currentTab to $6
        set position of windows whose tabs contains currentTab to {$7, $8}
        activate
    end tell
EOF
