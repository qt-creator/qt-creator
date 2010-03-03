#!/bin/sh

me=$(dirname $0)

for i in $QTDIR/src/declarative/qml/parser/*.{h,cpp,pri}; do
    sed -f $me/cmd.sed $i > $me/$(echo $(basename $i) | sed s/qdeclarativejs/qmljs/)
done


