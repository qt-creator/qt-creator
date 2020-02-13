#!/usr/bin/env python

############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
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

import argparse
import os
import subprocess

import common

def parse_arguments():
    parser = argparse.ArgumentParser(description="Deploy and 7z a directory of plugins.")
    parser.add_argument('--7z', help='path to 7z binary',
        default='7z.exe' if common.is_windows_platform() else '7z',
        metavar='<7z_binary>', dest='sevenzip')
    parser.add_argument('--qmake_binary', help='path to qmake binary which was used for compilation',
        required=common.is_linux_platform(), metavar='<qmake_binary>')
    parser.add_argument('source_directory', help='directory to deploy and 7z')
    parser.add_argument('target_file', help='target file path of the resulting 7z')
    return parser.parse_args()

if __name__ == "__main__":
    arguments = parse_arguments()
    qt_install_info = common.get_qt_install_info(arguments.qmake_binary)
    if common.is_linux_platform():
        common.fix_rpaths(arguments.source_directory,
                          os.path.join(arguments.source_directory, 'lib', 'Qt', 'lib'),
                          qt_install_info)
    if common.is_mac_platform():
        # remove Qt rpath
        lib_path = qt_install_info['QT_INSTALL_LIBS']
        common.os_walk(arguments.source_directory,
                       lambda fp: fp.endswith('.dylib'),
                       lambda fp: subprocess.call(['install_name_tool', '-delete_rpath', lib_path, fp]))
    subprocess.check_call([arguments.sevenzip, 'a', '-mx9', arguments.target_file,
        os.path.join(arguments.source_directory, '*')])
