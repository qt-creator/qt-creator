# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Sourced by the run-*.sh starter scripts: locates the test program,
# building it first if a configured build directory is present.

QMLMIX_EXECUTABLE=${QMLMIX_EXECUTABLE:-$DIR/build/qmlmixtest}

if [ ! -x "$QMLMIX_EXECUTABLE" ] && [ -f "$DIR/build/CMakeCache.txt" ]; then
    cmake --build "$DIR/build" || exit 1
fi

if [ ! -x "$QMLMIX_EXECUTABLE" ]; then
    echo "Test program not found: $QMLMIX_EXECUTABLE" >&2
    echo "Build it with the Qt to be tested:" >&2
    echo "    qt-cmake -S \"$DIR\" -B \"$DIR/build\" && cmake --build \"$DIR/build\"" >&2
    echo "or set QMLMIX_EXECUTABLE." >&2
    exit 1
fi
