// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Window {
    id: root
    visible: true
    width: 400
    height: 300

    property int counter: 0
    property string label: ""
    property int computed: 0
    property int hotStart: 0
    property int hotResult: 0

    QmlEntryPoint { id: backend }

    function square(x) {
        var result = x * x
        return result
    }

    function compute(n) {
        var values = [n, n + 1, n + 2]
        var note = "compute " + n
        var total = 0
        for (var k = 0; k < values.length; ++k)
            total = total + square(values[k])
        total += backend.process(total)          // MARKER: qml-to-cpp
        return total                             // MARKER: qml-return
    }

    function describe(text, count) {
        var suffix = " #" + count
        return text + suffix
    }

    Timer {
        id: ticker
        interval: 500
        running: true
        repeat: true
        onTriggered: {
            var message = "tick"
            root.counter = root.counter + 1
            root.label = describe(message, root.counter)
        }
    }

    Component.onCompleted: {
        root.hotStart = 1
        var sum = 0
        for (var i = 0; i < 20000; ++i) {
            sum += i
            sum += i
        }
        sum += 1
        root.hotResult = sum

        root.computed = compute(3)
    }
}
