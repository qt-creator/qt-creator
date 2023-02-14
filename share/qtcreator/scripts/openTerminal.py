#!/usr/bin/env python3

# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import pipes
import stat
import subprocess
import sys
from tempfile import NamedTemporaryFile

def quote_shell(arg):
    return pipes.quote(arg)

def clean_environment_script():
    # keep some basic environment settings to ensure functioning terminal and config files
    env_to_keep = ' '.join(['_', 'HOME', 'LOGNAME', 'PWD', 'SHELL', 'TMPDIR', 'USER', 'TERM',
                            'TERM_PROGRAM', 'TERM_PROGRAM_VERSION', 'TERM_SESSION_CLASS_ID',
                            'TERM_SESSION_ID'])
    return r'''
function ignore() {
  local keys=(''' + env_to_keep + ''')
  local v=$1
  for e in "${keys[@]}"; do [[ "$e" == "$v" ]] && return 0; done
}
while read -r line; do
  key=$(echo $line | /usr/bin/cut -d '=' -f 1)
  ignore $key || unset $key
done < <(env)
'''

def system_login_script_bash():
    return r'''
[ -r /etc/profile ] && source /etc/profile
# fake behavior of /etc/profile as if BASH was set. It isn't for non-interactive shell
export PS1='\h:\W \u\$ '
[ -r /etc/bashrc ] && source /etc/bashrc
'''

def login_script_bash():
    return r'''
if [ -f $HOME/.bash_profile ]; then
  source $HOME/.bash_profile
elif [ -f $HOME/.bash_login ]; then
  source $HOME/.bash_login ]
elif [ -f $HOME/.profile ]; then
  source $HOME/.profile
fi
'''

def system_login_script_zsh():
    return '[ -r /etc/profile ] && source /etc/profile\n'

def login_script_zsh():
    return r'''
[ -r $HOME/.zprofile ] && source $HOME/.zprofile
[ -r $HOME/.zshrc ] && source $HOME/.zshrc
[ -r $HOME/.zlogin ] && source $HOME/.zlogin
'''

def environment_script():
    return ''.join(['export ' + quote_shell(key + '=' + os.environ[key]) + '\n'
                    for key in os.environ])

def zsh_setup(shell):
    return (shell,
            system_login_script_zsh,
            login_script_zsh,
            shell + ' -c',
            shell + ' -d -f')

def bash_setup(shell):
    bash = shell if shell is not None and shell.endswith('/bash') else '/bin/bash'
    return (bash,
            system_login_script_bash,
            login_script_bash,
            bash + ' -c',
            bash + ' --noprofile -l')

def main():
    # create temporary file to be sourced into bash that deletes itself
    with NamedTemporaryFile(mode='wt', delete=False) as shell_script:
        shell = os.environ.get('SHELL')
        shell, system_login_script, login_script, non_interactive_shell, interactive_shell = (
            zsh_setup(shell) if shell is not None and shell.endswith('/zsh')
            else bash_setup(shell))

        commands = ('#!' + shell + '\n' +
                    'rm ' + quote_shell(shell_script.name) + '\n' +
                    clean_environment_script() +
                    system_login_script() + # /etc/(z)profile by default resets the path, so do first
                    environment_script() +
                    login_script() +
                    'cd ' + quote_shell(os.getcwd()) + '\n' +
                    ('exec ' + non_interactive_shell + ' ' +
                     quote_shell(' '.join([quote_shell(arg) for arg in sys.argv[1:]])) + '\n'
                     if len(sys.argv) > 1 else 'exec ' + interactive_shell + '\n')
                    )
        shell_script.write(commands)
        shell_script.flush()
        os.chmod(shell_script.name, stat.S_IXUSR | stat.S_IRUSR | stat.S_IWUSR)
        # TODO /usr/bin/open doesn't work with notarized app in macOS 13,
        #      use osascript instead (QTCREATORBUG-28683).
        #      This has the disadvantage that the Terminal windows doesn't close
        #      automatically anymore.
        # subprocess.call(['/usr/bin/open', '-a', 'Terminal', shell_script.name])
        subprocess.call(['/usr/bin/osascript', '-e', 'tell app "Terminal" to activate'])
        subprocess.call(['/usr/bin/osascript', '-e', 'tell app "Terminal" to do script "' + shell_script.name + '"'])

if __name__ == "__main__":
    main()
