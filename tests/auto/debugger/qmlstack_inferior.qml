// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Item {
    function compute(value) {
        var doubled = value * 2
        function helper() { return doubled }
        return backend.process(helper()) // MARKER: qml breakpoint line
    }
    function throwsError() {
        throw new Error("boom")
    }
    Timer {
        interval: 1000
        running: true
        onTriggered: throwsError()
    }
    Component.onCompleted: {
        compute(41)
        Qt.callLater(compute, 41)
    }
}
