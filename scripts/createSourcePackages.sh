#!/bin/bash

## Command line parameters
if [[ $# != 2 ]]; then
    cat <<USAGE
usage:
  $0 <version> <edition>

  Creates tar and zip source package from HEAD of the main repository and submodules.
  Files and directories are named after qt-creator-<edition>-src-<version>.
  example:
    $0 2.2.0-beta opensource
USAGE
    exit 1
fi

VERSION=$1
EDITION=$2
PREFIX=qt-creator-${EDITION}-src-${VERSION}
cd `dirname $0`/..
RESULTDIR=`pwd`
TEMPSOURCES=`mktemp -d -t qtcCreatorSourcePackage.XXXXXX`
echo "Temporary directory: ${TEMPSOURCES}"
echo "Creating tar archive..."

echo "  Creating tar sources of repositories..."
git archive --format=tar --prefix=${PREFIX}/ HEAD > ${TEMPSOURCES}/__qtcreator_main.tar || exit 1
echo "    qbs..."
cd src/shared/qbs || exit 1
git archive --format=tar --prefix=${PREFIX}/src/shared/qbs/ HEAD > ${TEMPSOURCES}/__qtcreator_qbs.tar || exit 1

echo "  Combining tar sources..."
cd ${TEMPSOURCES} || exit 1
tar xf __qtcreator_main.tar || exit 1
tar xf __qtcreator_qbs.tar || exit 1
tar czf "${RESULTDIR}/${PREFIX}.tar.gz" ${PREFIX}/ || exit 1

echo "Creating zip archive..."
echo "  Filtering binary vs text files..."
# write filter for text files (for use with 'file' command)
echo ".*:.*ASCII
.*:.*directory
.*:.*empty
.*:.*POSIX
.*:.*html
.*:.*text" > __txtpattern || exit 1
# list all files
find ${PREFIX} > __packagedfiles || exit 1
# record file types
file -f __packagedfiles > __filetypes || exit 1
echo "  Creating archive..."
# zip text files and binary files separately
cat __filetypes | grep -f __txtpattern -v | cut -d: -f1 | zip -9q "${RESULTDIR}/${PREFIX}.zip" -@ || exit 1
cat __filetypes | grep -f __txtpattern    | cut -d: -f1 | zip -l9q "${RESULTDIR}/${PREFIX}.zip" -@ || exit 1
