#!/bin/bash

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

[ $# -lt 5 ] && echo "Usage: $(basename $0) <app folder> <qt bin folder> <qt translations folder> <qt plugin folder> <qt quick 2 imports folder>" && exit 2
[ $(uname -s) != "Darwin" ] && echo "Run this script on Mac OS X" && exit 2;

app_path="$1"
resource_path="$app_path/Contents/Resources"
libexec_path="$app_path/Contents/Resources/libexec"
bin_src="$2"
translation_src="$3"

echo "Deploying Qt"

# copy qt creator qt.conf
if [ ! -f "$resource_path/qt.conf" ]; then
    echo "- Copying qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qt.conf" "$resource_path/qt.conf" || exit 1
fi

# copy libexec tools' qt.conf
if [ ! -f "$libexec_path/qt.conf" ]; then
    echo "- Copying libexec/qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/libexec_qt.conf" "$libexec_path/qt.conf" || exit 1
fi

# copy ios tools' qt.conf
if [ ! -f "$libexec_path/ios/qt.conf" ]; then
    echo "- Copying libexec/ios/qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/ios_qt.conf" "$libexec_path/ios/qt.conf" || exit 1
fi

# copy Qt translations
# check for known existing translation to avoid copying multiple times
if [ ! -f "$resource_path/translations/qt_de.qm" ]; then
    echo "- Copying Qt translations"
    cp "$translation_src"/*.qm "$resource_path/translations/" || exit 1
fi

# copy clang if needed
if [ $LLVM_INSTALL_DIR ]; then
    if [ "$LLVM_INSTALL_DIR"/bin/clangd -nt "$libexec_path"/clang/bin/clangd ]; then
        echo "- Copying clang"
        mkdir -p "$app_path/Contents/Frameworks" || exit 1
        # use recursive copy to make it copy symlinks as symlinks
        mkdir -p "$libexec_path/clang/bin"
        mkdir -p "$libexec_path/clang/lib"
        cp -Rf "$LLVM_INSTALL_DIR"/lib/clang "$libexec_path/clang/lib/" || exit 1
        cp -Rf "$LLVM_INSTALL_DIR"/lib/ClazyPlugin.dylib "$libexec_path/clang/lib/" || exit 1
        clangdsource="$LLVM_INSTALL_DIR"/bin/clangd
        cp -Rf "$clangdsource" "$libexec_path/clang/bin/" || exit 1
        clangtidysource="$LLVM_INSTALL_DIR"/bin/clang-tidy
        cp -Rf "$clangtidysource" "$libexec_path/clang/bin/" || exit 1
        clangformatsource="$LLVM_INSTALL_DIR"/bin/clang-format
        cp -Rf "$clangformatsource" "$libexec_path/clang/bin/" || exit 1
        clazysource="$LLVM_INSTALL_DIR"/bin/clazy-standalone
        cp -Rf "$clazysource" "$libexec_path/clang/bin/" || exit 1
        install_name_tool -add_rpath "@executable_path/../lib" "$libexec_path/clang/bin/clazy-standalone" 2> /dev/null
        install_name_tool -delete_rpath "/Users/qt/work/build/libclang/lib" "$libexec_path/clang/bin/clazy-standalone" 2> /dev/null
    fi
fi

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
