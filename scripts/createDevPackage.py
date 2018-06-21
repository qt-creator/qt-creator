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
import re
import subprocess
import sys

import common

# ========= COMMAND LINE ARGUMENTS ========

def parse_arguments():
    parser = argparse.ArgumentParser(description="Create Qt Creator development package.")
    parser.add_argument('--source', '-s', help='path to the Qt Creator sources', required=True,
        metavar='<path>')
    parser.add_argument('--build', '-b', help='path to the Qt Creator build', required=True,
        metavar='<path>')
    parser.add_argument('--verbose', '-v', help='verbose output', action='store_true', default=False)
    parser.add_argument('--7z', help='path to 7z binary',
        default='7z.exe' if common.is_windows_platform() else '7z',
        metavar='<7z_binary>', dest='sevenzip')
    parser.add_argument('--7z_out', '-o', help='output 7z file to create', metavar='<filename>',
        dest='sevenzip_target')
    parser.add_argument('target_directory')
    return parser.parse_args()

# ========= PATTERNS TO INCLUDE ===========

# slash at the end matches directories, no slash at the end matches files

source_include_patterns = [
    # directories
    r"^(?!(share|tests)/.*$)(.*/)?$",                     # look into all directories except under share/ and tests/
    r"^share/(qtcreator/(qml/(qmlpuppet/(.*/)?)?)?)?$", # for shared headers for qt quick designer plugins
    # files
    r"^HACKING$",
    r"^LICENSE.*$",
    r"^README.md$",
    r"^scripts/.*$", # include everything under scripts/
    r"^doc/.*$",     # include everything under doc/
    r"^.*\.pri$",    # .pri files in all directories that are looked into
    r"^.*\.h$",      # .h files in all directories that are looked into
    r"^.*\.hpp$"     # .hpp files in all directories that are looked into
]

build_include_patterns = [
    # directories
    r"^src/$",
    r"^src/app/$",
    # files
    r"^src/app/app_version.h$"
]
if common.is_windows_platform():
    build_include_patterns.extend([
        r"^lib/(.*/)?$", # consider all directories under lib/
        r"^lib/.*\.lib$"
    ])

# =========================================

def copy_regexp(regexp, source_directory, target_directory, verbose):
    def ignore(directory, filenames):
        relative_dir = os.path.relpath(directory, source_directory)
        file_target_dir = os.path.join(target_directory, relative_dir)
        ignored = []
        for filename in filenames:
            relative_file_path = os.path.normpath(os.path.join(relative_dir, filename))
            relative_file_path = relative_file_path.replace(os.sep, '/')
            if os.path.isdir(os.path.join(directory, filename)):
                relative_file_path += '/' # let directories end with slash
            if not regexp.match(relative_file_path):
                ignored.append(filename)
            elif verbose and os.path.isfile(os.path.join(directory, filename)):
                print(os.path.normpath(os.path.join(file_target_dir, filename)))
        return ignored

    common.copytree(source_directory, target_directory, symlinks=True, ignore=ignore)

def sanity_check_arguments(arguments):
    if os.path.exists(arguments.target_directory) and (os.path.isfile(arguments.target_directory)
                                                       or len(os.listdir(arguments.target_directory)) > 0):
        print('error: target directory "{0}" exists and is not empty'.format(arguments.target_directory))
        return False
    return True

def main():
    arguments = parse_arguments()
    if not sanity_check_arguments(arguments):
        sys.exit(1)

    source_include_regexp = re.compile('(' + ')|('.join(source_include_patterns) + ')')
    build_include_regexp = re.compile('(' + ')|('.join(build_include_patterns) + ')')

    copy_regexp(source_include_regexp, arguments.source, arguments.target_directory, arguments.verbose)
    copy_regexp(build_include_regexp, arguments.build, arguments.target_directory, arguments.verbose)

    if arguments.sevenzip_target:
        subprocess.check_call([arguments.sevenzip, 'a', '-mx9', arguments.sevenzip_target,
            os.path.join(arguments.target_directory, '*')])

if __name__ == "__main__":
    main()
