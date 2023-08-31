// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Controls 2.15

import StudioControls

Item {
    width: 400
    height: 800

    property var backend

    PopupLabel {
        x: 10
        y: 25

        text: qsTr("Type:")

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

    PopupLabel {
        x: 10
        y: 131
        text: qsTr("Name")
    }

    TextInput {
        id: name
        x: 70
        y: 131
        width: 156
        text: backend.name.text ?? ""
        onEditingFinished: {
            backend.name.activateText(name.text)
        }
    }

    PopupLabel {
        x: 10
        y: 81
        text: qsTr("Value")
    }

    TextInput {
        id: value
        x: 70
        y: 81
        width: 156
        text: backend.value.text ?? ""
        onEditingFinished: {
            backend.value.activateText(value.text)
        }
    }
}
