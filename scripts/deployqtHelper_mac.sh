#!/bin/bash

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

[ $# -lt 5 ] && echo "Usage: $(basename $0) <app folder> <qt bin folder> <qt translations folder> <qt plugin folder> <qt quick 2 imports folder>" && exit 2
[ $(uname -s) != "Darwin" ] && echo "Run this script on Mac OS X" && exit 2;

app_path="$1"
libexec_path="$app_path/Contents/Resources/libexec"
bin_src="$2"

echo "Deploying Qt"

#### macdeployqt

if [ ! -d "$app_path/Contents/Frameworks/QtCore.framework" ]; then

    echo "- Running macdeployqt ($bin_src/macdeployqt)"

    qbsapp="$app_path/Contents/MacOS/qbs"
    if [ -f "$qbsapp" ]; then
        qbsArguments=("-executable=$qbsapp" \
        "-executable=$qbsapp-config" \
        "-executable=$qbsapp-config-ui" \
        "-executable=$qbsapp-setup-android" \
        "-executable=$qbsapp-setup-qt" \
        "-executable=$qbsapp-setup-toolchains" \
        "-executable=$qbsapp-create-project" \
        "-executable=$libexec_path/qbs_processlauncher")
    fi

    qml2puppetapp="$libexec_path/qml2puppet"
    if [ -f "$qml2puppetapp" ]; then
        qml2puppetArgument="-executable=$qml2puppetapp"
    fi
    sdktoolapp="$libexec_path/sdktool"
    if [ -f "$sdktoolapp" ]; then
        sdktoolArgument="-executable=$sdktoolapp"
    fi

    "$bin_src/macdeployqt" "$app_path" \
        "-executable=$app_path/Contents/MacOS/qtdiag" \
        "-executable=$libexec_path/qtpromaker" \
        "$sdktoolArgument" \
        "-executable=$libexec_path/ios/iostool" \
        "-executable=$libexec_path/buildoutputparser" \
        "-executable=$libexec_path/cpaster" \
        "${qbsArguments[@]}" \
        "$qml2puppetArgument" || exit 1

fi

# clean up unneeded object files that are part of Qt for some static libraries
find "$app_path" -ipath "*/objects-*" -delete

# clean up after macdeployqt
# it deploys some plugins (and libs for these) that interfere with what we want
echo "Cleaning up after macdeployqt..."
toRemove=(\
    "Contents/PlugIns/tls/libqopensslbackend.dylib" \
    "Contents/PlugIns/sqldrivers/libqsqlpsql.dylib" \
    "Contents/PlugIns/sqldrivers/libqsqlodbc.dylib" \
    "Contents/Frameworks/libpq.*dylib" \
    "Contents/Frameworks/libssl.*dylib" \
    "Contents/Frameworks/libcrypto.*dylib" \
)
for f in "${toRemove[@]}"; do
    echo "- removing \"$app_path/$f\""
    rm "$app_path"/$f 2> /dev/null; done
exit 0
