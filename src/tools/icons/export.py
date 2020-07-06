#!/usr/bin/env python

############################################################################
#
# Copyright (C) 2020 The Qt Company Ltd.
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
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

from distutils import spawn


def qtcRoot():
    return os.path.abspath(
        os.path.join(os.path.dirname(sys.argv[0]), '../../..')).replace('\\', '/')


def svgIDs(svgFile, svgElementFilter):
    # The svg element IDs of images to export. They correspond to the
    # path and base name of each image in the Qt Creator sources.
    svgIDs = []
    svgTree = ET.ElementTree()
    svgTree.parse(os.path.join(qtcRoot(), svgFile))
    svgTreeRoot = svgTree.getroot()
    pattern = re.compile(svgElementFilter)
    for svgElement in svgTreeRoot.iter():
        try:
            svgElementID = svgElement.attrib['id']
            if '/' in svgElementID and pattern.match(svgElementID):
                svgIDs.append(svgElementID)
        except Exception:
            pass

    print("\n==== {} elements found which match {}"
          .format(len(svgIDs), svgElementFilter))
    return svgIDs


def pngName(svgID, scale):
    # File name is relative to qtcRoot()
    return svgID + ("" if scale == 1 else "@{}x".format(scale)) + ".png"


def checkDirectories(svgIDs):
    invalidDirectories = []
    for id in svgIDs:
        if not os.path.isdir(os.path.join(qtcRoot(), id, '../')):
            invalidDirectories.append(id)

    if invalidDirectories:
        print("\n==== {} IDs for which the output directory is missing:"
              .format(len(invalidDirectories)))
        print("\n".join(invalidDirectories))
        sys.exit("Output directories are missing.")


def printOutUnexported(svgIDs, scaleFactors):
    unexported = []
    partiallyExported = []
    for id in svgIDs:
        exportedCount = 0
        for scaleFactor in scaleFactors:
            if os.path.isfile(os.path.join(qtcRoot(), pngName(id, scaleFactor))):
                exportedCount += 1
        if exportedCount == 0:
            unexported.append(id)
        elif (exportedCount < len(scaleFactors)):
            partiallyExported.append(id)

    if partiallyExported:
        print("\n==== {} IDs for which not each .png is exported:"
              .format(len(partiallyExported)))
        print("\n".join(partiallyExported))
    if unexported:
        print("\n==== {} IDs for which all .pngs are missing:"
              .format(len(unexported)))
        print("\n".join(unexported))
    if partiallyExported or unexported:
        input("\nPress Enter to continue...")


def exportPngs(svgIDs, svgFile, scaleFactors, inkscape):
    inkscapeProcess = subprocess.Popen([inkscape, '--shell'],
                                       stdin=subprocess.PIPE,
                                       cwd=qtcRoot())
    actions = ["file-open:" + svgFile]
    for id in svgIDs:
        for scale in scaleFactors:
            actions += [
                "export-id:{}".format(id),
                "export-id-only",
                "export-dpi:{}".format(scale * 96),
                "export-filename:{}".format(pngName(id, scale)),
                "export-do"
            ]
    actions += ["quit-inkscape"]
    actionLine = "; ".join(actions) + "\n"
    print("Exporting pngs for {} Ids in {} scale factors."
          .format(len(svgIDs), len(scaleFactors)))
    inkscapeProcess.communicate(input=actionLine.encode())


def optimizePngs(svgIDs, scaleFactors, optipng):
    for id in svgIDs:
        for scale in scaleFactors:
            png = pngName(id, scale)
            print("Optimizing: {}".format(png))
            try:
                subprocess.check_call([optipng,
                                       "-o7",
                                       "-strip", "all",
                                       png],
                                      stderr=subprocess.DEVNULL,
                                      cwd=qtcRoot())
            except subprocess.CalledProcessError:
                sys.exit("Failed to optimize {}.".format(png))


def export(svgFile, filter, scaleFactors, inkscape, optipng):
    ids = svgIDs(svgFile, filter)
    if not ids:
        sys.exit("{} does not match any Id.".format(filter))

    checkDirectories(ids)
    printOutUnexported(ids, scaleFactors)
    exportPngs(ids, svgFile, scaleFactors, inkscape)
    optimizePngs(ids, scaleFactors, optipng)


def main():
    parser = argparse.ArgumentParser(description='Export svg elements to .png '
                                     'files and optimize the png. '
                                     'Requires Inkscape 1.x and optipng in Path.')
    parser.add_argument('filter',
                        help='a RegExp filter for svg element Ids, e.g.: .*device.*')
    args = parser.parse_args()

    inkscape = spawn.find_executable("inkscape")
    if inkscape is None:
        sys.exit("Inkscape was not found in Path.")

    optipng = spawn.find_executable("optipng")
    if optipng is None:
        sys.exit("Optipng was not found in Path.")

    export("src/tools/icons/qtcreatoricons.svg", args.filter, [1, 2],
           inkscape, optipng)

    return 0


if __name__ == '__main__':
    sys.exit(main())
