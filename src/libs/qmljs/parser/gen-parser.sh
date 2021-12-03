#!/bin/bash

# This is the script that generates the copy of the QmlJS parser from the sources
# in the qtdeclarative source tree.
#
# It applies a bunch of renames to make the source compatible with the Qt Creator
# sources as well as rewrites of the licenses.
#
# Example:
# cd src/libs/qmljs/parser
# QTDIR=~/path/to/qtdeclarative-checkout ./gen-parser.sh

###
# to update this script:
# 1. do all changes & commit them
# 2. run this script commenting out the two patch commands in the last lines below
# 3. update the first patch using
#         # git diff --cached > grammar.patch
# 4. uncomment the first (grammar) patch, re-run script
# 5. update the second patch with
#         # git diff > parser.patch
# 6. parser.patch needs to be manually edited to remove the patching
#    of non relevant files (gen-parser.sh, grammar.patch, parser.patch, qmljs.g)
# 7. test by running again with the patch commands activated and verify the diffs.
# 8. commit the updated .patch files
###

if [ -z "$QTDIR" -o -z "$QLALR" ]; then
  echo "Usage: QTDIR=~/path/to/qtdeclarative-checkout QLALR=~/path/to/qlalr $0" 1>&2
  exit 1
fi


me=$(dirname $0)

for i in $QTDIR/src/qml/parser/*.{g,h,cpp}; do
    if ! echo $i | grep -q qmljsglobal; then
        sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qqmljs/qmljs/)
    fi
done

for i in $QTDIR/src/qml/qmldirparser/*.{h,cpp}; do
    if ! echo $i | grep -q qmljsglobal; then
        sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qqml/qml/)
    fi
done

for i in $QTDIR/src/qml/common/qqmljs{sourcelocation,memorypool}_p.h; do
    sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qqmljs/qmljs/)
done

# remove qmlapiversion_p.h include
#include "qmlapiversion_p.h"
perl -p -0777 -i -e 's/#include \"qmlapiversion_p.h\"//' qmljsdiagnosticmessage_p.h
# remove qmlglobal_p.h include
perl -p -0777 -i -e 's/#include \"qmlglobal_p.h\"//' qmldirparser.cpp
# remove qmlglobal_p.h include
perl -p -0777 -i -e 's/#include \<QtQml\/qmlfile.h\>//' qmldirparser.cpp
# replace private/qhashedstring_p.h include and QHashedStringRef
perl -p -0777 -i -e 's/#include \<private\/qhashedstring_p.h\>//' qmldirparser_p.h
perl -p -0777 -i -e 's/QHashedStringRef/QString/g' qmldirparser_p.h qmldirparser.cpp
# replace include guards with #pragma once
for i in *.h; do
    grep -q generate $i || perl -p -0777 -i -e 's/#ifndef ([A-Z_]*)\n#define \1\n*([\s\S]*)\n#endif( \/\/ .*)?/#pragma once\n\n\2/' $i
done
# don't use the new QVarLengthArray::length()
sed -i -e 's/chars.length()/chars.size()/' $me/qmljslexer.cpp
sed -i -e 's/DiagnosticMessage::Error/Severity::Error/g' $me/qmljsparser.cpp
sed -i -e 's/DiagnosticMessage::Warning/Severity::Warning/g' $me/qmljsparser.cpp
sed -i -e 's/DiagnosticMessage::Warning/Severity::Warning/g' $me/qmljsparser_p.h
sed -i -e 's|#include <QtCore/qstring.h>|#include <QString>|g' $me/qmljsengine_p.h
sed -i -e 's|#include <QtCore/qset.h>|#include <QSet>|g' $me/qmljsengine_p.h
perl -p -0777 -i -e 's/QT_QML_BEGIN_NAMESPACE/#include <qmljs\/qmljsconstants.h>\nQT_QML_BEGIN_NAMESPACE/' qmljsengine_p.h

./changeLicense.py $me/../qmljs_global.h qml*.{cpp,h}

git add -u
git clang-format
git add -u
git reset gen-parser.sh grammar.patch qmljsgrammar.cpp qmljsgrammar_p.h qmljsparser.cpp qmljsparser_p.h
## comment from here to update grammar.patch using
## git diff --cached > grammar.patch
patch -R -p5 < grammar.patch
$QLALR qmljs.g

./changeLicense.py $me/../qmljs_global.h qml*.{cpp,h}

git add -u
git clang-format
git add -u
git reset gen-parser.sh grammar.patch parser.patch
## comment from here to update parser.patch
## git diff --cached > parser.patch
patch -p5 -R < parser.patch
git reset
