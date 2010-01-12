#!/bin/sh

# Prepend a copyright header to all files given on the command line.
# Sample usage:
# find . -type f -name \*.cpp -o -name \*.h | \
#     xargs ~/bin/hasCopyright.sh | grep ": NO COPYRIGHT" | grep "^./src/" | \
#     cut -d ':' -f1 | xargs ~/bin/fixCopyright.sh /tmp/copyright.txt

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
