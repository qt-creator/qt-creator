#!/usr/bin/env bash

version=4.4.3
workdir=/home/berlin/dev/qt-${version}-temp
destdir=/home/berlin/dev/qt-${version}-shipping
dir=qt-x11-opensource-src-${version}
file_tar="${dir}.tar"
file_tar_gz="${file_tar}.gz"
[ -z ${MAKE} ] && MAKE=make
envpath=/usr/bin:/bin

if gcc -dumpversion | grep '^4' ; then
	# GCC 4.x machine
	webkit=
else
	# GCC 3.3.5 machine
	webkit='-no-webkit'
fi


die() {
	echo $1 1>&2
	exit 1
}

rand_range() {
    incMin=$1
    incMax=$2
    echo $((RANDOM*(incMax-incMin+1)/32768+incMin))
}


setup() {
	mkdir -p "${workdir}"
	cd "${workdir}" || die "cd failed"
}

download() {
	[ -f "${file_tar_gz}" ] && return
	case `rand_range 1 2` in
	1)
		mirror=http://ftp.ntua.gr/pub/X11/Qt/qt/source
		;;
	*)
		mirror=http://wftp.tu-chemnitz.de/pub/Qt/qt/source
		;;
	esac
	wget "${mirror}/${file_tar_gz}" || die "Download failed"
}

unpack() {
	[ -d "${dir}" ] && return
	gzip -d "${file_tar_gz}" || die "gunzip failed"
	tar -xf "${file_tar}" || die "untar failed"
}

build() {
	(
		cd "${dir}" || die "cd failed"
		if [ ! -f config.status ] ; then
			env -i PATH=${envpath} ./configure \
				-prefix "${destdir}" \
				-optimized-qmake \
				-confirm-license \
				\
				-no-mmx -no-sse -no-sse2 -no-3dnow \
				-release -fast \
				${webkit} \
				-qt-zlib -qt-gif -qt-libtiff -qt-libpng -qt-libmng -qt-libjpeg \
				\
				|| die "configure failed"
		fi
	
		env -i PATH=${envpath} "${MAKE}" || die "make failed"
	)
	ret=$?; [ ${ret} = 0 ] || exit ${ret}
}

fix_rpath() {
	folder=$1
	pattern=$2
	rpath=$3
	(
		cd "${destdir}" || die "cd failed"
		while read file ; do
			echo "Fixing ${file}"
			chrpath -r "${rpath}" "${file}" 2>&1 | sed 's/^/\t/'
		done < <(find "${folder}" -type f -name "${pattern}")
	)
	ret=$?; [ ${ret} = 0 ] || exit ${ret}
}

inst() {
	(
		cd "${dir}" || die "cd failed"
		if [ ! -d "${destdir}" ]; then
			mkdir -p "${destdir}"
			env -i "${MAKE}" install || die "make install failed"
		fi

		cd "${destdir}" || die "cd failed"

		# Fix files bin/*
		fix_rpath bin '*' '$ORIGIN/../lib'

		# Fix files lib/*.so
		fix_rpath lib '*.so.?.?.?' '$ORIGIN'
		fix_rpath lib '*.so.?.?.?.debug' '$ORIGIN'

		# Fix files examples/tools/*/*/*.so
		fix_rpath examples/tools '*.so' '$ORIGIN/../../../lib'
		fix_rpath examples/tools '*.so.debug' '$ORIGIN/../../../lib'

		# Fix files plugins/*/*.so
		fix_rpath plugins '*.so' '$ORIGIN/../../lib'
		fix_rpath plugins '*.so.debug' '$ORIGIN/../../lib'
	)
	ret=$?; [ ${ret} = 0 ] || exit ${ret}
}

main() {
	(
		setup
		download
		unpack
		build
		inst
	)
	ret=$?; [ ${ret} = 0 ] || exit ${ret}
}

main
