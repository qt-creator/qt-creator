#!/bin/sh

############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

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
