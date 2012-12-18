#!/bin/bash
[ $# -lt 3 ] && echo "Usage: $(basename $0) <app folder> <qt translations folder> <qt plugin folder>" && exit 2
[ $(uname -s) != "Darwin" ] && echo "Run this script on Mac OS X" && exit 2;

# collect designer plugins
designerDestDir="$1/Contents/PlugIns/designer"
test -d "$designerDestDir" || mkdir -p "$designerDestDir"
for plugin in "$3"/designer/*.dylib; do
   cp "$plugin" "$designerDestDir"/ || exit 1
   pluginbase=`basename "$plugin"`
   designerPluginArguments+="\"-executable=$designerDestDir/$pluginbase\" "
done

macdeployqt "$1" \
        "-executable=$1/Contents/MacOS/qmlpuppet.app/Contents/MacOS/qmlpuppet" \
        "-executable=$1/Contents/Resources/qtpromaker" \
        "-executable=$1/Contents/Resources/sdktool" $designerPluginArguments || exit 1

qmlpuppetResources="$1/Contents/MacOS/qmlpuppet.app/Contents/Resources"
test -d "$qmlpuppetResources" || mkdir -p "$qmlpuppetResources"
cp "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qmlpuppet_qt.conf" "$qmlpuppetResources/qt.conf" || exit 1

# copy Qt translations
cp "$2"/*.qm "$1/Contents/Resources/translations/" || exit 1
