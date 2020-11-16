#!/bin/bash
#
# SPDX-FileCopyrightText: 2012-2013 Alex Turbov 
#
# Grab a documented (officially) class list from Qt project web site:
# http://qt-project.org/doc/qt-${version}/classes.html
#

version=$1
shift

case "$version" in
5*)
    url="http://qt-project.org/doc/qt-${version}/qtdoc/classes.html"
    ;;
4*)
    url="http://qt-project.org/doc/qt-${version}/classes.html"
    ;;
*)
    echo "*** Error: Only Qt4 and Qt5 supported!"
esac

if [ -n "$version" ]; then
  tmp=`mktemp`
  wget -O $tmp "$url"
  cat $tmp | egrep '^<dd><a href=".*\.html">.*</a></dd>$' \
    | sed -e 's,<dd><a href=".*\.html">\(.*\)</a></dd>,<item> \1 </item>,' \
    | grep -v 'qoutputrange'
  rm $tmp
else
  cat <<EOF
Usage:
  $0 Qt-version

Note: Only major and minor version required

Example:
  $0 4.8
EOF
fi
