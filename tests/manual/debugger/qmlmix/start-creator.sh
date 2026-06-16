#!/bin/sh
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Starts Qt Creator with native combined (C++ + QML) debugging enabled
# and opens this project. Set QTC to use a specific Qt Creator binary;
# defaults to the one built in this source tree, then PATH.

DIR=$(dirname "$(readlink -f "$0")")

QTC=${QTC:-$DIR/../../../../bin/qtcreator}
[ -x "$QTC" ] || QTC=qtcreator

echo "Remember to enable both C++ and QML debugging in" >&2
echo "Projects > Run > Debugger Settings." >&2

exec env QTC_DEBUGGER_NATIVE_MIXED=1 "$QTC" "$DIR/CMakeLists.txt"
