#!/bin/bash

me=$(dirname $0)

for i in $QTDIR/src/declarative/qml/parser/*.{h,cpp,pri}; do
    sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qdeclarativejs/qmljs/)
done

for i in $QTDIR/src/declarative/qml/qdeclarative{error.{h,cpp},dirparser{_p.h,.cpp}}; do
    sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qdeclarative/qml/)
done

# export QmlDirParser
perl -p -0777 -i -e 's/QT_BEGIN_NAMESPACE\n\nclass QmlError;\n\nclass QmlDirParser/#include "qmljsglobal_p.h"\n\nQT_BEGIN_NAMESPACE\n\nclass QmlError;\n\nclass QML_PARSER_EXPORT QmlDirParser/' qmldirparser_p.h
