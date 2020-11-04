#!/bin/bash
#
# SPDX-FileCopyrightText: 2011-2012 Alex Turbov 
#
# Simplest (and stupid) way to get #defines from a header file(s)
#
# TODO Think about to use clang to get (an actual) list of free functions and/or types, classes, etc
#      Using python bindings it seems possible and not so hard to code...
#

basepath=$1
shift

if [ -n "$*" ]; then
  for f in $*; do
    egrep '^\s*#\s*define\s+(Q|QT|QT3)_' ${basepath}/$f
  done \
    | sed 's,^\s*#\s*define\s\+\(Q[A-Z0-9_]\+\).*,<item> \1 </item>,' \
    | sort \
    | uniq \
    | grep -v EXPORT \
    | grep -v QT_BEGIN_ \
    | grep -v QT_END_ \
    | grep -v QT_MANGLE_
else
  cat <<EOF
Usage:
  $0 basepath [qt-header-filenames]

Example:
  $0 /usr/include/qt4/Qt qglobal.h qconfig.h qfeatures.h
EOF
fi
