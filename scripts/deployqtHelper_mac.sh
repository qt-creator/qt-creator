#!/bin/bash
[ $# -lt 5 ] && echo "Usage: $(basename $0) <app folder> <qt translations folder> <qt plugin folder> <qt quick imports folder> <qt quick 2 imports folder>" && exit 2
[ $(uname -s) != "Darwin" ] && echo "Run this script on Mac OS X" && exit 2;

# collect designer plugins
designerDestDir="$1/Contents/PlugIns/designer"
test -d "$designerDestDir" || mkdir -p "$designerDestDir"
for plugin in "$3"/designer/*.dylib; do
   cp "$plugin" "$designerDestDir"/ || exit 1
done

# copy Qt Quick 1 imports
importsDir="$1/Contents/Imports/qtquick1"
if [ -d "$4" ]; then
    test -d "$importsDir" || mkdir -p "$importsDir"
    cp -R "$4"/ "$importsDir"/
fi

# copy Qt Quick 2 imports
imports2Dir="$1/Contents/Imports/qtquick2"
if [ -d "$5" ]; then
    test -d "$imports2Dir" || mkdir -p "$imports2Dir"
    cp -R "$5"/ "$imports2Dir"/
fi

qml2puppetapp="$1/Contents/MacOS/qml2puppet.app"
if [ -d "$qml2puppetapp" ]; then
    qml2puppetArgument="-executable=$qml2puppetapp/Contents/MacOS/qml2puppet"
    qml2puppetResources="$1/Contents/MacOS/qml2puppet.app/Contents/Resources"
    test -d "$qml2puppetResources" || mkdir -p "$qml2puppetResources"
    cp "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qmlpuppet_qt.conf" "$qml2puppetResources/qt.conf" || exit 1
fi

macdeployqt "$1" \
        "-executable=$1/Contents/MacOS/qmlpuppet.app/Contents/MacOS/qmlpuppet" \
        "-executable=$1/Contents/Resources/qtpromaker" \
        "-executable=$1/Contents/Resources/sdktool" "$qml2puppetArgument" || exit 1

qmlpuppetResources="$1/Contents/MacOS/qmlpuppet.app/Contents/Resources"
test -d "$qmlpuppetResources" || mkdir -p "$qmlpuppetResources"
cp "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qmlpuppet_qt.conf" "$qmlpuppetResources/qt.conf" || exit 1

# copy Qt translations
cp "$2"/*.qm "$1/Contents/Resources/translations/" || exit 1
