#!/usr/bin/env python
############################################################################
#
# Copyright (C) 2018 The Qt Company Ltd.
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
import shutil
import subprocess
import tempfile

import common

def parse_arguments():
    parser = argparse.ArgumentParser(description="Create Qt Creator package, filtering out debug information files.",
        epilog="To sign the contents before packaging on macOS, set the SIGNING_IDENTITY and optionally the SIGNING_FLAGS environment variables.")
    parser.add_argument('--7z', help='path to 7z binary',
        default='7z.exe' if common.is_windows_platform() else '7z',
        metavar='<7z_binary>', dest='sevenzip')
    parser.add_argument('--debug', help='package only the files with debug information',
                        dest='debug', action='store_true', default=False)
    parser.add_argument('--exclude-toplevel', help='do not include the toplevel source directory itself in the resulting archive, only its contents',
                        dest='exclude_toplevel', action='store_true', default=False)
    parser.add_argument('target_archive', help='output 7z file to create')
    parser.add_argument('source_directory', help='source directory with the Qt Creator installation')
    return parser.parse_args()

def main():
    arguments = parse_arguments()
    tempdir_base = tempfile.mkdtemp()
    tempdir = os.path.join(tempdir_base, os.path.basename(arguments.source_directory))
    try:
        common.copytree(arguments.source_directory, tempdir, symlinks=True,
            ignore=(common.is_not_debug if arguments.debug else common.is_debug))
        # on macOS we might have to codesign (again) to account for removed debug info
        if not arguments.debug:
            common.codesign(tempdir)
        # package
        zip_source = os.path.join(tempdir, '*') if arguments.exclude_toplevel else tempdir
        subprocess.check_call([arguments.sevenzip, 'a',
            arguments.target_archive, zip_source])
    finally:
        shutil.rmtree(tempdir_base)
if __name__ == "__main__":
    main()
