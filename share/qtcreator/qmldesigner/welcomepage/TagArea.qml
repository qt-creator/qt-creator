// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0
import QtQuick.Layouts

Item {
    id: root
    height: (repeater.count > root.columnCount) ? root.tagHeight * 2 + root.tagSpacing
                                                : root.tagHeight

    property alias tags: repeater.model
    // private
    property int tagWidth: 75
    property int tagHeight: 25
    property int tagSpacing: 4

    readonly property int columnCount: 3

    Connections {
        target: root
        onWidthChanged: root.tagWidth = Math.floor((root.width - root.tagSpacing
                                                    * (root.columnCount - 1)) / root.columnCount)
    }

    Flow {
        anchors.fill: parent
        spacing: root.tagSpacing

        Repeater {
            id: repeater
            model: ["Qt 6", "Qt for MCU", "3D"]

            Tag {
                text: modelData
                width: root.tagWidth
                height: root.tagHeight
            }
        }
    }
}
