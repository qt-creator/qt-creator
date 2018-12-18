#!/bin/sh

# Copyright (C) 2018 The Qt Company Ltd.
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

>&2 echo "<h3>Remounting /sys/kernel/debug/tracing in mode 755 ...</h3><pre>"
mount -o remount,mode=755 /sys/kernel/debug/ || return 1
mount -o remount,mode=755 /sys/kernel/debug/tracing/ || return 1
>&2 echo "done"

register_arguments() {
    REGISTERS="$1"
    shift
    ARGUMENTS=""
    for R in $REGISTERS; do
        test -z "$*" && break;
        ARGUMENTS="$ARGUMENTS $R=$1";
        shift;
    done
    return ARGUMENTS
}

remove_tracepoints() {
    perf probe -d "perfprofiler_$1_$2*"
    return 0
}

match_tracepoints() {
    MACHINE=$1
    NAME=$2

    BASE="perfprofiler_${MACHINE}_${NAME}"
    RETURN=`perf probe -l "${BASE}_ret" | awk '{print $3}'`
    ENTRY=`echo ${RETURN} | awk '{sub(/%return/, ""); print $1}'`
    BAD=`perf probe -l "${BASE}*" | awk '{ if ($3 != "'$RETURN'" && $3 != "'$ENTRY'") { print $1 } }'`
    for PROBE in $BAD; do
        perf probe -d $PROBE
    done
    return 0
}

set_simple_tracepoint() {
    MACHINE="$1"
    shift
    BINARY="$1"
    shift
    NAME="$1"
    shift
    FUNCTION="$1"
    shift
    REGISTERS="$1"
    shift

    ARGUMENTS=""
    for R in ${REGISTERS}; do
        test -z "$*" && break;
        ARGUMENTS="${ARGUMENTS} $1=${R}";
        shift;
    done

    FULL_NAME="perfprofiler_${MACHINE}_${NAME}"
    perf probe -x "${BINARY}" -a "${FULL_NAME}=${FUNCTION} ${ARGUMENTS}" || return 1

    return 0
}

set_symbolic_tracepoint() {
    >&2 echo "</pre><h4>Trying symbolic trace point for $4 on $1 ...</h4><pre>"
    set_simple_tracepoint "$1" "$2" "$3_paramstest" "$4" "\$params" "$5" || return 1
    PARAM=`perf probe -l | awk "/perfprofiler_$1_$3_paramstest"'/{sub(/\)$/, ""); print $(NF)}'`
    perf probe -d "perfprofiler_$1_$3_paramstest"
    set_simple_tracepoint "$1" "$2" "$3" "$4" "$5" "$PARAM" || return 1
    return 0
}

set_register_tracepoint() {
    >&2 echo "</pre><h4>Trying register trace point for $4 on $1 ...</h4><pre>"
    case "$1" in
    aarch64|arm*)
        set_simple_tracepoint "$1" "$2" "$3" "$4" "%r0 %r1 %r2 %r3" $5 || return 1;
        ;;
    x86_64)
        set_simple_tracepoint "$1" "$2" "$3" "$4" "%di %si %dx %cx" $5 || return 1;
        ;;
    *)
        >&2 echo "Register trace points are not supported on $1"
        return 1
        ;;
    esac
    return 0
}

set_tracepoint() {
    if [ -n "$6" ]; then
        >&2 echo "</pre><h4>Trying plain trace point for $4 on $1 ...</h4><pre>"
        set_simple_tracepoint "$1" "$2" "$3" "$4" "$5" "$6" || return 1
    else
        set_register_tracepoint "$1" "$2" "$3" "$4" "$5" || \
            set_symbolic_tracepoint "$1" "$2" "$3" "$4" "$5" || \
            return 1
    fi
    return 0
}

HOST_MACHINE=`uname -m`
find /lib -name libc.so.6 | while read LIBC; do
    echo $LIBC | awk -F '/' '{print $(NF-1)}' | while IFS='-' read MACHINE KERNEL SYSTEM; do
        if [ "$MACHINE" = "lib" ]; then MACHINE=$HOST_MACHINE; fi
        >&2 echo "</pre><h3>Removing old trace points for $MACHINE</h3><pre>"
        remove_tracepoints "$MACHINE" "malloc"
        remove_tracepoints "$MACHINE" "free"
        remove_tracepoints "$MACHINE" "cfree"
        remove_tracepoints "$MACHINE" "realloc"
        remove_tracepoints "$MACHINE" "calloc"
        remove_tracepoints "$MACHINE" "valloc"
        remove_tracepoints "$MACHINE" "pvalloc"
        remove_tracepoints "$MACHINE" "memalign"
        remove_tracepoints "$MACHINE" "aligned_alloc"

        >&2 echo "</pre><h3>Adding trace points for $MACHINE</h3><pre>"
        set_tracepoint "$MACHINE" "$LIBC" "malloc" "malloc" "requested_amount" && \
            set_tracepoint "$MACHINE" "$LIBC" "malloc_ret" "malloc%return" "\$retval" "obtained_id" && \
            set_tracepoint "$MACHINE" "$LIBC" "free" "free" "released_id" && \
            set_tracepoint "$MACHINE" "$LIBC" "cfree" "cfree" "released_id" && \
            set_tracepoint "$MACHINE" "$LIBC" "realloc" "realloc" "released_id requested_amount" && \
            set_tracepoint "$MACHINE" "$LIBC" "realloc_ret" "realloc%return" "\$retval" "moved_id" && \
            set_tracepoint "$MACHINE" "$LIBC" "calloc" "calloc" "requested_blocks requested_amount" && \
            set_tracepoint "$MACHINE" "$LIBC" "calloc_ret" "calloc%return" "\$retval" "obtained_id" && \
            set_tracepoint "$MACHINE" "$LIBC" "valloc" "valloc" "requested_amount" && \
            set_tracepoint "$MACHINE" "$LIBC" "valloc_ret" "valloc%return" "\$retval" "obtained_id" && \
            set_tracepoint "$MACHINE" "$LIBC" "pvalloc" "pvalloc" "requested_amount" && \
            set_tracepoint "$MACHINE" "$LIBC" "pvalloc_ret" "pvalloc%return" "\$retval" "obtained_id" && \
            set_tracepoint "$MACHINE" "$LIBC" "memalign" "memalign" "align requested_amount" && \
            set_tracepoint "$MACHINE" "$LIBC" "memalign_ret" "memalign%return" "\$retval" "obtained_id" && \
            echo $MACHINE || \
            >&2 echo "</pre>!!! Failed to create some trace points for $MACHINE !!!<pre>"

        # aligned_alloc is conditional on C11 and may not be available
        set_tracepoint "$MACHINE" "$LIBC" "aligned_alloc" "aligned_alloc" "align requested_amount" && \
            set_tracepoint "$MACHINE" "$LIBC" "aligned_alloc_ret" "aligned_alloc%return" "\$retval" "obtained_id"

        >&2 echo "</pre><h3>Removing inlined mallocs for $MACHINE</h3><pre>"
        match_tracepoints "$MACHINE" "malloc"
        match_tracepoints "$MACHINE" "realloc"
        match_tracepoints "$MACHINE" "calloc"
        match_tracepoints "$MACHINE" "valloc"
        match_tracepoints "$MACHINE" "pvalloc"
        match_tracepoints "$MACHINE" "memalign"
        match_tracepoints "$MACHINE" "aligned_alloc"
    done
done

>&2 echo "</pre>"
exit
