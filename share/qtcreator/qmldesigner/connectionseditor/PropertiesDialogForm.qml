// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Controls 2.15

import StudioControls

Rectangle {
    width: 400
    height: 800
    color: "#1b1b1b"
    property var backend

    Text {
        id: text1
        x: 10
        y: 25
        color: "#ffffff"
        text: qsTr("Type:")
        font.pixelSize: 15
    }

    TopLevelComboBox {
        id: target
        x: 95
        width: 210
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -367
        model: backend.type.model ?? []
        onActivated: backend.type.activateIndex(target.currentIndex)
        property int currentTypeIndex: backend.type.currentIndex ?? 0
        onCurrentTypeIndexChanged: target.currentIndex = target.currentTypeIndex
    }

    Text {
        id: text2
        x: 10
        y: 131
        color: "#ffffff"
        text: qsTr("Name")
        font.pixelSize: 15
    }

    TextInput {
        id: name
        x: 70
        y: 131
        color: "white"
        width: 156
        text: backend.name.text ?? ""
        onEditingFinished: {
            backend.name.activateText(name.text)
        }
    }

    Text {
        x: 10
        y: 81
        color: "#ffffff"
        text: qsTr("Value")
        font.pixelSize: 15
    }

    TextInput {
        id: value
        color: "red"
        x: 70
        y: 81
        width: 156
        text: backend.value.text ?? ""
        onEditingFinished: {
            backend.value.activateText(value.text)
        }
    }
}
