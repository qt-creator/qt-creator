#!/bin/bash
[ $# -lt 5 ] && echo "Usage: $(basename $0) <app folder> <qt translations folder> <qt plugin folder> <qt quick imports folder> <qt quick 2 imports folder>" && exit 2
[ $(uname -s) != "Darwin" ] && echo "Run this script on Mac OS X" && exit 2;

echo "Deploying Qt"

# collect designer plugins
designerDestDir="$1/Contents/PlugIns/designer"
if [ ! -d "$designerDestDir" ]; then
    echo "- Copying designer plugins"
    mkdir -p "$designerDestDir"
    for plugin in "$3"/designer/*.dylib; do
        cp "$plugin" "$designerDestDir"/ || exit 1
    done
fi

# copy Qt Quick 1 imports
importsDir="$1/Contents/Imports/qtquick1"
if [ -d "$4" ]; then
    if [ ! -d "$importsDir" ]; then
        echo "- Copying Qt Quick 1 imports"
        mkdir -p "$importsDir"
        cp -R "$4"/ "$importsDir"/
    fi
fi

# copy Qt Quick 2 imports
imports2Dir="$1/Contents/Imports/qtquick2"
if [ -d "$5" ]; then
    if [ ! -d "$imports2Dir" ]; then
        echo "- Copying Qt Quick 2 imports"
        mkdir -p "$imports2Dir"
        cp -R "$5"/ "$imports2Dir"/
    fi
fi

# copy qt creator qt.conf
if [ ! -f "$1/Contents/Resources/qt.conf" ]; then
    echo "- Copying qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qt.conf" "$1/Contents/Resources/qt.conf" || exit 1
fi

# copy ios tools' qt.conf
if [ ! -f "$1/Contents/Resources/ios/qt.conf" ]; then
    echo "- Copying ios/qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/ios_qt.conf" "$1/Contents/Resources/ios/qt.conf" || exit 1
fi

# copy Qt translations
# check for known existing translation to avoid copying multiple times
if [ ! -f "$1/Contents/Resources/translations/qt_de.qm" ]; then
    echo "- Copying Qt translations"
    cp "$2"/*.qm "$1/Contents/Resources/translations/" || exit 1
fi

# copy libclang if needed
if [ $LLVM_INSTALL_DIR ]; then
    if [ "$LLVM_INSTALL_DIR"/lib/libclang.dylib -nt "$1/Contents/PlugIns"/libclang.dylib ]; then
        echo "- Copying libclang"
        mkdir -p "$1/Contents/Frameworks" || exit 1
        # use recursive copy to make it copy symlinks as symlinks
        cp -Rf "$LLVM_INSTALL_DIR"/lib/libclang.*dylib "$1/Contents/Frameworks/" || exit 1
        cp -Rf "$LLVM_INSTALL_DIR"/lib/clang "$1/Contents/Resources/cplusplus/" || exit 1
        clangsource="$LLVM_INSTALL_DIR"/bin/clang
        clanglinktarget="$(readlink "$clangsource")"
        cp -Rf "$clangsource" "$1/Contents/Resources/" || exit 1
        if [ $clanglinktarget ]; then
            cp -Rf "$(dirname "$clangsource")/$clanglinktarget" "$1/Contents/Resources/$clanglinktarget" || exit 1
        fi
    fi
    _CLANG_CODEMODEL_LIB="$1/Contents/PlugIns/libClangCodeModel_debug.dylib"
    if [ ! -f "$_CLANG_CODEMODEL_LIB" ]; then
        _CLANG_CODEMODEL_LIB="$1/Contents/PlugIns/libClangCodeModel.dylib"
    fi
    # this will just fail when run a second time on libClangCodeModel
    xcrun install_name_tool -delete_rpath "$LLVM_INSTALL_DIR/lib" "$_CLANG_CODEMODEL_LIB" || true
    xcrun install_name_tool -add_rpath "@loader_path/../Frameworks" "$_CLANG_CODEMODEL_LIB" || true
    clangbackendArgument="-executable=$1/Contents/Resources/clangbackend"
fi

#### macdeployqt

if [ ! -d "$1/Contents/Frameworks/QtCore.framework" ]; then

    qml2puppetapp="$1/Contents/Resources/qml2puppet"
    if [ -f "$qml2puppetapp" ]; then
        qml2puppetArgument="-executable=$qml2puppetapp"
    fi

    qbsapp="$1/Contents/MacOS/qbs"

    echo "- Running macdeployqt ($(which macdeployqt))"

    macdeployqt "$1" \
        "-executable=$1/Contents/Resources/qtpromaker" \
        "-executable=$1/Contents/Resources/sdktool" \
        "-executable=$1/Contents/Resources/ios/iostool" \
        "-executable=$1/Contents/Resources/ios/iossim" \
        "-executable=$1/Contents/Resources/ios/iossim_1_8_2" \
        "-executable=$1/Contents/Resources/buildoutputparser" \
        "-executable=$1/Contents/Resources/cpaster" \
        "-executable=$qbsapp" \
        "-executable=$qbsapp-config" \
        "-executable=$qbsapp-config-ui" \
        "-executable=$qbsapp-qmltypes" \
        "-executable=$qbsapp-setup-android" \
        "-executable=$qbsapp-setup-qt" \
        "-executable=$qbsapp-setup-toolchains" \
        "$qml2puppetArgument" "$clangbackendArgument" || exit 1

fi
