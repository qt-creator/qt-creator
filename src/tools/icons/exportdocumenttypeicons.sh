#!/bin/sh

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# optipng is required by this script
if ! hash optipng 2>/dev/null; then
    echo "optipng was not found in PATH" >&2
    exit 1
fi

cd `dirname $0`

# Adding the icons for the OSX document type icon for .ui files
for documentTypeName in "uifile" "qtcreator-project";\
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
