#!/bin/sh

# Scan files given on the command line for a copyright header.
# Only the first 15 lines will be examined and must contain the
# string 'Copyright'.
#
# Sample usage:
# find . -type f -name \*.cpp -o -name \*.h | xargs ~/bin/hasCopyright.sh

for i in $@ ; do
    if test -f "$i" && test -s "$i" ; then
        if head -n 15 "$i" | grep Copyright > /dev/null 2>&1 ; then
            echo "$i: Copyright."
        else
           echo "$i: NO COPYRIGHT."
        fi
    fi
done
