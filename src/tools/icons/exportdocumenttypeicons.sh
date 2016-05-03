#!/bin/sh

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

# optipng is required by this script
if ! hash optipng 2>/dev/null; then
    echo "optipng was not found in PATH" >&2
    exit 1
fi

cd `dirname $0`

# Adding the icons for the OSX document type icon for .ui files
for documentTypeName in "uifile" "profile" "prifile";\
do
    inconsetName=${documentTypeName}.iconset
    rm -rf $inconsetName
    mkdir $inconsetName
    for iconSize in 16 32 128 256 512;\
    do
        iconShortID="icon_${iconSize}x${iconSize}"
        iconLongID="${documentTypeName}_${iconShortID}"
        for sizeVariation in "" "@2x";\
        do
             iconSourcePng="${iconLongID}${sizeVariation}.png"
             iconTargetPng="${inconsetName}/${iconShortID}${sizeVariation}.png"
             optipng $iconSourcePng -o 7 -strip all
             cp $iconSourcePng $iconTargetPng
        done
    done
done
