#!/bin/bash
macdeployqt "$1" \
        "-executable=$1/Contents/MacOS/qmlpuppet.app/Contents/MacOS/qmlpuppet" \
        "-executable=$1/Contents/Resources/qtpromaker" \
        "-executable=$1/Contents/MacOS/qmlprofiler" || exit 1
qmlpuppetResources="$1/Contents/MacOS/qmlpuppet.app/Contents/Resources"
mkdir "$qmlpuppetResources"
cp "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qmlpuppet_qt.conf" "$qmlpuppetResources/qt.conf"
