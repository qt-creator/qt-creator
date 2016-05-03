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

# This script creates several application icon files by using
# Inkscape to rasterize .svg items to .png, adding shadows via
# imagemagick, creating .ico files via imagemagick and .icns
# files via iconutil (OSX only).

# optipng is required by this script
if ! hash optipng 2>/dev/null; then
    echo "optipng was not found in PATH" >&2
    exit 1
fi

# Imagemagick convert is required by this script
if ! hash convert 2>/dev/null; then
    echo "Imagemagick convert was not found in PATH" >&2
    exit 1
fi

cd `dirname $0`

applicationNames="qtcreator designer linguist assistant qdbusviewer qmlviewer"
applicationIconDimensions="16:0 24:0 32:1 48:1 64:1 128:2 256:3 512:7 1024:15"

# Creating the list of svg IDs to export
for applicationName in $applicationNames;\
do
    for applicationIconDimension in $applicationIconDimensions;\
    do
       applicationIconSize=`echo $applicationIconDimension | awk -F: '{ print $1 }'`
       iconIDs="${iconIDs} ${applicationName}_icon_${applicationIconSize}x${applicationIconSize}"
    done
done

# Copying the logos for Qt Creator's sources. Without shadows!
creatorLogoDir="logo"
rm -rf $creatorLogoDir
mkdir $creatorLogoDir
for uiFileIconSize in 16 24 32 48 64 128 256 512;\
do
    creatorLogoSource="qtcreator_icon_${uiFileIconSize}x${uiFileIconSize}.png"
    creatorLogoTargetDir="${creatorLogoDir}/${uiFileIconSize}"
    creatorLogoTarget="${creatorLogoTargetDir}/QtProject-qtcreator.png"
    optipng $creatorLogoSource -o 7 -strip all
    mkdir $creatorLogoTargetDir
    cp $creatorLogoSource $creatorLogoTarget
done

# Adding the shadows to the .png files
for applicationName in $applicationNames;\
do
    for applicationIconDimension in $applicationIconDimensions;\
    do
       iconSize=`echo $applicationIconDimension | awk -F: '{ print $1 }'`
       shadowSize=`echo $applicationIconDimension | awk -F: '{ print $2 }'`
       iconFile=${applicationName}_icon_${iconSize}x${iconSize}.png
       if [ "$shadowSize" != "0" ]
       then
           convert -page ${iconSize}x${iconSize} ${iconFile} \( +clone -background black -shadow 25x1+0+0 \) +swap -background none -flatten ${iconFile}
           convert -page ${iconSize}x${iconSize} ${iconFile} \( +clone -background black -shadow 40x${shadowSize}+0+${shadowSize} \) +swap -background none -flatten ${iconFile}
       fi
    done
done

# Creating the .ico files
iconSizes="256 128 64 48 32 24 16"
for applicationName in $applicationNames;\
do
    icoFiles=""
    for iconSize in $iconSizes;\
    do
       icoFiles="$icoFiles ${applicationName}_icon_${iconSize}x${iconSize}.png"
    done
    convert ${icoFiles} ${applicationName}.ico
done

# Optimizing the .pngs
for iconID in $iconIDs;\
do
   optipng "${iconID}.png" -o 7 -strip all
done

# Preparing application .iconsets for the conversion to .icns
for applicationName in $applicationNames;\
do
    inconsetName=${applicationName}.iconset
    rm -rf $inconsetName
    mkdir $inconsetName
    cp ${applicationName}_icon_16x16.png ${inconsetName}/icon_16x16.png
    cp ${applicationName}_icon_32x32.png ${inconsetName}/icon_32x32.png
    cp ${applicationName}_icon_128x128.png ${inconsetName}/icon_128x128.png
    cp ${applicationName}_icon_256x256.png ${inconsetName}/icon_256x256.png
    cp ${applicationName}_icon_512x512.png ${inconsetName}/icon_512x512.png
    cp ${applicationName}_icon_32x32.png ${inconsetName}/icon_16x16@2x.png
    cp ${applicationName}_icon_64x64.png ${inconsetName}/icon_32x32@2x.png
    cp ${applicationName}_icon_256x256.png ${inconsetName}/icon_128x128@2x.png
    cp ${applicationName}_icon_512x512.png ${inconsetName}/icon_256x256@2x.png
    cp ${applicationName}_icon_1024x1024.png ${inconsetName}/icon_512x512@2x.png
done
