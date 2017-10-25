#!/usr/bin/env python
################################################################################
# Copyright (C) The Qt Company Ltd.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#   * Neither the name of The Qt Company Ltd, nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
################################################################################

import os
import locale
import sys
import getopt
import subprocess
import re
import shutil
from glob import glob

import common

ignoreErrors = False
debug_build = False
encoding = locale.getdefaultlocale()[1]

def usage():
    print("Usage: %s <creator_install_dir> [qmake_path]" % os.path.basename(sys.argv[0]))

def which(program):
    def is_exe(fpath):
        return os.path.exists(fpath) and os.access(fpath, os.X_OK)

    fpath = os.path.dirname(program)
    if fpath:
        if is_exe(program):
            return program
        if common.is_windows_platform():
            if is_exe(program + ".exe"):
                return program  + ".exe"
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
            if common.is_windows_platform():
                if is_exe(exe_file + ".exe"):
                    return exe_file  + ".exe"

    return None

def is_debug(fpath):
    # match all Qt Core dlls from Qt4, Qt5beta2 and Qt5rc1 and later
    # which all have the number at different places
    coredebug = re.compile(r'Qt[1-9]?Core[1-9]?d[1-9]?.dll')
    # bootstrap exception
    if coredebug.search(fpath):
        return True
    output = subprocess.check_output(['dumpbin', '/imports', fpath])
    return coredebug.search(output.decode(encoding)) != None

def is_debug_build(install_dir):
    return is_debug(os.path.join(install_dir, 'bin', 'qtcreator.exe'))

def op_failed(details = None):
    if details != None:
        print(details)
    if ignoreErrors == False:
        print("Error: operation failed!")
        sys.exit(2)
    else:
        print("Error: operation failed, but proceeding gracefully.")

def is_ignored_windows_file(use_debug, basepath, filename):
    ignore_patterns = ['.lib', '.pdb', '.exp', '.ilk']
    if use_debug:
        ignore_patterns.extend(['libEGL.dll', 'libGLESv2.dll'])
    else:
        ignore_patterns.extend(['libEGLd.dll', 'libGLESv2d.dll'])
    for ip in ignore_patterns:
        if filename.endswith(ip):
            return True
    if filename.endswith('.dll'):
        filepath = os.path.join(basepath, filename)
        if use_debug != is_debug(filepath):
            return True
    return False

def ignored_qt_lib_files(path, filenames):
    if not common.is_windows_platform():
        return []
    return [fn for fn in filenames if is_ignored_windows_file(debug_build, path, fn)]

def copy_qt_libs(target_qt_prefix_path, qt_bin_dir, qt_libs_dir, qt_plugin_dir, qt_import_dir, qt_qml_dir, plugins, imports):
    print("copying Qt libraries...")

    if common.is_windows_platform():
        libraries = glob(os.path.join(qt_libs_dir, '*.dll'))
    else:
        libraries = glob(os.path.join(qt_libs_dir, '*.so.*'))

    if common.is_windows_platform():
        lib_dest = os.path.join(target_qt_prefix_path)
    else:
        lib_dest = os.path.join(target_qt_prefix_path, 'lib')

    if not os.path.exists(lib_dest):
        os.makedirs(lib_dest)

    if common.is_windows_platform():
        libraries = [lib for lib in libraries if not is_ignored_windows_file(debug_build, '', lib)]

    for library in libraries:
        print(library, '->', lib_dest)
        if os.path.islink(library):
            linkto = os.readlink(library)
            try:
                os.symlink(linkto, os.path.join(lib_dest, os.path.basename(library)))
            except OSError:
                op_failed("Link already exists!")
        else:
            shutil.copy(library, lib_dest)

    print("Copying plugins:", plugins)
    for plugin in plugins:
        target = os.path.join(target_qt_prefix_path, 'plugins', plugin)
        if (os.path.exists(target)):
            shutil.rmtree(target)
        pluginPath = os.path.join(qt_plugin_dir, plugin)
        if (os.path.exists(pluginPath)):
            print('{0} -> {1}'.format(pluginPath, target))
            common.copytree(pluginPath, target, ignore=ignored_qt_lib_files, symlinks=True)

    print("Copying imports:", imports)
    for qtimport in imports:
        target = os.path.join(target_qt_prefix_path, 'imports', qtimport)
        if (os.path.exists(target)):
            shutil.rmtree(target)
        import_path = os.path.join(qt_import_dir, qtimport)
        if os.path.exists(import_path):
            print('{0} -> {1}'.format(import_path, target))
            common.copytree(import_path, target, ignore=ignored_qt_lib_files, symlinks=True)

    if (os.path.exists(qt_qml_dir)):
        print("Copying qt quick 2 imports")
        target = os.path.join(target_qt_prefix_path, 'qml')
        if (os.path.exists(target)):
            shutil.rmtree(target)
        print('{0} -> {1}'.format(qt_qml_dir, target))
        common.copytree(qt_qml_dir, target, ignore=ignored_qt_lib_files, symlinks=True)

    print("Copying qtdiag")
    bin_dest = target_qt_prefix_path if common.is_windows_platform() else os.path.join(target_qt_prefix_path, 'bin')
    qtdiag_src = os.path.join(qt_bin_dir, 'qtdiag.exe' if common.is_windows_platform() else 'qtdiag')
    if not os.path.exists(bin_dest):
        os.makedirs(bin_dest)
    shutil.copy(qtdiag_src, bin_dest)


def add_qt_conf(target_path, qt_prefix_path):
    qtconf_filepath = os.path.join(target_path, 'qt.conf')
    prefix_path = os.path.relpath(qt_prefix_path, target_path).replace('\\', '/')
    print('Creating qt.conf in "{0}":'.format(qtconf_filepath))
    f = open(qtconf_filepath, 'w')
    f.write('[Paths]\n')
    f.write('Prefix={0}\n'.format(prefix_path))
    f.write('Binaries={0}\n'.format('bin' if common.is_linux_platform() else '.'))
    f.write('Libraries={0}\n'.format('lib' if common.is_linux_platform() else '.'))
    f.write('Plugins=plugins\n')
    f.write('Imports=imports\n')
    f.write('Qml2Imports=qml\n')
    f.close()

def copy_translations(install_dir, qt_tr_dir):
    translations = glob(os.path.join(qt_tr_dir, '*.qm'))
    tr_dir = os.path.join(install_dir, 'share', 'qtcreator', 'translations')

    print("copying translations...")
    for translation in translations:
        print(translation, '->', tr_dir)
        shutil.copy(translation, tr_dir)

def copyPreservingLinks(source, destination):
    if os.path.islink(source):
        linkto = os.readlink(source)
        destFilePath = destination
        if os.path.isdir(destination):
            destFilePath = os.path.join(destination, os.path.basename(source))
        os.symlink(linkto, destFilePath)
    else:
        shutil.copy(source, destination)

def deploy_libclang(install_dir, llvm_install_dir, chrpath_bin):
    # contains pairs of (source, target directory)
    deployinfo = []
    resourcesource = os.path.join(llvm_install_dir, 'lib', 'clang')
    if common.is_windows_platform():
        clangbindirtarget = os.path.join(install_dir, 'bin', 'clang', 'bin')
        if not os.path.exists(clangbindirtarget):
            os.makedirs(clangbindirtarget)
        clanglibdirtarget = os.path.join(install_dir, 'bin', 'clang', 'lib')
        if not os.path.exists(clanglibdirtarget):
            os.makedirs(clanglibdirtarget)
        deployinfo.append((os.path.join(llvm_install_dir, 'bin', 'libclang.dll'),
                           os.path.join(install_dir, 'bin')))
        deployinfo.append((os.path.join(llvm_install_dir, 'bin', 'clang.exe'),
                           clangbindirtarget))
        resourcetarget = os.path.join(clanglibdirtarget, 'clang')
    else:
        libsources = glob(os.path.join(llvm_install_dir, 'lib', 'libclang.so*'))
        for libsource in libsources:
            deployinfo.append((libsource, os.path.join(install_dir, 'lib', 'qtcreator')))
        clangbinary = os.path.join(llvm_install_dir, 'bin', 'clang')
        clangbinary_targetdir = os.path.join(install_dir, 'libexec', 'qtcreator', 'clang', 'bin')
        if not os.path.exists(clangbinary_targetdir):
            os.makedirs(clangbinary_targetdir)
        deployinfo.append((clangbinary, clangbinary_targetdir))
        # copy link target if clang is actually a symlink
        if os.path.islink(clangbinary):
            linktarget = os.readlink(clangbinary)
            deployinfo.append((os.path.join(os.path.dirname(clangbinary), linktarget),
                               os.path.join(clangbinary_targetdir, linktarget)))
        resourcetarget = os.path.join(install_dir, 'libexec', 'qtcreator', 'clang', 'lib', 'clang')

    print("copying libclang...")
    for source, target in deployinfo:
        print(source, '->', target)
        copyPreservingLinks(source, target)

    if common.is_linux_platform():
        # libclang was statically compiled, so there is no need for the RPATHs
        # and they are confusing when fixing RPATHs later in the process
        print("removing libclang RPATHs...")
        for source, target in deployinfo:
            if not os.path.islink(target):
                targetfilepath = target if not os.path.isdir(target) else os.path.join(target, os.path.basename(source))
                subprocess.check_call([chrpath_bin, '-d', targetfilepath])

    print(resourcesource, '->', resourcetarget)
    if (os.path.exists(resourcetarget)):
        shutil.rmtree(resourcetarget)
    common.copytree(resourcesource, resourcetarget, symlinks=True)

def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], 'hi', ['help', 'ignore-errors'])
    except getopt.GetoptError:
        usage()
        sys.exit(2)
    for o, _ in opts:
        if o in ('-h', '--help'):
            usage()
            sys.exit(0)
        if o in ('-i', '--ignore-errors'):
            global ignoreErrors
            ignoreErrors = True
            print("Note: Ignoring all errors")

    if len(args) < 1:
        usage()
        sys.exit(2)

    install_dir = args[0]
    if common.is_linux_platform():
        qt_deploy_prefix = os.path.join(install_dir, 'lib', 'Qt')
    else:
        qt_deploy_prefix = os.path.join(install_dir, 'bin')
    qmake_bin = 'qmake'
    if len(args) > 1:
        qmake_bin = args[1]
    qmake_bin = which(qmake_bin)

    if qmake_bin == None:
        print("Cannot find required binary 'qmake'.")
        sys.exit(2)

    chrpath_bin = None
    if common.is_linux_platform():
        chrpath_bin = which('chrpath')
        if chrpath_bin == None:
            print("Cannot find required binary 'chrpath'.")
            sys.exit(2)

    qt_install_info = common.get_qt_install_info(qmake_bin)
    QT_INSTALL_LIBS = qt_install_info['QT_INSTALL_LIBS']
    QT_INSTALL_BINS = qt_install_info['QT_INSTALL_BINS']
    QT_INSTALL_PLUGINS = qt_install_info['QT_INSTALL_PLUGINS']
    QT_INSTALL_IMPORTS = qt_install_info['QT_INSTALL_IMPORTS']
    QT_INSTALL_QML = qt_install_info['QT_INSTALL_QML']
    QT_INSTALL_TRANSLATIONS = qt_install_info['QT_INSTALL_TRANSLATIONS']

    plugins = ['accessible', 'codecs', 'designer', 'iconengines', 'imageformats', 'platformthemes', 'platforminputcontexts', 'platforms', 'printsupport', 'sqldrivers', 'styles', 'xcbglintegrations']
    imports = ['Qt', 'QtWebKit']

    if common.is_windows_platform():
        global debug_build
        debug_build = is_debug_build(install_dir)

    if common.is_windows_platform():
        copy_qt_libs(qt_deploy_prefix, QT_INSTALL_BINS, QT_INSTALL_BINS, QT_INSTALL_PLUGINS, QT_INSTALL_IMPORTS, QT_INSTALL_QML, plugins, imports)
    else:
        copy_qt_libs(qt_deploy_prefix, QT_INSTALL_BINS, QT_INSTALL_LIBS, QT_INSTALL_PLUGINS, QT_INSTALL_IMPORTS, QT_INSTALL_QML, plugins, imports)
    copy_translations(install_dir, QT_INSTALL_TRANSLATIONS)
    if "LLVM_INSTALL_DIR" in os.environ:
        deploy_libclang(install_dir, os.environ["LLVM_INSTALL_DIR"], chrpath_bin)

    if not common.is_windows_platform():
        print("fixing rpaths...")
        common.fix_rpaths(install_dir, os.path.join(qt_deploy_prefix, 'lib'), qt_install_info, chrpath_bin)
        add_qt_conf(os.path.join(install_dir, 'libexec', 'qtcreator'), qt_deploy_prefix) # e.g. for qml2puppet
        add_qt_conf(os.path.join(qt_deploy_prefix, 'bin'), qt_deploy_prefix) # e.g. qtdiag
    add_qt_conf(os.path.join(install_dir, 'bin'), qt_deploy_prefix)

if __name__ == "__main__":
    if common.is_mac_platform():
        print("macOS is not supported by this script, please use macqtdeploy!")
        sys.exit(2)
    else:
        main()
