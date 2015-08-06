#!/bin/sh

################################################################################
# Copyright (C) 2015 The Qt Company Ltd.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#   * Neither the name of The Qt Company Ltd, nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
################################################################################

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

