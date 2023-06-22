# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import argparse
import os
import locale
import shutil
import subprocess
import sys

encoding = locale.getdefaultlocale()[1]
if not encoding:
    encoding = 'UTF-8'

def is_windows_platform():
    return sys.platform.startswith('win')

def is_linux_platform():
    return sys.platform.startswith('linux')

def is_mac_platform():
    return sys.platform.startswith('darwin')

def to_posix_path(path):
    if is_windows_platform():
        # should switch to pathlib from python3
        return path.replace('\\', '/')
    return path

def check_print_call(command, workdir=None, env=None):
    print('------------------------------------------')
    print('COMMAND:')
    print(' '.join(['"' + c.replace('"', '\\"') + '"' for c in command]))
    print('PWD:      "' + (workdir if workdir else os.getcwd()) + '"')
    print('------------------------------------------')
    subprocess.check_call(command, cwd=workdir, env=env)


def get_git_SHA(path):
    try:
        output = subprocess.check_output(['git', 'rev-list', '-n1', 'HEAD'], cwd=path).strip()
        decoded_output = output.decode(encoding) if encoding else output
        return decoded_output
    except subprocess.CalledProcessError:
        return None
    return None


# get commit SHA either directly from git, or from a .tag file in the source directory
def get_commit_SHA(path):
    git_sha = get_git_SHA(path)
    if not git_sha:
        tagfile = os.path.join(path, '.tag')
        if os.path.exists(tagfile):
            with open(tagfile, 'r') as f:
                git_sha = f.read().strip()
    return git_sha

# copy of shutil.copytree that does not bail out if the target directory already exists
# and that does not create empty directories
def copytree(src, dst, symlinks=False, ignore=None):
    def ensure_dir(destdir, ensure):
        if ensure and not os.path.isdir(destdir):
            os.makedirs(destdir)
        return False

    names = os.listdir(src)
    if ignore is not None:
        ignored_names = ignore(src, names)
    else:
        ignored_names = set()

    needs_ensure_dest_dir = True
    errors = []
    for name in names:
        if name in ignored_names:
            continue
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)
        try:
            if symlinks and os.path.islink(srcname):
                needs_ensure_dest_dir = ensure_dir(dst, needs_ensure_dest_dir)
                linkto = os.readlink(srcname)
                os.symlink(linkto, dstname)
            elif os.path.isdir(srcname):
                copytree(srcname, dstname, symlinks, ignore)
            else:
                needs_ensure_dest_dir = ensure_dir(dst, needs_ensure_dest_dir)
                shutil.copy2(srcname, dstname)
            # XXX What about devices, sockets etc.?
        except (IOError, os.error) as why:
            errors.append((srcname, dstname, str(why)))
        # catch the Error from the recursive copytree so that we can
        # continue with other files
        except shutil.Error as err:
            errors.extend(err.args[0])
    try:
        if os.path.exists(dst):
            shutil.copystat(src, dst)
    except shutil.WindowsError:
        # can't copy file access times on Windows
        pass
    except OSError as why:
        errors.extend((src, dst, str(why)))
    if errors:
        raise shutil.Error(errors)

def get_qt_install_info(qmake_bin):
    output = subprocess.check_output([qmake_bin, '-query'])
    decoded_output = output.decode(encoding) if encoding else output
    lines = decoded_output.strip().split('\n')
    info = {}
    for line in lines:
        (var, sep, value) = line.partition(':')
        info[var.strip()] = value.strip()
    return info

def get_rpath(libfilepath, chrpath=None):
    if chrpath is None:
        chrpath = 'chrpath'
    try:
        output = subprocess.check_output([chrpath, '-l', libfilepath]).strip()
        decoded_output = output.decode(encoding) if encoding else output
    except subprocess.CalledProcessError: # no RPATH or RUNPATH
        return []
    marker = 'RPATH='
    index = decoded_output.find(marker)
    if index < 0:
        marker = 'RUNPATH='
        index = decoded_output.find(marker)
    if index < 0:
        return []
    return decoded_output[index + len(marker):].split(':')

def fix_rpaths(path, qt_deploy_path, qt_install_info, chrpath=None):
    if chrpath is None:
        chrpath = 'chrpath'
    qt_install_prefix = qt_install_info['QT_INSTALL_PREFIX']
    qt_install_libs = qt_install_info['QT_INSTALL_LIBS']

    def fix_rpaths_helper(filepath):
        rpath = get_rpath(filepath, chrpath)
        if len(rpath) <= 0:
            return
        # remove previous Qt RPATH
        new_rpath = [path for path in rpath if not path.startswith(qt_install_prefix)
                     and not path.startswith(qt_install_libs)]

        # check for Qt linking
        lddOutput = subprocess.check_output(['ldd', filepath])
        lddDecodedOutput = lddOutput.decode(encoding) if encoding else lddOutput
        if lddDecodedOutput.find('libQt') >= 0 or lddDecodedOutput.find('libicu') >= 0:
            # add Qt RPATH if necessary
            relative_path = os.path.relpath(qt_deploy_path, os.path.dirname(filepath))
            if relative_path == '.':
                relative_path = ''
            else:
                relative_path = '/' + relative_path
            qt_rpath = '$ORIGIN' + relative_path
            if not any((path == qt_rpath) for path in rpath):
                new_rpath.append(qt_rpath)

        # change RPATH
        if len(new_rpath) > 0:
            subprocess.check_call([chrpath, '-r', ':'.join(new_rpath), filepath])
        else: # no RPATH / RUNPATH left. delete.
            subprocess.check_call([chrpath, '-d', filepath])

    def is_unix_executable(filepath):
        # Whether a file is really a binary executable and not a script and not a symlink (unix only)
        if os.path.exists(filepath) and os.access(filepath, os.X_OK) and not os.path.islink(filepath):
            with open(filepath, 'rb') as f:
                return f.read(2) != "#!"

    def is_unix_library(filepath):
        # Whether a file is really a library and not a symlink (unix only)
        return os.path.basename(filepath).find('.so') != -1 and not os.path.islink(filepath)

    for dirpath, dirnames, filenames in os.walk(path):
        for filename in filenames:
            filepath = os.path.join(dirpath, filename)
            if is_unix_executable(filepath) or is_unix_library(filepath):
                fix_rpaths_helper(filepath)

def is_debug_file(filepath):
    if is_mac_platform():
        return filepath.endswith('.dSYM') or '.dSYM/' in filepath
    elif is_linux_platform():
        return filepath.endswith('.debug')
    else:
        return filepath.endswith('.pdb')

def is_debug(path, filenames):
    return [fn for fn in filenames if is_debug_file(os.path.join(path, fn))]

def is_not_debug(path, filenames):
    files = [fn for fn in filenames if os.path.isfile(os.path.join(path, fn))]
    return [fn for fn in files if not is_debug_file(os.path.join(path, fn))]

def codesign_call(identity=None, flags=None):
    signing_identity = identity or os.environ.get('SIGNING_IDENTITY')
    if not signing_identity:
        return None
    codesign_call = ['codesign', '-o', 'runtime', '--force', '-s', signing_identity,
                     '-v']
    signing_flags = flags or os.environ.get('SIGNING_FLAGS')
    if signing_flags:
        codesign_call.extend(signing_flags.split())
    return codesign_call

def codesign_executable(path):
    codesign = codesign_call()
    if not codesign:
        return
    entitlements_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'dist',
                                     'installer', 'mac', os.path.basename(path) + '.entitlements')
    if os.path.exists(entitlements_path):
        codesign.extend(['--entitlements', entitlements_path])
    subprocess.check_call(codesign + [path])

def os_walk(path, filter, function):
    for r, _, fs in os.walk(path):
        for f in fs:
            ff = os.path.join(r, f)
            if filter(ff):
                function(ff)

def conditional_sign_recursive(path, filter):
    if is_mac_platform():
        os_walk(path, filter, lambda fp: codesign_executable(fp))

def codesign(app_path, identity=None, flags=None):
    codesign = codesign_call(identity, flags)
    if not codesign or not is_mac_platform():
        return
    # sign all executables in Resources
    conditional_sign_recursive(os.path.join(app_path, 'Contents', 'Resources'),
                               lambda ff: os.access(ff, os.X_OK))
    # sign all libraries in Imports
    conditional_sign_recursive(os.path.join(app_path, 'Contents', 'Imports'),
                               lambda ff: ff.endswith('.dylib'))

    # sign the whole bundle
    entitlements_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'dist',
                                     'installer', 'mac', 'entitlements.plist')
    subprocess.check_call(codesign + ['--deep', app_path, '--entitlements', entitlements_path])

def codesign_main(args):
    codesign(args.app_bundle, args.identity, args.flags)

def main():
    parser = argparse.ArgumentParser(description='Qt Creator build tools')
    subparsers = parser.add_subparsers(title='subcommands', required=True)
    parser_codesign = subparsers.add_parser('codesign', description='Codesign macOS app bundle')
    parser_codesign.add_argument('app_bundle')
    parser_codesign.add_argument('-s', '--identity', help='Codesign identity')
    parser_codesign.add_argument('--flags', help='Additional flags')
    parser_codesign.set_defaults(func=codesign_main)
    args = parser.parse_args()
    args.func(args)

if __name__ == '__main__':
    main()
