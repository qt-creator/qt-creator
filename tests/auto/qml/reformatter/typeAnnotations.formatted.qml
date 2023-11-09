
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick

Text {
    function aaa(t: int, k: double): int {
        return 42
    }

    function bbb(aaa): int {
        return 42
    }

    function abc(cba: int) {
        return 42
    }
}
