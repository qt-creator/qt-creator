#!/bin/sh
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
# http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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

cd `dirname $0`

# QTC_SRC is expected to point to the source root of Qt Creator
if [ -z "$QTC_SRC" ]; then
    QTC_SRC=$(cd ../../..; pwd)
fi

# Inkscape is required by this script
if ! hash inkscape 2>/dev/null; then
    echo "Inkscape was not found in PATH" >&2
    exit 1
fi

# The svg element IDs of images to export. They correspond to the
# path and base name of each image in the Qt Creator sources.
ids=$(sed -n 's,^[[:space:]]*id="\(\(src\|share\)/[^"]*\)".*$,\1,p' qtcreatoricons.svg)

# The shell mode of Inkscape is used to execute several export commands
# with one launch of Inkscape. The commands are first all written into a
# file "inkscape_commands" and then piped into Inkscape's shell.
for i in $ids; do
    # The normal resolution (default = 90 dpi)
    echo "qtcreatoricons.svg --export-id=$i --export-id-only --export-png=$QTC_SRC/$i.png"
    # The "@2x" resolution
    echo "qtcreatoricons.svg --export-id=$i --export-id-only --export-png=$QTC_SRC/$i@2x.png --export-dpi=180"
done | inkscape --shell

if hash optipng 2>/dev/null; then
    # Optimizing pngs via optipng
    for i in $ids; do
        optipng -o7 -strip "all" $QTC_SRC/$i.png
        optipng -o7 -strip "all" $QTC_SRC/$i@2x.png
    done
else
    echo "optipng was not found in PATH. Please do not push the unoptimized .pngs to the main repository." >&2
fi
