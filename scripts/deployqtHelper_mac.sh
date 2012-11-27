#!/bin/bash
[ $# -lt 2 ] && echo "Usage: $(basename $0) <app folder> <qt translations folder>" && exit 2
[ $(uname -s) != "Darwin" ] && echo "Run this script on Mac OS X" && exit 2;

macdeployqt "$1" \
        "-executable=$1/Contents/MacOS/qmlpuppet.app/Contents/MacOS/qmlpuppet" \
        "-executable=$1/Contents/Resources/qtpromaker" \
        "-executable=$1/Contents/Resources/sdktool" || exit 1

qmlpuppetResources="$1/Contents/MacOS/qmlpuppet.app/Contents/Resources"
test -d "$qmlpuppetResources" || mkdir -p "$qmlpuppetResources"
cp "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qmlpuppet_qt.conf" "$qmlpuppetResources/qt.conf"

# copy Qt translations
cp "$2"/qt_*.qm "$1/Contents/Resources/translations/"
