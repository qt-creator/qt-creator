// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

QtObject {
    id: root

    property int globalValue: 41

    function compute(value) {
        var nested = ({ alpha: 1, beta: "two", inner: ({ deep: 7 }) })
        var doubled = value * 2 // breakpoint line
        globalValue = value
        return doubled // second breakpoint line
    }

    function throwsError() {
        throw new Error("boom")
    }
    property Timer throwTimer: Timer {
        interval: 4000
        running: true
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
        onTriggered: root.compute(root.globalValue + 1)
    }
}
