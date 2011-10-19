#!/bin/sh

# Scan files given on the command line for a copyright header.
# Only the first 15 lines will be examined and must contain the
# string 'Copyright'.
#
# Sample usage:
# find . -type f -name \*.cpp -o -name \*.h | xargs ~/bin/hasCopyright.sh

for i in "$@" ; do
    if test -f "$i" && test -s "$i" ; then
        if head -n 35 "$1" | grep "qt-info@nokia.com" > /dev/null 2>&1 ; then
             echo "$i: OLD EMAIL IN USE!"
        elif head -n 35 "$i" | grep Copyright > /dev/null 2>&1 ; then
            if head -n 35 "$i" | grep "GNU Lesser General Public License" > /dev/null 2>&1 &&
               head -n 35 "$i" | grep "Other Usage" > /dev/null 2>&1 ; then
                echo "$i: Copyright ok"
            else
                echo "$i: WRONG COPYRIGHT"
            fi
        else
           echo "$i: NO COPYRIGHT"
        fi
    fi
done
