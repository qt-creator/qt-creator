#!/usr/bin/env python

############################################################################
#
# Copyright (C) 2018 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

import os
import pipes
import subprocess
import sys

def quote_for_applescript(arg):
    return arg.replace('\\', '\\\\').replace('"', '\\"')

def quote(arg):
    return quote_for_applescript(pipes.quote(arg))

def quote_args(args):
    return [quote(arg) for arg in args]

def cd_cwd_arguments():
    return ['cd', os.getcwd()]

def apple_script(shell_script):
    return '''
--Terminal opens a window by default when it is not running, so check
on applicationIsRunning(applicationName)
        tell application "System Events" to count (every process whose name is applicationName)
        return result is greater than 0
end applicationIsRunning
set terminalWasRunning to applicationIsRunning("Terminal")

set cdScript to "{}"
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
'''.format(shell_script)

def main():
    args = quote_args(cd_cwd_arguments()) + [';'] + quote_args(sys.argv[1:])
    shell_script = ' '.join(args)
    osascript_process = subprocess.Popen(['/usr/bin/osascript'], stdin=subprocess.PIPE)
    osascript_process.communicate(apple_script(shell_script))

if __name__ == "__main__":
    main()
