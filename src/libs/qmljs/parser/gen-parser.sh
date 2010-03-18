#!/bin/bash

me=$(dirname $0)

for i in $QTDIR/src/declarative/qml/parser/*.{h,cpp,pri}; do
    sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qdeclarativejs/qmljs/)
done

for i in $QTDIR/src/declarative/qml/qdeclarative{error.{h,cpp},dirparser{_p.h,.cpp}}; do
    sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qdeclarative/qml/)
done

