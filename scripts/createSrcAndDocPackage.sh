#!/bin/bash

## Command line parameters
if [[ $# != 2 ]]; then
    cat <<USAGE
usage:
  $0 <refspec> <version>

  Creates tar and zip source package from <refspec>.
  Files and directories are named after <version>.
  example:
    $0 origin/2.2 2.2.0
USAGE
    exit 1
fi

BRANCH=$1
VERSION=$2
cd `dirname $0`/..
RESULTDIR=`pwd`
echo "Creating tar archive..."
git archive --format=tar --prefix=qt-creator-${VERSION}-src/ ${BRANCH} | gzip > qt-creator-${VERSION}-src.tar.gz || exit 1

echo "Creating zip archive..."
# untargz the sources for the revision into temporary dir
TEMPSOURCES=`mktemp -d -t qtcCreatorSourcePackage.XXXXXX`
tar xzf qt-creator-${VERSION}-src.tar.gz -C "$TEMPSOURCES" || exit 1
cd $TEMPSOURCES || exit 1
# write filter for text files (for use with 'file' command)
echo ".*:.*ASCII
.*:.*directory
.*:.*empty
.*:.*POSIX
.*:.*html
.*:.*text" > __txtpattern || exit 1
# list all files
find qt-creator-${VERSION}-src > __packagedfiles || exit 1
# record file types
file -f __packagedfiles > __filetypes || exit 1
# zip text files and binary files separately
cat __filetypes | grep -f __txtpattern -v | cut -d: -f1 | zip -9q "$RESULTDIR"/qt-creator-${VERSION}-src.zip -@ || exit 1
cat __filetypes | grep -f __txtpattern    | cut -d: -f1 | zip -l9q "$RESULTDIR"/qt-creator-${VERSION}-src.zip -@ || exit 1
