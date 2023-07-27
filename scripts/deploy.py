#!/usr/bin/env python3
# Copyright (C) The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    parser.add_argument('--elfutils-path',
                        help='Path to elfutils installation for use by perfprofiler (Windows, Linux)')
    parser.add_argument('--llvm-path',
                        help='Path to LLVM installation')
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

def with_exe_ext(filepath):
    return filepath + '.exe' if common.is_windows_platform() else filepath

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
    # Qt ships some unneeded object files in the qml plugins
    # On Windows we also do not want to ship the wrong debug/release .dlls or .lib files etc
    # And get rid of debug info directories (.dSYM) on macOS
    if common.is_linux_platform():
        return [fn for fn in filenames if fn.endswith('.cpp.o')]
    if common.is_mac_platform():
        return [fn for fn in filenames if fn.endswith('.dylib.dSYM') or fn.startswith('objects-')]
    return [fn for fn in filenames
            if fn.endswith('.cpp.obj') or is_ignored_windows_file(debug_build, path, fn)]

def copy_qt_libs(target_qt_prefix_path, qt_bin_dir, qt_libs_dir):
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


def deploy_qtdiag(qtc_binary_path, qt_install):
    print("Copying qtdiag")
    qtdiag_src = os.path.join(qt_install.bin, with_exe_ext('qtdiag'))
    destdir = (qtc_binary_path if common.is_windows_platform()
              else os.path.join(qtc_binary_path, 'Contents', 'MacOS') if common.is_mac_platform()
              else os.path.join(qtc_binary_path, '..', 'lib', 'Qt', 'bin'))
    if not os.path.exists(destdir):
        os.makedirs(destdir)
    shutil.copy(qtdiag_src, destdir)
    if common.is_mac_platform():
        # fix RPATHs
        qtdiag_dest = os.path.join(destdir, 'qtdiag')
        subprocess.check_call(['xcrun', 'install_name_tool', '-add_rpath', '@loader_path/../Frameworks', qtdiag_dest])
        subprocess.check_call(['xcrun', 'install_name_tool', '-delete_rpath', '@loader_path/../lib', qtdiag_dest])


def deploy_plugins(qtc_binary_path, qt_install):
    plugins = ['assetimporters', 'accessible', 'codecs', 'designer', 'iconengines', 'imageformats', 'platformthemes',
               'platforminputcontexts', 'platforms', 'printsupport', 'qmltooling', 'sqldrivers', 'styles',
               'xcbglintegrations',
               'wayland-decoration-client',
               'wayland-graphics-integration-client',
               'wayland-shell-integration',
               'tls'
               ]
    print("Copying plugins:", plugins)
    destdir = (os.path.join(qtc_binary_path, 'plugins') if common.is_windows_platform()
               else os.path.join(qtc_binary_path, 'Contents', 'PlugIns') if common.is_mac_platform()
               else os.path.join(qtc_binary_path, '..', 'lib', 'Qt', 'plugins'))
    for plugin in plugins:
        target = os.path.join(destdir, plugin)
        if (os.path.exists(target)):
            shutil.rmtree(target)
        pluginPath = os.path.join(qt_install.plugins, plugin)
        if (os.path.exists(pluginPath)):
            print('{0} -> {1}'.format(pluginPath, target))
            common.copytree(pluginPath, target, ignore=ignored_qt_lib_files, symlinks=True)


def deploy_imports(qtc_binary_path, qt_install):
    print("Copying qt quick 2 imports")
    destdir = (os.path.join(qtc_binary_path, 'qml') if common.is_windows_platform()
               else os.path.join(qtc_binary_path, 'Contents', 'Imports', 'qtquick2') if common.is_mac_platform()
               else os.path.join(qtc_binary_path, '..', 'lib', 'Qt', 'qml'))
    if (os.path.exists(destdir)):
        shutil.rmtree(destdir)
    print('{0} -> {1}'.format(qt_install.qml, destdir))
    common.copytree(qt_install.qml, destdir, ignore=ignored_qt_lib_files, symlinks=True)


def qt_conf_contents():
    if common.is_linux_platform():
        return '''[Paths]
Prefix={0}
Binaries=bin
Libraries=lib
Plugins=plugins
Qml2Imports=qml
'''
    if common.is_windows_platform():
        return '''[Paths]
Prefix={0}
Binaries=.
Libraries=.
Plugins=plugins
Qml2Imports=qml
'''
    return '''[Paths]
Prefix={0}
Binaries=MacOS
Libraries=Frameworks
Plugins=PlugIns
Qml2Imports=Imports/qtquick2
'''


def add_qt_conf(target_path, qt_prefix_path):
    qtconf_filepath = os.path.join(target_path, 'qt.conf')
    prefix_path = os.path.relpath(qt_prefix_path, target_path).replace('\\', '/')
    print('Creating qt.conf in "{0}":'.format(qtconf_filepath))
    f = open(qtconf_filepath, 'w')
    f.write(qt_conf_contents().format(prefix_path))
    f.close()


def deploy_qt_conf_files(qtc_binary_path):
    if common.is_linux_platform():
        qt_prefix_path = os.path.join(qtc_binary_path, '..', 'lib', 'Qt')
        add_qt_conf(os.path.join(qtc_binary_path, '..', 'libexec', 'qtcreator'), qt_prefix_path)
        add_qt_conf(os.path.join(qtc_binary_path, '..', 'lib', 'Qt', 'bin'), qt_prefix_path) # qtdiag
        add_qt_conf(qtc_binary_path, qt_prefix_path) # QtC itself
    if common.is_windows_platform():
        add_qt_conf(qtc_binary_path, qtc_binary_path) # QtC itself, libexec, and qtdiag etc
    if common.is_mac_platform():
        qt_prefix_path = os.path.join(qtc_binary_path, 'Contents')
        qtc_resources_path = os.path.join(qtc_binary_path, 'Contents', 'Resources')
        add_qt_conf(os.path.join(qtc_resources_path, 'libexec'), qt_prefix_path)
        add_qt_conf(os.path.join(qtc_resources_path, 'libexec', 'ios'), qt_prefix_path)
        # The Prefix path for a qt.conf in Contents/Resources/ is handled specially/funny by Qt,
        # relative paths are resolved relative to Contents/, so we must enforces Prefix=.
        add_qt_conf(qtc_resources_path, qtc_resources_path) # QtC itself


def deploy_translations(qtc_binary_path, qt_install):
    print("Copying translations...")
    translations = glob(os.path.join(qt_install.translations, '*.qm'))
    destdir = (os.path.join(qtc_binary_path, 'Contents', 'Resources', 'translations') if common.is_mac_platform()
               else os.path.join(qtc_binary_path, '..', 'share', 'qtcreator', 'translations'))
    for translation in translations:
        print(translation, '->', destdir)
        shutil.copy(translation, destdir)

def copyPreservingLinks(source, destination):
    if os.path.islink(source):
        linkto = os.readlink(source)
        destFilePath = destination
        if os.path.isdir(destination):
            destFilePath = os.path.join(destination, os.path.basename(source))
        os.symlink(linkto, destFilePath)
    else:
        shutil.copy(source, destination)

def deploy_clang(qtc_binary_path, llvm_install_dir, chrpath_bin):
    print("Copying clang...")

    # contains pairs of (source, target directory)
    deployinfo = []
    resourcesource = os.path.join(llvm_install_dir, 'lib', 'clang')
    if common.is_windows_platform():
        clang_targetdir = os.path.join(qtc_binary_path, 'clang')
        clangbinary_targetdir = os.path.join(clang_targetdir, 'bin')
        resourcetarget = os.path.join(clang_targetdir, 'lib', 'clang')
    elif common.is_linux_platform():
        clang_targetdir = os.path.join(qtc_binary_path, '..', 'libexec', 'qtcreator', 'clang')
        clangbinary_targetdir = os.path.join(clang_targetdir, 'bin')
        resourcetarget = os.path.join(clang_targetdir, 'lib', 'clang')
        # ClazyPlugin. On RHEL ClazyPlugin is in lib64, so check both
        clanglibs_targetdir = os.path.join(clang_targetdir, 'lib')
        if not os.path.exists(clanglibs_targetdir):
            os.makedirs(clanglibs_targetdir)
        for lib_pattern in ['lib64/ClazyPlugin.so', 'lib/ClazyPlugin.so']:
            for lib in glob(os.path.join(llvm_install_dir, lib_pattern)):
                deployinfo.append((lib, clanglibs_targetdir))
    else:
        clang_targetdir = os.path.join(qtc_binary_path, 'Contents', 'Resources', 'libexec', 'clang')
        clanglibs_targetdir = os.path.join(clang_targetdir, 'lib')
        clangbinary_targetdir = os.path.join(clang_targetdir, 'bin')
        resourcetarget = os.path.join(clang_targetdir, 'lib', 'clang')
        # ClazyPlugin
        clanglibs_targetdir = os.path.join(clang_targetdir, 'lib')
        if not os.path.exists(clanglibs_targetdir):
            os.makedirs(clanglibs_targetdir)
        clazy_plugin = os.path.join(llvm_install_dir, 'lib', 'ClazyPlugin.dylib')
        deployinfo.append((clazy_plugin, clanglibs_targetdir))

    # collect binaries
    if not os.path.exists(clangbinary_targetdir):
        os.makedirs(clangbinary_targetdir)
    for binary in ['clangd', 'clang-tidy', 'clazy-standalone', 'clang-format']:
        binary_filepath = os.path.join(llvm_install_dir, 'bin', with_exe_ext(binary))
        if os.path.exists(binary_filepath):
            deployinfo.append((binary_filepath, clangbinary_targetdir))
            # add link target if binary is actually a symlink (to a binary in the same directory)
            if not common.is_windows_platform() and os.path.islink(binary_filepath):
                linktarget = os.readlink(binary_filepath)
                deployinfo.append((os.path.join(os.path.dirname(binary_filepath), linktarget),
                                   os.path.join(clangbinary_targetdir, linktarget)))

    for source, target in deployinfo:
        print(source, '->', target)
        copyPreservingLinks(source, target)

    if common.is_linux_platform():
        # libclang was statically compiled, so there is no need for the RPATHs
        # and they are confusing when fixing RPATHs later in the process.
        # Also fix clazy-standalone RPATH.
        for source, target in deployinfo:
            filename = os.path.basename(source)
            targetfilepath = target if not os.path.isdir(target) else os.path.join(target, filename)
            if filename == 'clazy-standalone':
                subprocess.check_call([chrpath_bin, '-r', '$ORIGIN/../lib', targetfilepath])
            elif not os.path.islink(target):
                targetfilepath = target if not os.path.isdir(target) else os.path.join(target, os.path.basename(source))
                subprocess.check_call([chrpath_bin, '-d', targetfilepath])

    if common.is_mac_platform():
        # fix RPATH of clazy-standalone
        clazy_dest = os.path.join(clangbinary_targetdir, 'clazy-standalone')
        subprocess.call(['xcrun', 'install_name_tool', '-add_rpath', '@loader_path/../lib', clazy_dest], stderr=subprocess.DEVNULL)
        subprocess.call(['xcrun', 'install_name_tool', '-delete_rpath', '/Users/qt/work/build/libclang/lib', clazy_dest], stderr=subprocess.DEVNULL)

    print(resourcesource, '->', resourcetarget)
    if (os.path.exists(resourcetarget)):
        shutil.rmtree(resourcetarget)
    common.copytree(resourcesource, resourcetarget, symlinks=True)

def deploy_elfutils(qtc_install_dir, chrpath_bin, args):
    if common.is_mac_platform():
        return

    libs = ['elf', 'dw']
    version = '1'

    def lib_name(name, version):
        return ('lib' + name + '.so.' + version if common.is_linux_platform()
                else name + '.dll')

    def find_elfutils_lib_path(path):
        for root, _, files in os.walk(path):
            if lib_name('elf', version) in files:
                return root
        return path

    elfutils_lib_path = find_elfutils_lib_path(os.path.join(args.elfutils_path, 'lib'))
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

def deploy_qt_mac(qtc_binary_path, qt_install):
    # This runs macdeployqt
    # Collect things to pass via -executable
    libexec_path = os.path.join(qtc_binary_path, 'Contents', 'Resources', 'libexec')
    bin_path = os.path.join(qtc_binary_path, 'Contents', 'MacOS')
    plugins_path = os.path.join(qtc_binary_path, 'Contents', 'PlugIns')
    frameworks_path = os.path.join(qtc_binary_path, 'Contents', 'Frameworks')
    additional_paths = []
    # Qbs
    apps = ['qbs', 'qbs-config', 'qbs-config-ui', 'qbs-setup-android', 'qbs-setup-qt',
            'qbs-setup-toolchains', 'qbs-create-project']
    for app in apps:
        additional_paths.append(os.path.join(bin_path, app))
    additional_paths.append(os.path.join(libexec_path, 'qbs_processlauncher'))
    # qml2puppet
    puppets = glob(os.path.join(libexec_path, 'qml2puppet*'))
    for puppet in puppets:
        additional_paths.append(puppet)
    # qtdiag
    additional_paths.append(os.path.join(bin_path, 'qtdiag'))
    # other libexec
    additional_paths.append(os.path.join(libexec_path, 'sdktool'))
    additional_paths.append(os.path.join(libexec_path, 'qtpromaker'))
    additional_paths.append(os.path.join(libexec_path, 'buildoutputparser'))
    additional_paths.append(os.path.join(libexec_path, 'cpaster'))
    additional_paths.append(os.path.join(libexec_path, 'ios', 'iostool'))

    existing_additional_paths = [p for p in additional_paths if os.path.exists(p)]
    macdeployqt = os.path.join(qt_install.bin, 'macdeployqt')
    print('Running macdeployqt (' + macdeployqt + ')')
    print('- with additional paths:', existing_additional_paths)
    executable_args = ['-executable='+path for path in existing_additional_paths]
    subprocess.check_call([macdeployqt, qtc_binary_path] + executable_args)

    # clean up some things that might have been deployed, but we don't want
    to_remove = [
        os.path.join(plugins_path, 'tls', 'libqopensslbackend.dylib'),
        os.path.join(plugins_path, 'sqldrivers', 'libqsqlpsql.dylib'),
        os.path.join(plugins_path, 'sqldrivers', 'libqsqlodbc.dylib'),
    ]
    to_remove.extend(glob(os.path.join(frameworks_path, 'libpq.*dylib')))
    to_remove.extend(glob(os.path.join(frameworks_path, 'libssl.*dylib')))
    to_remove.extend(glob(os.path.join(frameworks_path, 'libcrypto.*dylib')))
    for path in to_remove:
        if os.path.isfile(path):
            print('- Removing ' + path)
            os.remove(path)


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
    chrpath_bin = None
    if common.is_linux_platform():
        chrpath_bin = which('chrpath')
        if chrpath_bin == None:
            print("Cannot find required binary 'chrpath'.")
            sys.exit(2)

    if common.is_windows_platform():
        global debug_build
        debug_build = is_debug(args.qtcreator_binary)

    (qt_install_info, qt_install) = get_qt_install_info(args.qmake_binary)
    # <qtc>/bin for Win/Lin, <path>/<appname>.app for macOS
    qtcreator_binary_path = (args.qtcreator_binary if common.is_mac_platform()
                             else os.path.dirname(args.qtcreator_binary))

    deploy_qtdiag(qtcreator_binary_path, qt_install)
    deploy_plugins(qtcreator_binary_path, qt_install)
    deploy_imports(qtcreator_binary_path, qt_install)
    deploy_translations(qtcreator_binary_path, qt_install)
    deploy_qt_conf_files(qtcreator_binary_path)
    if args.llvm_path:
        deploy_clang(qtcreator_binary_path, args.llvm_path, chrpath_bin)

    if common.is_mac_platform():
        deploy_qt_mac(qtcreator_binary_path, qt_install)
    else:
        install_dir = os.path.abspath(os.path.join(qtcreator_binary_path, '..'))
        if common.is_linux_platform():
            qt_deploy_prefix = os.path.join(install_dir, 'lib', 'Qt')
        else:
            qt_deploy_prefix = os.path.join(install_dir, 'bin')

        if common.is_windows_platform():
            copy_qt_libs(qt_deploy_prefix, qt_install.bin, qt_install.bin)
        else:
            copy_qt_libs(qt_deploy_prefix, qt_install.bin, qt_install.lib)

        if args.elfutils_path:
            deploy_elfutils(install_dir, chrpath_bin, args)
        if not common.is_windows_platform():
            print("fixing rpaths...")
            common.fix_rpaths(install_dir, os.path.join(qt_deploy_prefix, 'lib'), qt_install_info, chrpath_bin)


if __name__ == "__main__":
    main()
