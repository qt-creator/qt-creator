#!/usr/bin/env python

############################################################################
#
# Copyright (C) 2022 The Qt Company Ltd.
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

# This script calls Inkscape to rasterize several images into png files.
# The images end up in the final position of the source tree.
# Each image is generated as normal and high resolution variant.
# Each png file is afterwards optimized with optipng.

import argparse
import os
import pathlib
import subprocess
import sys

from distutils import spawn


def qtcRoot():
    return os.path.abspath(
        os.path.join(os.path.dirname(sys.argv[0]), '../../..')).replace('\\', '/')


def recolorizePng(pngPath, color, magick):
    print("Recolorizing: {}".format(pngPath))
    try:
        subprocess.check_call([magick,
                               pngPath,
                               "-depth", "24",
                               "-colorspace", "rgb",
                               "(", "+clone", "-background", "white", "-flatten", ")",
                               "(", "+clone", "-negate", "-alpha", "copy", ")",
                               "(", "+clone", "-fuzz", "100%", "-fill", color, "-opaque", color, ")",
                               "(", "-clone", "-1,-2", "-compose", "dst_in", "-composite", ")",
                               "-delete", "0--2",
                               pngPath])
    except subprocess.CalledProcessError:
        sys.exit("Failed to recolorize {}.".format(pngPath))


def optimizePng(pngPath, optipng):
    print("Optimizing: {}".format(pngPath))
    try:
        subprocess.check_call([optipng,
                               "-o7",
                               "-strip", "all",
                               pngPath],
                              stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        sys.exit("Failed to optimize {}.".format(pngPath))


def processDocsPath(docsPath, color, magick, optipng):
    pngPaths = docsPath.glob('**/*.png')
    for pngPath in pngPaths:
        recolorizePng(pngPath, color, magick)
        optimizePng(pngPath, optipng)


def main():
    parser = argparse.ArgumentParser(description='Recolor icon .png files in the '
                                     'Documentation. '
                                     'Requires ImageMagick and Optipng in Path.')
    parser.add_argument('-docspath',
                        help='The path to process (recursively).',
                        type=pathlib.Path,
                        default=qtcRoot() + "/doc/qtcreator/images/icons")
    parser.add_argument('-color',
                        help='The icon foreground color.',
                        default="#808080")
    args = parser.parse_args()

    magick = spawn.find_executable("magick")
    if magick is None:
        sys.exit("ImageMagick was not found in Path.")

    optipng = spawn.find_executable("optipng")
    if optipng is None:
        sys.exit("Optipng was not found in Path.")

    processDocsPath(args.docspath, args.color, magick, optipng)

    return 0


if __name__ == '__main__':
    sys.exit(main())
