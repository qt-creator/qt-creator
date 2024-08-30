// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick3D

Node {
    id: textLabelNode

    Rectangle {
        id: embeddedRect
        width: embeddedText.width
        height: embeddedText.height
        x: -(width / 2)
        y: -(height / 2)
        color: "#ffffff"
        Text {
            id: embeddedText
            text: qsTr("3D Text")
            font.pixelSize: 50
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            padding: 5
        }
    }
}
