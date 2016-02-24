#! /bin/sh

# Use this script if you add paths to LD_LIBRARY_PATH
# that contain libraries that conflict with the
# libraries that Qt Creator depends on.

makeAbsolute() {
    case $1 in
        /*)
            # already absolute, return it
            echo "$1"
            ;;
        *)
            # relative, prepend $2 made absolute
            echo `makeAbsolute "$2" "$PWD"`/"$1" | sed 's,/\.$,,'
            ;;
    esac
}

me=`which "$0"` # Search $PATH if necessary
if test -L "$me"; then
    # Try readlink(1)
    readlink=`type readlink 2>/dev/null` || readlink=
    if test -n "$readlink"; then
        # We have readlink(1), so we can use it. Assuming GNU readlink (for -f).
        me=`readlink -nf "$me"`
    else
        # No readlink(1), so let's try ls -l
        me=`ls -l "$me" | sed 's/^.*-> //'`
        base=`dirname "$me"`
        me=`makeAbsolute "$me" "$base"`
    fi
fi

bindir=`dirname "$me"`
libdir=`cd "$bindir/../lib" ; pwd`
# Add path to deployed Qt libraries in package
qtlibdir=$libdir/Qt/lib
if test -d "$qtlibdir"; then
    qtlibpath=:$qtlibdir
fi
# Add Qt Creator library path
LD_LIBRARY_PATH=$libdir:$libdir/qtcreator$qtlibpath${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export LD_LIBRARY_PATH
exec "$bindir/qtcreator" ${1+"$@"}
