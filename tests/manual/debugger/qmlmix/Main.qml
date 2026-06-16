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

    Backend { id: backend }

    // Deepest JS frame in the compute() call chain.
    function square(x) {
        var result = x * x                          // MARKER: deep-js
        return result
    }

    // A JS scope with varied locals that also calls into C++.
    function compute(n) {
        var values = [n, n + 1, n + 2]
        var note = "compute " + n
        var total = 0
        for (var k = 0; k < values.length; ++k)
            total = total + square(values[k])       // MARKER: js-call
        total += backend.process(total)             // MARKER: qml-to-cpp
        return total
    }

    // Formats a status string; gives the timer handler a nested JS frame.
    function describe(text, count) {
        var suffix = " #" + count                   // MARKER: describe-js
        return text + suffix
    }

    Timer {
        id: ticker
        interval: 500
        running: true
        repeat: true
        onTriggered: {
            var message = "tick"                            // MARKER: handler-js
            root.counter = root.counter + 1                 // MARKER: handler-write
            root.label = describe(message, root.counter)    // MARKER: handler-call
        }
    }

    Component.onCompleted: {
        // Hot loop for the breakpoint cost experiment; the two property
        // writes delimit it for timing from the outside.
        root.hotStart = 1
        var sum = 0
        for (var i = 0; i < 20000; ++i) {
            sum += i                                // MARKER: hot-loop-body
            sum += i
        }
        sum += 1
        root.hotResult = sum

        // Exercises the nested JS stack and the C++ call once at startup.
        root.computed = compute(3)                  // MARKER: compute-call
    }
}
