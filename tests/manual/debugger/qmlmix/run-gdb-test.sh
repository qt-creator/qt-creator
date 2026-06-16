#!/bin/sh
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Runs the scripted check of the service-based native combined
# debugging (see README). Exits non-zero on failure.

DIR=$(dirname "$(readlink -f "$0")")
. "$DIR/ensure-built.sh"

QMLMIX_EXECUTABLE=$QMLMIX_EXECUTABLE \
    exec gdb -batch -x "$DIR/qmlmix-gdb-test.py" 2>/dev/null
