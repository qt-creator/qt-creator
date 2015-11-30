#!/bin/bash

[ $# -lt 2 ] && echo "Usage: $(basename $0) <folder> <name.dmg>" && exit 2
[ $(uname -s) != "Darwin" ] && echo "Run this script on Mac OS X" && exit 2;
sourceFolder="$1"
intermediateFolder=$(mktemp -d "/tmp/packagedir.XXXXX")
finalDMGName="$2"
title="Qt Creator"

echo Preparing image artifacts...
cp -a "${sourceFolder}/" "${intermediateFolder}"
ln -s /Applications "${intermediateFolder}"
cp "$(dirname "${BASH_SOURCE[0]}")/../LICENSE.LGPLv21" "${intermediateFolder}/LICENSE_LGPLv21.txt"
cp "$(dirname "${BASH_SOURCE[0]}")/../LICENSE.LGPLv3" "${intermediateFolder}/LICENSE_LGPLv3.txt"
echo Creating image...
hdiutil create -srcfolder "${intermediateFolder}" -volname "${title}" -format UDBZ "${finalDMGName}" -ov -scrub -size 1g -verbose
# make sure that the image is umounted etc
sleep 4

# clean up
rm -rf "${intermediateFolder}"
