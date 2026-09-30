// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Item {
    // MARKER: qml line without code
    function compute(value) {
        var doubled = value * 2
        function helper() { return doubled }
        var result = backend.process(helper()) // MARKER: qml breakpoint line
        return result // MARKER: after the native call
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
