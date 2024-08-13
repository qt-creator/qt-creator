// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

T.Button {
    id: root
    width: childrenRect.width
    state: "idle"
    property alias linkText: linkTextLabel.text

    Text {
        id: linkTextLabel
        color: "#57b9fc"
        text: qsTr("Screen01.ui.qml - Line 8")
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
    }

    Rectangle {
        id: rectangle
        width: linkTextLabel.width
        height: 1
        color: "#57b9fc"
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
    }
    states: [
        State {
            name: "idle"
            when: !root.hovered && !root.pressed

            PropertyChanges {
                target: rectangle
                visible: false
            }
        },
        State {
            name: "hoverClick"
            when: (root.hovered || root.pressed)

            PropertyChanges {
                target: rectangle
                visible: true
            }
        }
    ]
}
