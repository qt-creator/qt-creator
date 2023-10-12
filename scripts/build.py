#!/usr/bin/env python3
# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# import the print function which is used in python 3.x
from __future__ import print_function

import argparse
import collections
import os
import shlex
import shutil

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
    parser.add_argument('--qt-path', help='Path to Qt')

    parser.add_argument('--build-type', help='Build type to pass to CMake (defaults to RelWithDebInfo)',
                        default='RelWithDebInfo')

    # clang codemodel
    parser.add_argument('--llvm-path', help='Path to LLVM installation for Clang code model',
                        default=os.environ.get('LLVM_INSTALL_DIR'))

    # perfparser
    parser.add_argument('--elfutils-path',
                        help='Path to elfutils installation for use by perfprofiler (Windows, Linux)')

    # signing
    parser.add_argument('--keychain-unlock-script',
                        help='Path to script for unlocking the keychain used for signing (macOS)')
    parser.add_argument('--sign-command',
                        help='Command to use for signing (Windows). The installation directory to sign is added at the end. Is run in the CWD.')

    # cdbextension
    parser.add_argument('--python-path',
                        help='Path to python libraries for use by cdbextension (Windows)')

    parser.add_argument('--python3', help='File path to python3 executable for generating translations',
                        default=default_python3())

    parser.add_argument('--no-qtcreator',
                        help='Skip Qt Creator build (only build separate tools)',
                        action='store_true', default=False)
    parser.add_argument('--no-cdb',
                        help='Skip cdbextension and the python dependency packaging step (Windows)',
                        action='store_true', default=(not common.is_windows_platform()))
    parser.add_argument('--no-qbs', help='Skip building Qbs as part of Qt Creator', action='store_true', default=False);
    parser.add_argument('--no-docs', help='Skip documentation generation',
                        action='store_true', default=False)
    parser.add_argument('--no-build-date', help='Does not show build date in about dialog, for reproducible builds',
                        action='store_true', default=False)
    parser.add_argument('--no-dmg', help='Skip disk image creation (macOS)',
                        action='store_true', default=False)
    parser.add_argument('--no-zip', help='Skip creation of 7zip files for install and developer package',
                        action='store_true', default=False)
    parser.add_argument('--with-tests', help='Enable building of tests',
                        action='store_true', default=False)
    parser.add_argument('--with-pch', help='Enable building with PCH',
                        action='store_true', default=False)
    parser.add_argument('--with-cpack', help='Create packages with cpack',
                        action='store_true', default=False)
    parser.add_argument('--add-path', help='Prepends a CMAKE_PREFIX_PATH to the build',
                        action='append', dest='prefix_paths', default=[])
    parser.add_argument('--add-module-path', help='Prepends a CMAKE_MODULE_PATH to the build',
                        action='append', dest='module_paths', default=[])
    parser.add_argument('--add-make-arg', help='Passes the argument to the make tool.',
                        action='append', dest='make_args', default=[])
    parser.add_argument('--add-config', help=('Adds the argument to the CMake configuration call. '
                                              'Use "--add-config=-DSOMEVAR=SOMEVALUE" if the argument begins with a dash.'),
                        action='append', dest='config_args', default=[])
    parser.add_argument('--zip-infix', help='Adds an infix to generated zip files, use e.g. for a build number.',
                        default='')
    parser.add_argument('--zip-threads', help='Sets number of threads to use for 7z. Use "+" for turning threads on '
                        'without a specific number of threads. This is directly passed to the "-mmt" option of 7z.',
                        default='2')
    parser.add_argument('--add-sanitize-flags', help="Sets flags for sanitizer compilation flags used in Debug builds",
                        action='append', dest='sanitize_flags', default=[] )

    args = parser.parse_args()
    args.with_debug_info = args.build_type == 'RelWithDebInfo'

    if not args.qt_path and not args.no_qtcreator:
        parser.error("argument --qt-path is required if --no-qtcreator is not given")

    if args.with_cpack:
        if common.is_mac_platform():
            print('warning: --with-cpack is not supported on macOS, turning off')
            args.with_cpack = False
        elif common.is_linux_platform():
            args.cpack_generators = ['DEB']
        elif common.is_windows_platform():
            args.cpack_generators = []
            if shutil.which('makensis'):
                args.cpack_generators += ['NSIS64']
            if shutil.which('candle') and shutil.which('torch'):
                args.cpack_generators += ['WIX']
            else:
                print('warning: could not find NSIS or WIX, turning cpack off')
                args.with_cpack = False

    return args

def common_cmake_arguments(args):
    separate_debug_info_option = 'ON' if args.with_debug_info else 'OFF'
    cmake_args = ['-DCMAKE_BUILD_TYPE=' + args.build_type,
                  '-DQTC_SEPARATE_DEBUG_INFO=' + separate_debug_info_option,
                  '-G', 'Ninja']

    if args.python3:
        cmake_args += ['-DPython3_EXECUTABLE=' + args.python3]
    if args.python_path:
        cmake_args += ['-DPython3_ROOT_DIR=' + args.python_path]

    if args.module_paths:
        module_paths = [common.to_posix_path(os.path.abspath(fp)) for fp in args.module_paths]
        cmake_args += ['-DCMAKE_MODULE_PATH=' + ';'.join(module_paths)]

    # force MSVC on Windows, because it looks for GCC in the PATH first,
    # even if MSVC is first mentioned in the PATH...
    # TODO would be nicer if we only did this if cl.exe is indeed first in the PATH
    if common.is_windows_platform():
        if not os.environ.get('CC') and not os.environ.get('CXX'):
            cmake_args += ['-DCMAKE_C_COMPILER=cl',
                           '-DCMAKE_CXX_COMPILER=cl']

    pch_option = 'ON' if args.with_pch else 'OFF'
    cmake_args += ['-DBUILD_WITH_PCH=' + pch_option]

    # work around QTBUG-89754
    # Qt otherwise adds dependencies on libGLX and libOpenGL
    cmake_args += ['-DOpenGL_GL_PREFERENCE=LEGACY']

    cmake_args += args.config_args

    return cmake_args

def build_qtcreator(args, paths):
    def cmake_option(option):
        return 'ON' if option else 'OFF'
    if args.no_qtcreator:
        return
    if not os.path.exists(paths.build):
        os.makedirs(paths.build)
    build_qbs = (True if not args.no_qbs and os.path.exists(os.path.join(paths.src, 'src', 'shared', 'qbs', 'CMakeLists.txt'))
                 else False)
    prefix_paths = [os.path.abspath(fp) for fp in args.prefix_paths] + [paths.qt]
    if paths.llvm:
        prefix_paths += [paths.llvm]
    if paths.elfutils:
        prefix_paths += [paths.elfutils]
    prefix_paths = [common.to_posix_path(fp) for fp in prefix_paths]
    cmake_args = ['cmake',
                  '-DCMAKE_PREFIX_PATH=' + ';'.join(prefix_paths),
                  '-DSHOW_BUILD_DATE=' + cmake_option(not args.no_build_date),
                  '-DWITH_DOCS=' + cmake_option(not args.no_docs),
                  '-DBUILD_QBS=' + cmake_option(build_qbs),
                  '-DBUILD_DEVELOPER_DOCS=' + cmake_option(not args.no_docs),
                  '-DBUILD_EXECUTABLE_SDKTOOL=OFF',
                  '-DQTC_FORCE_XCB=ON',
                  '-DWITH_TESTS=' + cmake_option(args.with_tests)]
    cmake_args += common_cmake_arguments(args)

    if common.is_windows_platform():
        cmake_args += ['-DBUILD_EXECUTABLE_WIN32INTERRUPT=OFF',
                       '-DBUILD_EXECUTABLE_WIN64INTERRUPT=OFF',
                       '-DBUILD_LIBRARY_QTCREATORCDBEXT=OFF']

    ide_revision = common.get_commit_SHA(paths.src)
    if ide_revision:
        cmake_args += ['-DIDE_REVISION=ON',
                       '-DIDE_REVISION_STR=' + ide_revision[:10],
                       '-DIDE_REVISION_URL=https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=' + ide_revision]

    if not args.build_type.lower() == 'release' and args.sanitize_flags:
        cmake_args += ['-DWITH_SANITIZE=ON',
                       '-DSANITIZE_FLAGS=' + ",".join(args.sanitize_flags)]

    if args.with_cpack:
        cmake_args += ['-DCPACK_PACKAGE_FILE_NAME=qtcreator' + args.zip_infix]
        if common.is_linux_platform():
            cmake_args += ['-DCPACK_INSTALL_PREFIX=/opt/qt-creator']

    common.check_print_call(cmake_args + [paths.src], paths.build)
    build_args = ['cmake', '--build', '.']
    if args.make_args:
        build_args += ['--'] + args.make_args
    common.check_print_call(build_args, paths.build)
    if not args.no_docs:
        common.check_print_call(['cmake', '--build', '.', '--target', 'docs'], paths.build)

    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install, '--strip'],
                            paths.build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                             '--component', 'Dependencies'],
                            paths.build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.dev_install,
                             '--component', 'Devel'],
                            paths.build)
    if args.with_debug_info:
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.debug_install,
                                 '--component', 'DebugInfo'],
                                 paths.build)
    if not args.no_docs:
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'qch_docs'],
                                paths.build)
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'html_docs'],
                                paths.build)

def build_wininterrupt(args, paths):
    if not common.is_windows_platform():
        return
    if not os.path.exists(paths.wininterrupt_build):
        os.makedirs(paths.wininterrupt_build)
    prefix_paths = [common.to_posix_path(os.path.abspath(fp)) for fp in args.prefix_paths]
    cmake_args = ['-DCMAKE_PREFIX_PATH=' + ';'.join(prefix_paths),
                  '-DCMAKE_INSTALL_PREFIX=' + common.to_posix_path(paths.wininterrupt_install)]
    cmake_args += common_cmake_arguments(args)
    common.check_print_call(['cmake'] + cmake_args + [os.path.join(paths.src, 'src', 'tools', 'wininterrupt')],
                            paths.wininterrupt_build)
    common.check_print_call(['cmake', '--build', '.'], paths.wininterrupt_build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.wininterrupt_install,
                             '--component', 'wininterrupt'],
                            paths.wininterrupt_build)

def build_qtcreatorcdbext(args, paths):
    if args.no_cdb:
        return
    if not os.path.exists(paths.qtcreatorcdbext_build):
        os.makedirs(paths.qtcreatorcdbext_build)
    prefix_paths = [common.to_posix_path(os.path.abspath(fp)) for fp in args.prefix_paths]
    cmake_args = ['-DCMAKE_PREFIX_PATH=' + ';'.join(prefix_paths),
                  '-DCMAKE_INSTALL_PREFIX=' + common.to_posix_path(paths.qtcreatorcdbext_install)]
    cmake_args += common_cmake_arguments(args)
    common.check_print_call(['cmake'] + cmake_args + [os.path.join(paths.src, 'src', 'libs', 'qtcreatorcdbext')],
                            paths.qtcreatorcdbext_build)
    common.check_print_call(['cmake', '--build', '.'], paths.qtcreatorcdbext_build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.qtcreatorcdbext_install,
                             '--component', 'qtcreatorcdbext'],
                            paths.qtcreatorcdbext_build)

def zipPatternForApp(paths):
    # workaround for QTBUG-95845
    if not common.is_mac_platform():
        return '*'
    apps = [d for d in os.listdir(paths.install) if d.endswith('.app')]
    return apps[0] if apps else '*'


def package_qtcreator(args, paths):
    if common.is_windows_platform() and args.sign_command:
        command = shlex.split(args.sign_command)
        if not args.no_qtcreator:
            common.check_print_call(command + [paths.install])
        common.check_print_call(command + [paths.wininterrupt_install])
        if not args.no_cdb:
            common.check_print_call(command + [paths.qtcreatorcdbext_install])

    if not args.no_zip:
        if not args.no_qtcreator:
            common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                     os.path.join(paths.result, 'qtcreator' + args.zip_infix + '.7z'),
                                     zipPatternForApp(paths)],
                                    paths.install)
            common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                     os.path.join(paths.result, 'qtcreator' + args.zip_infix + '_dev.7z'),
                                     '*'],
                                    paths.dev_install)
            if args.with_debug_info:
                common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                         os.path.join(paths.result, 'qtcreator' + args.zip_infix + '-debug.7z'),
                                         '*'],
                                        paths.debug_install)
        if common.is_windows_platform():
            common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                     os.path.join(paths.result, 'wininterrupt' + args.zip_infix + '.7z'),
                                     '*'],
                                    paths.wininterrupt_install)
            if not args.no_cdb:
                common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                         os.path.join(paths.result, 'qtcreatorcdbext' + args.zip_infix + '.7z'),
                                         '*'],
                                        paths.qtcreatorcdbext_install)

    if common.is_mac_platform() and not args.no_qtcreator:
        if args.keychain_unlock_script:
            common.check_print_call([args.keychain_unlock_script], paths.install)
        if os.environ.get('SIGNING_IDENTITY'):
            signed_install_path = paths.install + '-signed'
            common.copytree(paths.install, signed_install_path, symlinks=True)
            apps = [d for d in os.listdir(signed_install_path) if d.endswith('.app')]
            if apps:
                app = apps[0]
                common.codesign(os.path.join(signed_install_path, app))
                if not args.no_zip:
                    common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                             os.path.join(paths.result, 'qtcreator' + args.zip_infix + '-signed.7z'),
                                             app],
                                            signed_install_path)
        if not args.no_dmg:
            common.check_print_call(['python', '-u',
                                     os.path.join(paths.src, 'scripts', 'makedmg.py'),
                                     'qt-creator' + args.zip_infix + '.dmg',
                                     'Qt Creator',
                                     paths.src,
                                     paths.install],
                                    paths.result)
    if args.with_cpack and args.cpack_generators:
        common.check_print_call(['cpack', '-G', ';'.join(args.cpack_generators)], paths.build)


def get_paths(args):
    Paths = collections.namedtuple('Paths',
                                   ['qt', 'src', 'build', 'wininterrupt_build', 'qtcreatorcdbext_build',
                                    'install', 'dev_install', 'debug_install',
                                    'wininterrupt_install', 'qtcreatorcdbext_install', 'result',
                                    'elfutils', 'llvm'])
    build_path = os.path.abspath(args.build)
    install_path = os.path.join(build_path, 'install')
    qt_path = os.path.abspath(args.qt_path) if args.qt_path else None
    return Paths(qt=qt_path,
                 src=os.path.abspath(args.src),
                 build=os.path.join(build_path, 'build'),
                 wininterrupt_build=os.path.join(build_path, 'build-wininterrupt'),
                 qtcreatorcdbext_build=os.path.join(build_path, 'build-qtcreatorcdbext'),
                 install=os.path.join(install_path, 'qt-creator'),
                 dev_install=os.path.join(install_path, 'qt-creator-dev'),
                 debug_install=os.path.join(install_path, 'qt-creator-debug'),
                 wininterrupt_install=os.path.join(install_path, 'wininterrupt'),
                 qtcreatorcdbext_install=os.path.join(install_path, 'qtcreatorcdbext'),
                 result=build_path,
                 elfutils=os.path.abspath(args.elfutils_path) if args.elfutils_path else None,
                 llvm=os.path.abspath(args.llvm_path) if args.llvm_path else None)

def main():
    args = get_arguments()
    paths = get_paths(args)

    build_qtcreator(args, paths)
    build_wininterrupt(args, paths)
    build_qtcreatorcdbext(args, paths)
    package_qtcreator(args, paths)

if __name__ == '__main__':
    main()
