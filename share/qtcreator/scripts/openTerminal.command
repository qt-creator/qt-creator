#! /bin/bash

i=`pwd`
i=${i//\\/\\\\\\\\}
i=${i//\"/\\\\\\\"}
i=${i//\$/\\\\\\\$}
i=${i//\`/\\\\\\\`}
i=\\\"$i\\\"
osascript <<EOF
    --Terminal opens a window by default when it is not running, so check
    on applicationIsRunning(applicationName)
            tell application "System Events" to count (every process whose name is applicationName)
            return result is greater than 0
    end applicationIsRunning
    set terminalWasRunning to applicationIsRunning("Terminal")

    set cdScript to "cd $i"
    tell application "Terminal"
        --do script will open a new window if none given, but terminal already opens one if not running
        if terminalWasRunning then
            do script cdScript
        else
            do script cdScript in first window
        end if
        set currentTab to the result
        set currentWindow to first window whose tabs contains currentTab
        activate
    end tell
EOF
