#!/bin/sh
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Runs the QML breakpoint strategy cost experiment (see README).
# Without arguments all modes are measured in sequence; this takes a
# few minutes because of the gdbtrap modes. Pass one or more mode
# names to run a subset. Needs debug info for libQt6Qml.

DIR=$(dirname "$(readlink -f "$0")")
. "$DIR/ensure-built.sh"

MODES=${*:-baseline gdbtrap_count gdbtrap_decode qt_v4_miss qt_v4_hit}

for mode in $MODES; do
    EXP_MODE=$mode QMLMIX_EXECUTABLE=$QMLMIX_EXECUTABLE \
        gdb -batch -x "$DIR/qmlmix-bp-experiment.py" 2>/dev/null \
        | grep -E 'EXPRESULT|QT_V4|STOPPED|FAILED'
done
