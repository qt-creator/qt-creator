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

import argparse
import collections
import os
import locale
import sys
import subprocess
import re
import shutil
from glob import glob

import common

debug_build = False
encoding = locale.getdefaultlocale()[1]

def get_args():
    parser = argparse.ArgumentParser(description='Deploy Qt Creator dependencies for packaging')
    parser.add_argument('-i', '--ignore-errors', help='For backward compatibility',
                        action='store_true', default=False)
    parser.add_argument('--elfutils-path',
                        help='Path to elfutils installation for use by perfprofiler (Windows, Linux)')
    # TODO remove defaulting to LLVM_INSTALL_DIR when we no longer build qmake based packages
    parser.add_argument('--llvm-path',
                        help='Path to LLVM installation',
                        default=os.environ.get('LLVM_INSTALL_DIR'))
    parser.add_argument('qtcreator_binary', help='Path to Qt Creator binary (or the app bundle on macOS)')
    parser.add_argument('qmake_binary', help='Path to qmake binary')

    args = parser.parse_args()

    args.qtcreator_binary = os.path.abspath(args.qtcreator_binary)
    if common.is_mac_platform():
        if not args.qtcreator_binary.lower().endswith(".app"):
            args.qtcreator_binary = args.qtcreator_binary + ".app"
        check = os.path.isdir
    else:
        check = os.path.isfile
        if common.is_windows_platform() and not args.qtcreator_binary.lower().endswith(".exe"):
            args.qtcreator_binary = args.qtcreator_binary + ".exe"

    if not check(args.qtcreator_binary):
        print('Cannot find Qt Creator binary.')
        sys.exit(1)

    args.qmake_binary = which(args.qmake_binary)
    if not args.qmake_binary:
        print('Cannot find qmake binary.')
        sys.exit(2)

    return args

def usage():
    print("Usage: %s <existing_qtcreator_binary> [qmake_path]" % os.path.basename(sys.argv[0]))

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
    # try to use dumpbin (MSVC) or objdump (MinGW), otherwise ship all .dlls
    if which('dumpbin'):
        output = subprocess.check_output(['dumpbin', '/imports', fpath])
    elif which('objdump'):
        output = subprocess.check_output(['objdump', '-p', fpath])
    else:
        return debug_build
    return coredebug.search(output.decode(encoding)) != None

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

def copy_qt_libs(target_qt_prefix_path, qt_bin_dir, qt_libs_dir, qt_plugin_dir, qt_qml_dir, plugins):
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
                pass
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
        for binary in ['clang', 'clang-cl', 'clangd', 'clang-tidy', 'clazy-standalone']:
            binary_filepath = os.path.join(llvm_install_dir, 'bin', binary + '.exe')
            if os.path.exists(binary_filepath):
                deployinfo.append((binary_filepath, clangbindirtarget))
        resourcetarget = os.path.join(clanglibdirtarget, 'clang')
    else:
        # libclang -> Qt Creator libraries
        libsources = glob(os.path.join(llvm_install_dir, 'lib', 'libclang.so*'))
        for libsource in libsources:
            deployinfo.append((libsource, os.path.join(install_dir, 'lib', 'qtcreator')))
        # clang binaries -> clang libexec
        clangbinary_targetdir = os.path.join(install_dir, 'libexec', 'qtcreator', 'clang', 'bin')
        if not os.path.exists(clangbinary_targetdir):
            os.makedirs(clangbinary_targetdir)
        for binary in ['clang', 'clangd', 'clang-tidy', 'clazy-standalone']:
            binary_filepath = os.path.join(llvm_install_dir, 'bin', binary)
            if os.path.exists(binary_filepath):
                deployinfo.append((binary_filepath, clangbinary_targetdir))
                # add link target if binary is actually a symlink (to a binary in the same directory)
                if os.path.islink(binary_filepath):
                    linktarget = os.readlink(binary_filepath)
                    deployinfo.append((os.path.join(os.path.dirname(binary_filepath), linktarget),
                                       os.path.join(clangbinary_targetdir, linktarget)))
        clanglibs_targetdir = os.path.join(install_dir, 'libexec', 'qtcreator', 'clang', 'lib')
        # support libraries (for clazy) -> clang libexec
        if not os.path.exists(clanglibs_targetdir):
            os.makedirs(clanglibs_targetdir)
        # on RHEL ClazyPlugin is in lib64
        for lib_pattern in ['lib64/ClazyPlugin.so', 'lib/ClazyPlugin.so', 'lib/libclang-cpp.so*']:
            for lib in glob(os.path.join(llvm_install_dir, lib_pattern)):
                deployinfo.append((lib, clanglibs_targetdir))
        resourcetarget = os.path.join(install_dir, 'libexec', 'qtcreator', 'clang', 'lib', 'clang')

    print("copying libclang...")
    for source, target in deployinfo:
        print(source, '->', target)
        copyPreservingLinks(source, target)

    if common.is_linux_platform():
        # libclang was statically compiled, so there is no need for the RPATHs
        # and they are confusing when fixing RPATHs later in the process.
        # Also fix clazy-standalone RPATH.
        print("fixing Clang RPATHs...")
        for source, target in deployinfo:
            filename = os.path.basename(source)
            targetfilepath = target if not os.path.isdir(target) else os.path.join(target, filename)
            if filename == 'clazy-standalone':
                subprocess.check_call([chrpath_bin, '-r', '$ORIGIN/../lib', targetfilepath])
            elif not os.path.islink(target):
                targetfilepath = target if not os.path.isdir(target) else os.path.join(target, os.path.basename(source))
                subprocess.check_call([chrpath_bin, '-d', targetfilepath])

    print(resourcesource, '->', resourcetarget)
    if (os.path.exists(resourcetarget)):
        shutil.rmtree(resourcetarget)
    common.copytree(resourcesource, resourcetarget, symlinks=True)

def deploy_elfutils(qtc_install_dir, chrpath_bin, args):
    if common.is_mac_platform():
        return

    def lib_name(name, version):
        return ('lib' + name + '.so.' + version if common.is_linux_platform()
                else name + '.dll')

    version = '1'
    libs = ['elf', 'dw']
    elfutils_lib_path = os.path.join(args.elfutils_path, 'lib')
    if common.is_linux_platform():
        install_path = os.path.join(qtc_install_dir, 'lib', 'elfutils')
        backends_install_path = install_path
    elif common.is_windows_platform():
        install_path = os.path.join(qtc_install_dir, 'bin')
        backends_install_path = os.path.join(qtc_install_dir, 'lib', 'elfutils')
        libs.append('eu_compat')
    if not os.path.exists(install_path):
        os.makedirs(install_path)
    if not os.path.exists(backends_install_path):
        os.makedirs(backends_install_path)
    # copy main libs
    libs = [os.path.join(elfutils_lib_path, lib_name(lib, version)) for lib in libs]
    for lib in libs:
        print(lib, '->', install_path)
        shutil.copy(lib, install_path)
    # fix rpath
    if common.is_linux_platform():
        relative_path = os.path.relpath(backends_install_path, install_path)
        subprocess.check_call([chrpath_bin, '-r', os.path.join('$ORIGIN', relative_path),
                               os.path.join(install_path, lib_name('dw', version))])
    # copy backend files
    # only non-versioned, we never dlopen the versioned ones
    files = glob(os.path.join(elfutils_lib_path, 'elfutils', '*ebl_*.*'))
    versioned_files = glob(os.path.join(elfutils_lib_path, 'elfutils', '*ebl_*.*-*.*.*'))
    unversioned_files = [file for file in files if file not in versioned_files]
    for file in unversioned_files:
        print(file, '->', backends_install_path)
        shutil.copy(file, backends_install_path)

def deploy_mac(args):
    (_, qt_install) = get_qt_install_info(args.qmake_binary)

    env = dict(os.environ)
    if args.llvm_path:
        env['LLVM_INSTALL_DIR'] = args.llvm_path

    script_path = os.path.dirname(os.path.realpath(__file__))
    deployqtHelper_mac = os.path.join(script_path, 'deployqtHelper_mac.sh')
    common.check_print_call([deployqtHelper_mac, args.qtcreator_binary, qt_install.bin,
                             qt_install.translations, qt_install.plugins, qt_install.qml],
                            env=env)

def get_qt_install_info(qmake_binary):
    qt_install_info = common.get_qt_install_info(qmake_binary)
    QtInstallInfo = collections.namedtuple('QtInstallInfo', ['bin', 'lib', 'plugins',
                                                             'qml', 'translations'])
    return (qt_install_info,
            QtInstallInfo(bin=qt_install_info['QT_INSTALL_BINS'],
                          lib=qt_install_info['QT_INSTALL_LIBS'],
                          plugins=qt_install_info['QT_INSTALL_PLUGINS'],
                          qml=qt_install_info['QT_INSTALL_QML'],
                          translations=qt_install_info['QT_INSTALL_TRANSLATIONS']))

def main():
    args = get_args()
    if common.is_mac_platform():
        deploy_mac(args)
        return

    (qt_install_info, qt_install) = get_qt_install_info(args.qmake_binary)

    qtcreator_binary_path = os.path.dirname(args.qtcreator_binary)
    install_dir = os.path.abspath(os.path.join(qtcreator_binary_path, '..'))
    if common.is_linux_platform():
        qt_deploy_prefix = os.path.join(install_dir, 'lib', 'Qt')
    else:
        qt_deploy_prefix = os.path.join(install_dir, 'bin')

    chrpath_bin = None
    if common.is_linux_platform():
        chrpath_bin = which('chrpath')
        if chrpath_bin == None:
            print("Cannot find required binary 'chrpath'.")
            sys.exit(2)

    plugins = ['assetimporters', 'accessible', 'codecs', 'designer', 'iconengines', 'imageformats', 'platformthemes',
               'platforminputcontexts', 'platforms', 'printsupport', 'qmltooling', 'sqldrivers', 'styles',
               'xcbglintegrations',
               'wayland-decoration-client',
               'wayland-graphics-integration-client',
               'wayland-shell-integration',
               ]

    if common.is_windows_platform():
        global debug_build
        debug_build = is_debug(args.qtcreator_binary)

    if common.is_windows_platform():
        copy_qt_libs(qt_deploy_prefix, qt_install.bin, qt_install.bin, qt_install.plugins, qt_install.qml, plugins)
    else:
        copy_qt_libs(qt_deploy_prefix, qt_install.bin, qt_install.lib, qt_install.plugins, qt_install.qml, plugins)
    copy_translations(install_dir, qt_install.translations)
    if args.llvm_path:
        deploy_libclang(install_dir, args.llvm_path, chrpath_bin)

    if args.elfutils_path:
        deploy_elfutils(install_dir, chrpath_bin, args)
    if not common.is_windows_platform():
        print("fixing rpaths...")
        common.fix_rpaths(install_dir, os.path.join(qt_deploy_prefix, 'lib'), qt_install_info, chrpath_bin)
        add_qt_conf(os.path.join(install_dir, 'libexec', 'qtcreator'), qt_deploy_prefix) # e.g. for qml2puppet
        add_qt_conf(os.path.join(qt_deploy_prefix, 'bin'), qt_deploy_prefix) # e.g. qtdiag
    add_qt_conf(os.path.join(install_dir, 'bin'), qt_deploy_prefix)

if __name__ == "__main__":
    main()
