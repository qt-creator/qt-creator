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
from tempfile import NamedTemporaryFile

def quote_applescript(arg):
    return arg.replace('\\', '\\\\').replace('"', '\\"')

def quote_shell(arg):
    return pipes.quote(arg)

def clean_environment_script():
    # keep some basic environment settings to ensure functioning terminal and config files
    env_to_keep = ' '.join(['_', 'HOME', 'LOGNAME', 'PWD', 'SHELL', 'TMPDIR', 'USER', 'TERM',
                            'TERM_PROGRAM', 'TERM_PROGRAM_VERSION', 'TERM_SESSION_CLASS_ID',
                            'TERM_SESSION_ID'])
    return '''
function ignore() {
  local keys="''' + env_to_keep + '''"
  local v=$1
  for e in $keys; do [[ "$e" == "$v" ]] && return 0; done
}
while read -r line; do
  key=$(echo $line | /usr/bin/cut -d '=' -f 1)
  ignore $key || unset $key
done < <(env)
'''

def system_login_script():
    return 'if [ -f /etc/profile ]; then source /etc/profile; fi\n'

def login_script():
    return '''
if [ -f $HOME/.bash_profile ]; then
  source $HOME/.bash_profile
elif [ -f $HOME/.bash_login ]; then
  source $HOME/.bash_login ]
elif [ -f $HOME/.profile ]; then
  source $HOME/.profile
fi
'''

def environment_script():
    return ''.join(['export ' + quote_shell(key + '=' + os.environ[key]) + '\n'
                    for key in os.environ])

def apple_script(shell_command):
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
'''.format(shell_command)

def main():
    # create temporary file to be sourced into bash that deletes itself
    with NamedTemporaryFile(delete=False) as shell_script:
        quoted_shell_script = quote_shell(shell_script.name)
        commands = (clean_environment_script() +
                    system_login_script() + # /etc/profile by default resets the path, so do first
                    environment_script() +
                    login_script() +
                    'cd ' + quote_shell(os.getcwd()) + '\n' +
                    ' '.join([quote_shell(arg) for arg in sys.argv[1:]]) + '\n' +
                    'rm ' + quoted_shell_script + '\n'
                    )
        shell_script.write(commands)
        shell_script.flush()
        shell_command = quote_applescript('source ' + quoted_shell_script)
        osascript_process = subprocess.Popen(['/usr/bin/osascript'], stdin=subprocess.PIPE)
        osascript_process.communicate(apple_script(shell_command))

if __name__ == "__main__":
    main()
