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
import os

import common

def get_arguments():
    parser = argparse.ArgumentParser(description='Build Qt Creator for packaging')
    parser.add_argument('--name', help='Name to use for build results', required=True)
    parser.add_argument('--src', help='Path to sources', required=True)
    parser.add_argument('--build', help='Path that should be used for building', required=True)
    parser.add_argument('--qt-path', help='Path to Qt', required=True)
    parser.add_argument('--qtc-path',
                        help='Path to Qt Creator installation including development package',
                        required=True)
    parser.add_argument('--output-path', help='Output path for resulting 7zip files')
    parser.add_argument('--add-path', help='Adds a CMAKE_PREFIX_PATH to the build',
                        action='append', dest='prefix_paths', default=[])
    parser.add_argument('--add-config', help=('Adds the argument to the CMake configuration call. '
                        'Use "--add-config=-DSOMEVAR=SOMEVALUE" if the argument begins with a dash.'),
                        action='append', dest='config_args', default=[])
    parser.add_argument('--with-docs', help='Build and install documentation.',
                        action='store_true', default=False)
    parser.add_argument('--deploy', help='Installs the "Dependencies" component of the plugin.',
                        action='store_true', default=False)
    parser.add_argument('--debug', help='Enable debug builds', action='store_true', default=False)
    return parser.parse_args()

def build(args, paths):
    if not os.path.exists(paths.build):
        os.makedirs(paths.build)
    if not os.path.exists(paths.result):
        os.makedirs(paths.result)
    prefix_paths = [paths.qt, paths.qt_creator] + [os.path.abspath(fp) for fp in args.prefix_paths]
    build_type = 'Debug' if args.debug else 'Release'
    cmake_args = ['cmake',
                  '-DCMAKE_PREFIX_PATH=' + ';'.join(prefix_paths),
                  '-DCMAKE_BUILD_TYPE=' + build_type,
                  '-DCMAKE_INSTALL_PREFIX=' + paths.install,
                  '-G', 'Ninja']

    # force MSVC on Windows, because it looks for GCC in the PATH first,
    # even if MSVC is first mentioned in the PATH...
    # TODO would be nicer if we only did this if cl.exe is indeed first in the PATH
    if common.is_windows_platform():
        cmake_args += ['-DCMAKE_C_COMPILER=cl',
                       '-DCMAKE_CXX_COMPILER=cl']

    # TODO this works around a CMake bug https://gitlab.kitware.com/cmake/cmake/issues/20119
    cmake_args += ['-DBUILD_WITH_PCH=OFF']

    if args.with_docs:
        cmake_args += ['-DWITH_DOCS=ON']

    ide_revision = common.get_commit_SHA(paths.src)
    if ide_revision:
        cmake_args += ['-DQTC_PLUGIN_REVISION=' + ide_revision]
        with open(os.path.join(paths.result, args.name + '.7z.git_sha'), 'w') as f:
            f.write(ide_revision)

    cmake_args += args.config_args
    common.check_print_call(cmake_args + [paths.src], paths.build)
    common.check_print_call(['cmake', '--build', '.'], paths.build)
    if args.with_docs:
        common.check_print_call(['cmake', '--build', '.', '--target', 'docs'], paths.build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install, '--strip'],
                            paths.build)
    if args.with_docs:
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'qch_docs'],
                                paths.build)
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'html_docs'],
                                paths.build)
    if args.deploy:
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'Dependencies'],
                                paths.build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.dev_install,
                             '--component', 'Devel'],
                            paths.build)

def package(args, paths):
    if not os.path.exists(paths.result):
        os.makedirs(paths.result)
    common.check_print_call(['7z', 'a', '-mmt2', os.path.join(paths.result, args.name + '.7z'), '*'],
                            paths.install)
    if os.path.exists(paths.dev_install):  # some plugins might not provide anything in Devel
        common.check_print_call(['7z', 'a', '-mmt2',
                                 os.path.join(paths.result, args.name + '_dev.7z'), '*'],
                                paths.dev_install)

def get_paths(args):
    Paths = collections.namedtuple('Paths',
                                   ['qt', 'src', 'build', 'qt_creator',
                                    'install', 'dev_install', 'result'])
    build_path = os.path.abspath(args.build)
    install_path = os.path.join(build_path, 'install')
    result_path = os.path.abspath(args.output_path) if args.output_path else build_path
    return Paths(qt=os.path.abspath(args.qt_path),
                 src=os.path.abspath(args.src),
                 build=os.path.join(build_path, 'build'),
                 qt_creator=os.path.abspath(args.qtc_path),
                 install=os.path.join(install_path, args.name),
                 dev_install=os.path.join(install_path, args.name + '-dev'),
                 result=result_path)

def main():
    args = get_arguments()
    paths = get_paths(args)

    build(args, paths)
    package(args, paths)

if __name__ == '__main__':
    main()
