#!/bin/sh

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Prepend a copyright header to all files given on the command line.
# Sample usage:
# find . -type f -name \*.cpp -o -name \*.h | \
#     xargs ~/bin/hasCopyright.pl | grep ": No copyright, NOK" | grep "^./src/" | \
#     cut -d ':' -f1 | xargs ~/bin/fixCopyright.sh dist/copyright_template.txt

COPYRIGHT_HEADER=$1

test -f "$COPYRIGHT_HEADER" || exit 16
shift

echo "Using $COPYRIGHT_HEADER..."

WORKDIR=`mktemp -d`
test -d "$WORKDIR" || exit 17

for i in $@ ; do
    echo -n "Fixing $i..."
    if test -f "$i" && test -s "$i" ; then
        BASENAME=`basename "$i"`
        TMP_NAME="$WORKDIR/$BASENAME"
        sed '/./,$!d' "$i" > "$TMP_NAME" # remove leading empty lines
        cat "$COPYRIGHT_HEADER" "$TMP_NAME" > "$i"
        rm "$TMP_NAME"
    fi
    echo done.
done

rmdir "$WORKDIR"
