#!/bin/bash
################################################################################
# Copyright (C) 2013 Digia Plc
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#   * Neither the name of Digia Plc, nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
################################################################################

if [ $# -lt 1 ]; then
    echo "Usage: $(basename $1) <creator_install_dir> [qmake_path]"
    exit 1
fi

INSTALL_DIR="$1"
QMAKE_BIN="${2:-$(which qmake)}"

if [ ! -e "$QMAKE_BIN" ]; then
    echo "Could not detetermine location of 'qmake'!"
    exit 1;
fi

CHRPATH=$(which chrpath)
if [ ! -e "$CHRPATH" ]; then
    echo "Cannot find required binary 'chrpath'."
    exit 1
fi

QT_INSTALL_LIBS="$($QMAKE_BIN -query QT_INSTALL_LIBS)"
QT_INSTALL_PLUGINS="$($QMAKE_BIN -query QT_INSTALL_PLUGINS)"
QT_INSTALL_IMPORTS="$($QMAKE_BIN -query QT_INSTALL_IMPORTS)"
QT_INSTALL_TRANSLATIONS="$($QMAKE_BIN -query QT_INSTALL_TRANSLATIONS)"

plugins="accessible designer iconengines imageformats sqldrivers"
imports="Qt QtWebKit"
tr_catalogs="assistant designer qt qt_help"
tr_languages="$(cd $INSTALL_DIR/share/qtcreator/translations; echo qtcreator_* | sed -e 's,[^_]*_\([^.]*\)\.,\1 ,g')"

function fix_rpaths()
{
   pushd $INSTALL_DIR/lib
   find qtcreator/ -maxdepth 1 -name "*.so*" -type f -exec $CHRPATH -r \$ORIGIN {} \;
   cd $INSTALL_DIR/lib/qtcreator
   find plugins/ -maxdepth 2 -name "*.so" -type f -exec $CHRPATH -r \$ORIGIN/../.. {} \;

   cd $INSTALL_DIR/bin
   # all executable files in bin
   find  -maxdepth 1 -type f -executable -exec $CHRPATH -r \$ORIGIN/../lib/qtcreator {} \;
   # all lib of imports and plugins one level underneath bin
   find -mindepth 2 -maxdepth 2 -type f -name "*.so"  -exec $CHRPATH -r \$ORIGIN/../../lib/qtcreator {} \;
   find -mindepth 3 -maxdepth 3 -type f -name "*.so"  -exec $CHRPATH -r \$ORIGIN/../../../lib/qtcreator {} \;
   find -mindepth 4 -maxdepth 4 -type f -name "*.so"  -exec $CHRPATH -r \$ORIGIN/../../../../lib/qtcreator {} \;
}

function copy_binaries()
{
    cp -a $QT_INSTALL_LIBS/*.so* $INSTALL_DIR/lib/qtcreator

    for plugin in $plugins; do
        cp -a $QT_INSTALL_PLUGINS/$plugin $INSTALL_DIR/bin
    done

    for import in $imports; do
        cp -a $QT_INSTALL_IMPORTS/$import $INSTALL_DIR/bin
    done
}

function copy_translations()
{
    for language in $tr_languages; do
        for catalog in $tr_catalogs; do
            cp -a $QT_INSTALL_TRANSLATIONS/${catalog}_${language}.qm $INSTALL_DIR/share/qtcreator/translations
        done
    done
}

copy_binaries
copy_translations
fix_rpaths

