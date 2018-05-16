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
import time

import common

def parse_arguments():
    parser = argparse.ArgumentParser(description='Create Qt Creator disk image, filtering out debug information files.',
        epilog="To sign the contents before packaging on macOS, set the SIGNING_IDENTITY and optionally the SIGNING_FLAGS environment variables.")
    parser.add_argument('target_diskimage', help='output .dmg file to create')
    parser.add_argument('dmg_volumename', help='volume name to use for the disk image')
    parser.add_argument('source_directory', help='directory with the Qt Creator sources')
    parser.add_argument('binary_directory', help='directory that contains the Qt Creator.app')
    return parser.parse_args()

def main():
    arguments = parse_arguments()
    tempdir_base = tempfile.mkdtemp()
    tempdir = os.path.join(tempdir_base, os.path.basename(arguments.binary_directory))
    try:
        common.copytree(arguments.binary_directory, tempdir, symlinks=True, ignore=common.is_debug)
        if common.is_mac_platform():
            app_path = [app for app in os.listdir(tempdir) if app.endswith('.app')][0]
            common.codesign(os.path.join(tempdir, app_path))
        os.symlink('/Applications', os.path.join(tempdir, 'Applications'))
        shutil.copy(os.path.join(arguments.source_directory, 'LICENSE.GPL3-EXCEPT'), tempdir)
        dmg_cmd = ['hdiutil', 'create', '-srcfolder', tempdir, '-volname', arguments.dmg_volumename,
            '-format', 'UDBZ', arguments.target_diskimage, '-ov', '-scrub', '-size', '1g', '-verbose']
        subprocess.check_call(dmg_cmd)
        # sleep a few seconds to make sure disk image is fully unmounted etc
        time.sleep(5)
    finally:
        shutil.rmtree(tempdir_base)
if __name__ == "__main__":
    main()
