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

me=$(dirname $0)

for i in $QTDIR/src/qml/qml/parser/*.{g,h,cpp,pri}; do
    sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qqmljs/qmljs/)
done

for i in $QTDIR/src/qml/qml/qqml{error.{h,cpp},dirparser{_p.h,.cpp}}; do
    sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qqml/qml/)
done

# export QmlDirParser
perl -p -0777 -i -e 's/QT_BEGIN_NAMESPACE\n\nclass QmlError;\nclass QmlEngine;\nclass Q_AUTOTEST_EXPORT QmlDirParser/#include "qmljsglobal_p.h"\n\nQT_BEGIN_NAMESPACE\n\nclass QmlError;\nclass QmlEngine;\nclass QML_PARSER_EXPORT QmlDirParser/' qmldirparser_p.h
# export QmlJSGrammar
perl -p -0777 -i -e 's/#include <QtCore\/qglobal.h>\n\nQT_BEGIN_NAMESPACE\n\nclass QmlJSGrammar\n/#include "qmljsglobal_p.h"\n#include <QtCore\/qglobal.h>\n\nQT_BEGIN_NAMESPACE\n\nclass QML_PARSER_EXPORT QmlJSGrammar\n/' qmljsgrammar_p.h
# remove qmlglobal_p.h include
perl -p -0777 -i -e 's/#include \"qmlglobal_p.h\"//' qmldirparser.cpp
# remove qmlglobal_p.h include
perl -p -0777 -i -e 's/#include \"qmlglobal_p.h\"//' qmlerror.cpp
# remove qmlglobal_p.h include
perl -p -0777 -i -e 's/#include \<QtQml\/qmlfile.h\>//' qmldirparser.cpp
# remove QtQml/qtqmlglobal.h include
perl -p -0777 -i -e 's/#include \<QtQml\/qtqmlglobal.h\>//' qmlerror.h
# replace private/qhashedstring_p.h include and QHashedStringRef
perl -p -0777 -i -e 's/#include \<private\/qhashedstring_p.h\>//' qmldirparser_p.h
perl -p -0777 -i -e 's/QHashedStringRef/QString/g' qmldirparser_p.h qmldirparser.cpp
# don't use the new QVarLengthArray::length()
sed -i -e 's/chars.length()/chars.size()/' $me/qmljslexer.cpp

./changeLicense.py $me/../qmljs_global.h qml*.{cpp,h}

patch -p5 < parser.patch
