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

# This script calls Inkscape to rasterize several images into png files.
# The images end up in the final position of the source tree.
# Each images is generated as normal and high resolution variant.
# Each png file is afterwards optimized with optipng.

import sys, os, subprocess, re, xml.etree.ElementTree as ET
from distutils import spawn

scriptDir = os.path.dirname(os.path.abspath(sys.argv[0])) + '/'

# QTC_SRC is expected to point to the source root of Qt Creator
qtcSourceRoot = os.getenv('QTC_SRC', os.path.abspath(scriptDir + '../../..')) \
    .replace('\\', '/') + '/'

svgElementFilter = ""
if len(sys.argv) > 1:
    svgElementFilter = sys.argv[1]

# Inkscape is required by this script
inkscapeExecutable = spawn.find_executable("inkscape")
if not inkscapeExecutable:
    sys.stderr.write("Inkscape was not found in PATH\n")
    sys.exit(1)

# The svg element IDs of images to export. They correspond to the
# path and base name of each image in the Qt Creator sources.
svgIDs = []
svgTree = ET.ElementTree()
svgTree.parse(scriptDir + "qtcreatoricons.svg")
svgTreeRoot = svgTree.getroot()
for svgElement in svgTreeRoot.iter():
    try:
        svgElementID = svgElement.attrib['id']
        if svgElementID.startswith(('src/', 'share/')):
            if svgElementFilter != "":
                pattern = re.compile(svgElementFilter)
                if pattern.match(svgElementID):
                    svgIDs.append(svgElementID)
            else:
                svgIDs.append(svgElementID)
    except:
        pass

# The shell mode of Inkscape is used to execute several export commands
# with one launch of Inkscape.
inkscapeShellCommands = ""
for id in svgIDs:
    inkscapeShellCommands += "qtcreatoricons.svg --export-id=" + id + " --export-id-only --export-png=" + qtcSourceRoot + id + ".png --export-dpi=96\n"
    inkscapeShellCommands += "qtcreatoricons.svg --export-id=" + id + " --export-id-only --export-png=" + qtcSourceRoot + id + "@2x.png --export-dpi=192\n"
inkscapeShellCommands += "quit\n"
inkscapeProcess = subprocess.Popen(['inkscape', '--shell'], stdin=subprocess.PIPE, shell=True, cwd=scriptDir)
inkscapeProcess.communicate(input=inkscapeShellCommands.encode())

# Optimizing pngs via optipng
optipngExecutable = spawn.find_executable("optipng")
if not optipngExecutable:
    sys.stderr.write("optipng was not found in PATH. Please do not push the unoptimized .pngs to the main repository.\n")
else:
    for id in svgIDs:
        subprocess.call(["optipng", "-o7", "-strip", "all", qtcSourceRoot + id + ".png"])
        subprocess.call(["optipng", "-o7", "-strip", "all", qtcSourceRoot + id + "@2x.png"])
