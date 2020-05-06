#!/usr/bin/env python
#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the release tools of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

# import the print function which is used in python 3.x
from __future__ import print_function

import argparse
import collections
import glob
import os

import common

def existing_path(path):
    return path if os.path.exists(path) else None

def default_python3():
    path_system = os.path.join('/usr', 'bin') if not common.is_windows_platform() else None
    path = os.environ.get('PYTHON3_PATH') or path_system
    postfix = '.exe' if common.is_windows_platform() else ''
    return (path if not path
            else (existing_path(os.path.join(path, 'python3' + postfix)) or
                  existing_path(os.path.join(path, 'python' + postfix))))

def get_arguments():
    parser = argparse.ArgumentParser(description='Build Qt Creator for packaging')
    parser.add_argument('--src', help='path to sources', required=True)
    parser.add_argument('--build', help='path that should be used for building', required=True)
    parser.add_argument('--qt-path', help='Path to Qt', required=True)

    parser.add_argument('--debug', help='Enable debug builds', action='store_true', default=False)

    # clang codemodel
    parser.add_argument('--llvm-path', help='Path to LLVM installation for Clang code model',
                        default=os.environ.get('LLVM_INSTALL_DIR'))

    # perfparser
    parser.add_argument('--elfutils-path',
                        help='Path to elfutils installation for use by perfprofiler (Windows, Linux)')

    # signing
    parser.add_argument('--keychain-unlock-script',
                        help='Path to script for unlocking the keychain used for signing (macOS)')

    # cdbextension
    parser.add_argument('--python-path',
                        help='Path to python libraries for use by cdbextension (Windows)')

    parser.add_argument('--app-target', help='File name of the executable / app bundle',
                        default=('Qt Creator.app' if common.is_mac_platform()
                                 else 'qtcreator'))
    parser.add_argument('--python3', help='File path to python3 executable for generating translations',
                        default=default_python3())

    parser.add_argument('--no-cdb',
                        help='Skip cdbextension and the python dependency packaging step (Windows)',
                        action='store_true', default=(not common.is_windows_platform()))
    parser.add_argument('--no-docs', help='Skip documentation generation',
                        action='store_true', default=False)
    parser.add_argument('--no-dmg', help='Skip disk image creation (macOS)',
                        action='store_true', default=False)
    parser.add_argument('--no-zip', help='Skip creation of 7zip files for install and developer package',
                        action='store_true', default=False)
    return parser.parse_args()

def build_qtcreator(args, paths):
    if not os.path.exists(paths.build):
        os.makedirs(paths.build)
    prefix_paths = [paths.qt]
    if args.llvm_path:
        prefix_paths += [args.llvm_path]
    if args.elfutils_path:
        prefix_paths += [args.elfutils_path]
    build_type = 'Debug' if args.debug else 'Release'
    with_docs_str = 'OFF' if args.no_docs else 'ON'
    cmake_args = ['cmake',
                  '-DCMAKE_PREFIX_PATH=' + ';'.join(prefix_paths),
                  '-DCMAKE_BUILD_TYPE=' + build_type,
                  '-DWITH_DOCS=' + with_docs_str,
                  '-DBUILD_DEVELOPER_DOCS=' + with_docs_str,
                  '-DBUILD_EXECUTABLE_SDKTOOL=OFF',
                  '-DCMAKE_INSTALL_PREFIX=' + paths.install,
                  '-DWITH_TESTS=OFF',
                  '-G', 'Ninja']

    if args.python3:
        cmake_args += ['-DPYTHON_EXECUTABLE=' + args.python3]

    # force MSVC on Windows, because it looks for GCC in the PATH first,
    # even if MSVC is first mentioned in the PATH...
    # TODO would be nicer if we only did this if cl.exe is indeed first in the PATH
    if common.is_windows_platform():
        cmake_args += ['-DCMAKE_C_COMPILER=cl',
                       '-DCMAKE_CXX_COMPILER=cl',
                       '-DBUILD_EXECUTABLE_WIN32INTERRUPT=OFF',
                       '-DBUILD_EXECUTABLE_WIN64INTERRUPT=OFF',
                       '-DBUILD_LIBRARY_QTCREATORCDBEXT=OFF']
        if args.python_path:
            python_library = glob.glob(os.path.join(args.python_path, 'libs', 'python??.lib'))
            if python_library:
                cmake_args += ['-DPYTHON_LIBRARY=' + python_library[0],
                               '-DPYTHON_INCLUDE_DIR=' + os.path.join(args.python_path, 'include')]

    # TODO this works around a CMake bug https://gitlab.kitware.com/cmake/cmake/issues/20119
    cmake_args += ['-DBUILD_WITH_PCH=OFF']

    ide_revision = common.get_commit_SHA(paths.src)
    if ide_revision:
        cmake_args += ['-DIDE_REVISION=ON',
                       '-DIDE_REVISION_STR=' + ide_revision,
                       '-DIDE_REVISION_URL=https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=' + ide_revision]

    common.check_print_call(cmake_args + [paths.src], paths.build)
    common.check_print_call(['cmake', '--build', '.'], paths.build)
    if not args.no_docs:
        common.check_print_call(['cmake', '--build', '.', '--target', 'docs'], paths.build)

    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install, '--strip'],
                            paths.build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.dev_install,
                             '--component', 'Devel'],
                            paths.build)
    if not args.no_docs:
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'qtc_docs_qtcreator'],
                                paths.build)
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'html_docs_qtcreator'],
                                paths.build)
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'html_docs_qtcreator-dev'],
                                paths.build)
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'html_docs_qtcreator-dev'],
                                paths.build)
def build_wininterrupt(args, paths):
    if not common.is_windows_platform():
        return
    # assumes existing Qt Creator build
    cmake_args = ['-DBUILD_EXECUTABLE_WIN32INTERRUPT=ON',
                  '-DBUILD_EXECUTABLE_WIN64INTERRUPT=ON',
                  '-DBUILD_LIBRARY_QTCREATORCDBEXT=OFF']
    common.check_print_call(['cmake'] + cmake_args + [paths.src], paths.build)
    common.check_print_call(['cmake', '--build', '.'], paths.build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.wininterrupt_install,
                             '--component', 'wininterrupt'],
                            paths.build)

def build_qtcreatorcdbext(args, paths):
    if args.no_cdb:
        return
    # assumes existing Qt Creator build
    cmake_args = ['-DBUILD_EXECUTABLE_WIN32INTERRUPT=OFF',
                  '-DBUILD_EXECUTABLE_WIN64INTERRUPT=OFF',
                  '-DBUILD_LIBRARY_QTCREATORCDBEXT=ON']
    common.check_print_call(['cmake'] + cmake_args + [paths.src], paths.build)
    common.check_print_call(['cmake', '--build', '.'], paths.build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.qtcreatorcdbext_install,
                             '--component', 'qtcreatorcdbext'],
                            paths.build)

def deploy_qt(args, paths):
    if common.is_mac_platform():
        script = os.path.join(paths.src, 'scripts', 'deployqtHelper_mac.sh')
        app = os.path.join(paths.install, args.app_target)
        # TODO this is wrong if Qt is set up non-standard
        # TODO integrate deployqtHelper_mac.sh into deployqt.py, finally
        qt_bins = os.path.join(paths.qt, 'bin')
        qt_translations = os.path.join(paths.qt, 'translations')
        qt_plugins = os.path.join(paths.qt, 'plugins')
        qt_imports = os.path.join(paths.qt, 'imports')
        qt_qml = os.path.join(paths.qt, 'qml')
        common.check_print_call([script, app, qt_bins, qt_translations, qt_plugins,
                                 qt_imports, qt_qml],
                                paths.build)
    else:
        exe = os.path.join(paths.install, 'bin', args.app_target)
        common.check_print_call(['python', '-u', os.path.join(paths.src, 'scripts', 'deployqt.py'),
                                 '-i', exe, os.path.join(paths.qt, 'bin', 'qmake')],
                                paths.build)

def package_qtcreator(args, paths):
    if not args.no_zip:
        common.check_print_call(['7z', 'a', '-mmt2', os.path.join(paths.result, 'qtcreator.7z'), '*'],
                                paths.install)
        common.check_print_call(['7z', 'a', '-mmt2',
                                 os.path.join(paths.result, 'qtcreator_dev.7z'), '*'],
                                paths.dev_install)
        if common.is_windows_platform():
            common.check_print_call(['7z', 'a', '-mmt2',
                                     os.path.join(paths.result, 'wininterrupt.7z'), '*'],
                                    paths.wininterrupt_install)
            if not args.no_cdb:
                common.check_print_call(['7z', 'a', '-mmt2',
                                         os.path.join(paths.result, 'qtcreatorcdbext.7z'), '*'],
                                        paths.qtcreatorcdbext_install)

    if common.is_mac_platform():
        if args.keychain_unlock_script:
            common.check_print_call([args.keychain_unlock_script], paths.install)
        if not args.no_dmg:
            common.check_print_call(['python', '-u',
                                     os.path.join(paths.src, 'scripts', 'makedmg.py'),
                                     'qt-creator.dmg',
                                     'Qt Creator',
                                     paths.src,
                                     paths.install],
                                    paths.result)

def get_paths(args):
    Paths = collections.namedtuple('Paths',
                                   ['qt', 'src', 'build',
                                    'install', 'dev_install', 'wininterrupt_install',
                                    'qtcreatorcdbext_install', 'result'])
    build_path = os.path.abspath(args.build)
    install_path = os.path.join(build_path, 'install')
    return Paths(qt=os.path.abspath(args.qt_path),
                 src=os.path.abspath(args.src),
                 build=os.path.join(build_path, 'build'),
                 install=os.path.join(install_path, 'qt-creator'),
                 dev_install=os.path.join(install_path, 'qt-creator-dev'),
                 wininterrupt_install=os.path.join(install_path, 'wininterrupt'),
                 qtcreatorcdbext_install=os.path.join(install_path, 'qtcreatorcdbext'),
                 result=build_path)

def main():
    args = get_arguments()
    paths = get_paths(args)

    build_qtcreator(args, paths)
    build_wininterrupt(args, paths)
    build_qtcreatorcdbext(args, paths)
    deploy_qt(args, paths)
    package_qtcreator(args, paths)

if __name__ == '__main__':
    main()
