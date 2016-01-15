#! /usr/bin/env bash

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

WORKER=./fill_deps.sh
DEPFILE=deps

write_deps_file() {
    INPUT=$1
    ESCAPED_OUTPUT=`sed 's/\//\\\\\//g' <<<"$1"`

    {
	    echo '#! /usr/bin/env bash'
	    grep '^\(CXXFLAGS\|INCPATH\|DEFINES\)' ${INPUT} \
		    | sed \
			    -e 's/$(\([^)]\+\))/${\1}/g' \
			    -e 's/"/\\"/g' \
                -e 's/^\([^ ]\+\) *= *\(.\+\)/\1="\2"/'
	    echo
        echo 'touch deps'
	    grep '^.$(CXX)' ${INPUT} \
		    | grep -v '$@' \
		    | sed \
			    -e 's/^\t\$(CXX)\(.\+\)$/makedepend \1 -w 1000000 -f '${DEPFILE}' -p "" -a -o .o 2>\/dev\/null/' \
			    -e 's/$(\([^)]\+\))/${\1}/g' \
			    -e 's/\\/\//g'
    } > "${WORKER}"
    chmod a+x "${WORKER}"
}



PWD_BACKUP=$PWD

while read makefile ; do
    dir=`dirname "${makefile}"`
    if [ `find "${dir}" -maxdepth 1 -name '*.cpp' | wc -l 2>/dev/null` -ge 1 ]; then
        echo "Directory: $dir"
        cd $dir
        rm -f "${DEPFILE}"
        write_deps_file Makefile
        "${WORKER}"
        TEMPFILE=`mktemp`
        sed 's/^.\+\/\([^\/]\+.o:\)/\1/' "${DEPFILE}" > "${TEMPFILE}"
        mv "${TEMPFILE}" "${DEPFILE}"
        # rm "${WORKER}"

        cd ${PWD_BACKUP}
        echo "include ${DEPFILE}" >> "${makefile}"
    fi
done < <(find src -name 'Makefile')

cd ${PWD_BACKUP}

