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

# --- helpers -----------------------------------------------------------------

printUsage()
{
    echo "Usage: $0 [-v] [-c clang-executable] file line column"
    echo
    echo "Options:"
    echo "  -v                   Run c-index-test instead of clang to get more details."
    echo "  -c clang-executable  Use the provided clang-executable."
    echo
    echo "The clang executable is determined by this order:"
    echo "  1. Use clang from the -c option."
    echo "  2. Use clang from environment variable CLANG_FOR_COMPLETION."
    echo "  3. Use clang from \$PATH."
    echo
    echo "Path to c-index-test will be determined with the help of the clang executable."
}

# There is no readlink on cygwin.
hasReadLink()
{
    return $(command -v readlink >/dev/null 2>&1)
}

checkIfFileExistsOrDie()
{
    [ ! -f "$1" ] && echo "'$1' is not a file or does not exist." && exit 1
}

checkIfFileExistsAndIsExecutableOrDie()
{
    checkIfFileExistsOrDie "$1"
    [ ! -x "$1" ] && echo "'$1' is not executable." && exit 2
}

findClangOrDie()
{
    if [ -z "$CLANG_EXEC" ]; then
        if [ -n "${CLANG_FOR_COMPLETION}" ]; then
            CLANG_EXEC=${CLANG_FOR_COMPLETION}
        else
            CLANG_EXEC=$(which clang)
        fi
    fi
    hasReadLink && CLANG_EXEC=$(readlink -e "$CLANG_EXEC")
    checkIfFileExistsAndIsExecutableOrDie "$CLANG_EXEC"
}

findCIndexTestOrDie()
{
    if [ -n "$RUN_WITH_CINDEXTEST" ]; then
        CINDEXTEST_EXEC=$(echo $CLANG_EXEC | sed -e 's/clang/c-index-test/g')
        hasReadLink && CINDEXTEST_EXEC=$(readlink -e "$CINDEXTEST_EXEC")
        checkIfFileExistsAndIsExecutableOrDie "$CINDEXTEST_EXEC"
    fi
}

printClangVersion()
{
    command="${CLANG_EXEC} --version"
    echo "Command: $command"
    eval $command
}

runCodeCompletion()
{
    if [ -n "${CINDEXTEST_EXEC}" ]; then
        command="${CINDEXTEST_EXEC} -code-completion-at=${FILE}:${LINE}:${COLUMN} ${FILE}"
    else
        command="${CLANG_EXEC} -cc1 -code-completion-at ${FILE}:${LINE}:${COLUMN} ${FILE}"
    fi
    echo "Command: $command"
    eval $command
}

# --- Process arguments -------------------------------------------------------

CLANG_EXEC=
RUN_WITH_CINDEXTEST=

FILE=
LINE=
COLUMN=

while [ -n "$1" ]; do
    param=$1
    shift
    case $param in
        -h | -help | --help)
            printUsage
            exit 0
            ;;
        -v | -verbose | --verbose)
            RUN_WITH_CINDEXTEST=1
            ;;
        -c | -clang | --clang)
            CLANG_EXEC=$1
            shift
            ;;
        *)
            break;
            ;;
    esac
done

[ "$#" -ne 2 ] && printUsage && exit 1
checkIfFileExistsOrDie "$param"
FILE=$param
LINE=$1
COLUMN=$2

# --- main --------------------------------------------------------------------

findClangOrDie
findCIndexTestOrDie

printClangVersion
echo
runCodeCompletion

