// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick.Window 2.0
import QtQuick 2.0

Window {
    id: window
    property Item containedObject: null

    readonly property Item firstChild: window.contentItem.children.length > 0 ? window.contentItem.children[0] : null

    property bool writeGuard: false

    onFirstChildChanged: {
        window.writeGuard = true
        window.containedObject = window.firstChild
        window.writeGuard = false
    }

    onContainedObjectChanged: {
        if (window.writeGuard)
            return

        if (containedObject == undefined || containedObject == null) {
            visible = false
            return
        }

        window.width = containedObject.width
        window.height = containedObject.height

        containedObject.parent = contentItem
        window.visible = true
    }

    Binding {
        target: window.firstChild
        when: window.firstChild
        property: "height"
        value: window.height
    }

    Binding {
        target: window.firstChild
        when: window.firstChild
        property: "width"
        value: window.width
    }
}
