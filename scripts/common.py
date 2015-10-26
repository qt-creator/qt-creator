#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

import os
import shutil
import subprocess
import sys

def is_windows_platform():
    return sys.platform.startswith('win')

def is_linux_platform():
    return sys.platform.startswith('linux')

def is_mac_platform():
    return sys.platform.startswith('darwin')

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
    lines = output.strip().split('\n')
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
    except subprocess.CalledProcessError: # no RPATH or RUNPATH
        return []
    marker = 'RPATH='
    index = output.find(marker)
    if index < 0:
        marker = 'RUNPATH='
        index = output.find(marker)
    if index < 0:
        return []
    return output[index + len(marker):].split(':')

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
        new_rpath = filter(lambda path: not path.startswith(qt_install_prefix) and not path.startswith(qt_install_libs),
                           rpath)

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
            with open(filepath) as f:
                return f.read(2) != "#!"

    def is_unix_library(filepath):
        # Whether a file is really a library and not a symlink (unix only)
        return os.path.basename(filepath).find('.so') != -1 and not os.path.islink(filepath)

    for dirpath, dirnames, filenames in os.walk(path):
        for filename in filenames:
            filepath = os.path.join(dirpath, filename)
            if is_unix_executable(filepath) or is_unix_library(filepath):
                fix_rpaths_helper(filepath)

