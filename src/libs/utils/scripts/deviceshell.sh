# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
FINAL_OUT=$(mktemp -u) || exit 2
mkfifo "$FINAL_OUT" || exit 3

finalOutput() {
    local fileInputBuffer
    while read fileInputBuffer
    do
        if test -f "$fileInputBuffer.err"; then
            cat $fileInputBuffer.err
        fi
        cat $fileInputBuffer
        rm -f $fileInputBuffer.err $fileInputBuffer
    done
}

finalOutput < $FINAL_OUT &

readAndMark() {
    local buffer
    while read buffer
    do
        printf '%s:%s:%s\n' "$1" "$2" "$buffer"
    done
}

base64decode()
{
    base64 -d 2>/dev/null
}

base64encode()
{
    base64 2>/dev/null
}

executeAndMark()
{
    PID="$1"
    INDATA="$2"
    shift
    shift
    CMD="$@"

    # LogFile
    TMPFILE=$(mktemp)

    # Output Streams
    stdoutenc=$(mktemp -u)
    stderrenc=$(mktemp -u)
    mkfifo "$stdoutenc" "$stderrenc"

    # app output streams
    stdoutraw=$(mktemp -u)
    stderrraw=$(mktemp -u)
    mkfifo "$stdoutraw" "$stderrraw"

    # Cleanup
    trap 'rm -f "$stdoutenc" "$stderrenc" "$stdoutraw" "$stderrraw"' EXIT

    # Pipe all app output through base64, and then into the output streams
    cat $stdoutraw | base64encode > "$stdoutenc" &
    cat $stderrraw | base64encode > "$stderrenc" &

    # Mark the app's output streams
    readAndMark $PID 'O' < "$stdoutenc" >> $TMPFILE &
    readAndMark $PID 'E' < "$stderrenc" >> $TMPFILE.err &

    # Start the app ...
    if [ -z "$INDATA" ]
    then
        eval $CMD 1> "$stdoutraw" 2> "$stderrraw"
    else
        echo $INDATA | base64decode | eval "$CMD" 1> "$stdoutraw" 2> "$stderrraw"
    fi

    exitcode=$(echo $? | base64encode)
    wait
    echo "$PID:R:$exitcode" >> $TMPFILE
    echo $TMPFILE
}

execute()
{
    PID="$1"

    if [ "$#" -lt "3" ]; then
        TMPFILE=$(mktemp)
        echo "$PID:R:MjU1Cg==" > $TMPFILE
        echo $TMPFILE
    else
        INDATA=$(eval echo "$2")
        shift
        shift
        CMD=$@
        executeAndMark $PID "$INDATA" "$CMD"
    fi
}

cleanup()
{
    kill -- -$$
    exit 1
}

trap cleanup 1 2 3 6

echo SCRIPT_INSTALLED >&2

(while read -r id inData cmd; do
    if [ "$id" = "exit" ]; then
        exit
    fi
    execute $id $inData $cmd || echo "$id:R:255" &
done) > $FINAL_OUT
