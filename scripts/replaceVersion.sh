#! /usr/bin/env bash

## Command line parameters
if [[ $# != 2 ]]; then
    cat <<USAGE
usage:
  $0 <old> <new>
example:
  $0 '1.2.3' '1.2.4'
USAGE
    exit 1
fi


## Process and show version
OLD=`sed 's/\./\\\\./g' <<<"$1"`
NEW=`sed 's/\./\\\\./g' <<<"$2"`

OLD_MAJOR=`sed 's/^\([0-9]\)\.[0-9]\.[0-9]$/\1/' <<<"$1"`
NEW_MAJOR=`sed 's/^\([0-9]\)\.[0-9]\.[0-9]$/\1/' <<<"$2"`

OLD_MINOR=`sed 's/^[0-9]\.\([0-9]\)\.[0-9]$/\1/' <<<"$1"`
NEW_MINOR=`sed 's/^[0-9]\.\([0-9]\)\.[0-9]$/\1/' <<<"$2"`

OLD_RELEASE=`sed 's/^[0-9]\.[0-9]\.\([0-9]\)$/\1/' <<<"$1"`
NEW_RELEASE=`sed 's/^[0-9]\.[0-9]\.\([0-9]\)$/\1/' <<<"$2"`

OLD_THREE="${OLD_MAJOR}${OLD_MINOR}${OLD_RELEASE}"
NEW_THREE="${NEW_MAJOR}${NEW_MINOR}${NEW_RELEASE}"

OLD_DOT_THREE="${OLD_MAJOR}\\.${OLD_MINOR}\\.${OLD_RELEASE}"
NEW_DOT_THREE="${NEW_MAJOR}\\.${NEW_MINOR}\\.${NEW_RELEASE}"

OLD_DOT_FOUR="${OLD_MAJOR}\\.${OLD_MINOR}\\.${OLD_RELEASE}\\.0"
NEW_DOT_FOUR="${NEW_MAJOR}\\.${NEW_MINOR}\\.${NEW_RELEASE}\\.0"

OLD_COMMA_FOUR="${OLD_MAJOR},${OLD_MINOR},${OLD_RELEASE},0"
NEW_COMMA_FOUR="${NEW_MAJOR},${NEW_MINOR},${NEW_RELEASE},0"

echo "#==============================================="
echo "# Plain    '${OLD}'     -> '${NEW}'"
echo "#-----------------------------------------------"
echo "# Major    '${OLD_MAJOR}'           -> '${NEW_MAJOR}'"
echo "# Minor    '${OLD_MINOR}'           -> '${NEW_MINOR}'"
echo "# Release  '${OLD_RELEASE}'           -> '${NEW_RELEASE}'"
echo "#-----------------------------------------------"
echo "# 3        '${OLD_THREE}'         -> '${NEW_THREE}'"
echo "# Dot 3    '${OLD_DOT_THREE}'     -> '${NEW_DOT_THREE}'"
echo "# Dot 4    '${OLD_DOT_FOUR}'  -> '${NEW_DOT_FOUR}'"
echo "# Comma 4  '${OLD_COMMA_FOUR}'     -> '${NEW_COMMA_FOUR}'"
echo "#==============================================="
echo


## Make script safe to call from anywhere by going home first
SCRIPT_DIR=`dirname "${PWD}/$0"`/..
echo "Entering directory \`${SCRIPT_DIR}'"
pushd "${SCRIPT_DIR}" &>/dev/null || exit 1


## Patch *.pluginspec
while read i ; do
    echo "Patching \`$i'"
    TMPFILE=`mktemp versionPatch.XXXXXX`
    sed -e 's/version="'"${OLD}"'"/version="'"${NEW}"'"/' \
            -e 's/compatVersion="'"${OLD}"'"/compatVersion="'"${NEW}"'"/' \
            "${i}" > "${TMPFILE}"
    mv -f "${TMPFILE}" "${i}"
done < <(find . -name '*.pluginspec')


## Patch coreconstants.h
TMPFILE=`mktemp versionPatch.XXXXXX`
CORE_CONSTANT_H="${SCRIPT_DIR}/src/plugins/coreplugin/coreconstants.h"
echo "Patching \`${CORE_CONSTANT_H}'"
sed \
        -e 's/^\(#define IDE_VERSION_MAJOR \)'"${OLD_MAJOR}"'/\1'"${NEW_MAJOR}"'/' \
        -e 's/^\(#define IDE_VERSION_MINOR \)'"${OLD_MINOR}"'/\1'"${NEW_MINOR}"'/' \
        -e 's/^\(#define IDE_VERSION_RELEASE \)'"${OLD_RELEASE}"'/\1'"${NEW_RELEASE}"'/' \
    "${CORE_CONSTANT_H}" > "${TMPFILE}"
mv -f "${TMPFILE}" "${CORE_CONSTANT_H}"


## Patch installer.rc
TMPFILE=`mktemp versionPatch.XXXXXX`
INSTALLER_RC="${SCRIPT_DIR}/../../dev/ide/nightly_builds/installer/installer.rc"
echo "Patching \`${INSTALLER_RC}'"
sed \
        -e "s/"${OLD_DOT_FOUR}"/"${NEW_DOT_FOUR}"/" \
        -e "s/"${OLD_COMMA_FOUR}"/"${NEW_COMMA_FOUR}"/" \
    "${INSTALLER_RC}" > "${TMPFILE}"
    p4 edit `sed -e 's/\/\.\//\//g' -e 's/\/[^\/]\+\/\.\.\//\//g' <<<"${INSTALLER_RC}"`
mv -f "${TMPFILE}" "${INSTALLER_RC}"


## Patch Info.plist
TMPFILE=`mktemp versionPatch.XXXXXX`
INFO_PLIST="${SCRIPT_DIR}/share/qtcreator/Info.plist"
echo "Patching \`${INFO_PLIST}'"
sed \
        -e "s/"${OLD}"/"${NEW}"/" \
    "${INFO_PLIST}" > "${TMPFILE}"
mv -f "${TMPFILE}" "${INFO_PLIST}"


## Patch qtcreator.qdocconf
TMPFILE=`mktemp versionPatch.XXXXXX`
QDOCCONF="${SCRIPT_DIR}/doc/qtcreator.qdocconf"
echo "Patching \`${QDOCCONF}'"
sed \
        -e "s/"${OLD_DOT_THREE}"/"${NEW_DOT_THREE}"/" \
        -e "s/"${OLD_THREE}"/"${NEW_THREE}"/" \
    "${QDOCCONF}" > "${TMPFILE}"
mv -f "${TMPFILE}" "${QDOCCONF}"


## Patch qtcreator.qdoc
TMPFILE=`mktemp versionPatch.XXXXXX`
QDOC="${SCRIPT_DIR}/doc/qtcreator.qdoc"
echo "Patching \`${QDOC}'"
sed \
        -e 's/'${OLD_DOT_THREE}'/'${NEW_DOT_THREE}'/' \
    "${QDOC}" > "${TMPFILE}"
mv -f "${TMPFILE}" "${QDOC}"


## Go back to original $PWD
echo "Leaving directory \`${SCRIPT_DIR}'"
popd &>/dev/null || exit 1
exit 0
