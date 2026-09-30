// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

QtObject {
    id: root

    property int globalValue: 41
    property string globalMessage: "hi"

    function compute(value) {
        var longLocal = "0123456789".repeat(200) + "LONGTEXTEND"
        var nested = ({ alpha: 1, beta: "two", inner: ({ deep: 7 }) })
        var localObject = ({ payload: 7 })
        var doubled = value * 2 // breakpoint line
        globalValue = value
        reporter.report()
        root.recurse(45)
        return doubled // second breakpoint line
    }

    property bool keepSpinning: true
    function spin() {
        var idle = root.globalValue // spin body line
        if (!root.keepSpinning)
            Qt.quit()
    }
    property Timer spinTimer: Timer {
        interval: 200
        running: true
        repeat: true
        onTriggered: root.spin()
    }

    function recurse(depth) {
        if (depth <= 0)
            return 0 // deep recursion line
        return recurse(depth - 1) + 1 // recursive call line
    }
    property Timer recurseTimer: Timer {
        interval: 3500
        running: true
        repeat: true
        onTriggered: root.recurse(45)
    }

    function throwsError() {
        throw new Error("boom")
    }
    property Timer throwTimer: Timer {
        interval: 4000
        running: true
        repeat: true
        onTriggered: root.throwsError()
    }

    property QtObject orphanHost: null
    property Component orphanComponent: Component {
        id: orphanComponent
        QtObject {
            id: orphanHost
            property QtObject orphan: null
            property Component innerComponent: Component {
                id: innerComponent
                QtObject {
                    id: orphanObject
                    property int orphanValue: 7
                }
            }
            Component.onCompleted: orphanHost.orphan = innerComponent.createObject(null)
        }
    }
    Component.onCompleted: root.orphanHost = orphanComponent.createObject(null)

    property Timer timer: Timer {
        interval: 3000
        running: true
        repeat: true
        onTriggered: root.compute(root.globalValue + 1)
    }
}
