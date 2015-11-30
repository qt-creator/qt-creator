#!/usr/bin/env python
#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

# This script calls Inkscape to rasterize several images into png files.
# The images end up in the final position of the source tree.
# Each images is generated as normal and high resolution variant.
# Each png file is afterwards optimized with optipng.

import sys, os, subprocess, xml.etree.ElementTree as ET
from distutils import spawn

scriptDir = os.path.dirname(os.path.abspath(sys.argv[0])) + '/'

# QTC_SRC is expected to point to the source root of Qt Creator
qtcSourceRoot = os.getenv('QTC_SRC', os.path.abspath(scriptDir + '../../..')) \
    .replace('\\', '/') + '/'

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
            svgIDs.append(svgElementID)
    except:
        pass

# The shell mode of Inkscape is used to execute several export commands
# with one launch of Inkscape.
inkscapeShellCommands = ""
for id in svgIDs:
    inkscapeShellCommands += "qtcreatoricons.svg --export-id=" + id + " --export-id-only --export-png=" + qtcSourceRoot + id + ".png\n"
    inkscapeShellCommands += "qtcreatoricons.svg --export-id=" + id + " --export-id-only --export-png=" + qtcSourceRoot + id + "@2x.png --export-dpi=180\n"
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
