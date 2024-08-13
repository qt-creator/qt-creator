// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

T.Button {
    id: root
    width: 100
    height: 29
    property alias labelText: label.text
    state: "idle"
    checkable: true

    Rectangle {
        id: background
        color: "#282828"
        radius: 5
        border.color: "#424242"
        anchors.fill: parent
    }

    Text {
        id: label
        color: "#ffffff"
        text: qsTr("Issues")
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: 12
        anchors.horizontalCenter: parent.horizontalCenter
    }
    states: [
        State {
            name: "idle"
            when: !root.hovered && !root.pressed & !root.checked
        },
        State {
            name: "hover"
            when: root.hovered && !root.pressed & !root.checked

            PropertyChanges {
                target: background
                color: "#2f2f2f"
            }
        },
        State {
            name: "pressCheck"
            when: (root.pressed || root.checked)

            PropertyChanges {
                target: background
                color: "#57b9fc"
                border.color: "#57b9fc"
            }
        }
    ]
}
