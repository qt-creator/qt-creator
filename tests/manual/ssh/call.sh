#!/bin/sh

dir_name=`dirname $0`
exec=`basename $dir_name`
LD_LIBRARY_PATH=../../../lib/qtcreator:../../../lib/qtcreator/plugins/Nokia:$LD_LIBRARY_PATH  "$dir_name/$exec" "$@"
