#! /usr/bin/env bash

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

