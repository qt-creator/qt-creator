
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
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
        text: qsTr("Target")
        font.pixelSize: 15
    }

    Text {
        id: text111
        x: 80
        y: 25
        color: "red"
        text: backend.targetNode
        font.pixelSize: 15
    }

    TopLevelComboBox {
        id: target
        x: 101
        width: 210
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -335
        model: backend.property.model ?? []
        enabled: false
        //I see no use case to actually change the property name
        //onActivated: backend.targetNode.activateIndex(target.currentIndex)
        property int currentTypeIndex: backend.property.currentIndex ?? 0
        onCurrentTypeIndexChanged: target.currentIndex = target.currentTypeIndex
    }

    Text {
        id: text2
        x: 13
        y: 111
        color: "#ffffff"
        text: qsTr("Source Propety")
        font.pixelSize: 15
    }

    TopLevelComboBox {
        id: sourceNode
        x: 135
        y: 98
        width: 156

        model: backend.sourceNode.model ?? []

        onModelChanged: sourceNode.currentIndex = sourceNode.currentTypeIndex

        onActivated: backend.sourceNode.activateIndex(sourceNode.currentIndex)
        property int currentTypeIndex: backend.sourceNode.currentIndex ?? 0
        onCurrentTypeIndexChanged: sourceNode.currentIndex = sourceNode.currentTypeIndex
    }

    Text {
        x: 13
        y: 88
        color: "#ffffff"
        text: qsTr("Source Node")
        font.pixelSize: 15
    }

    TopLevelComboBox {
        id: sourceProperty
        x: 140
        y: 121
        width: 156

        model: backend.sourceProperty.model ?? []
        onModelChanged: sourceProperty.currentIndex = sourceProperty.currentTypeIndex
        onActivated: backend.sourceProperty.activateIndex(
                         sourceProperty.currentIndex)
        property int currentTypeIndex: backend.sourceProperty.currentIndex ?? 0
        onCurrentTypeIndexChanged: sourceProperty.currentIndex = sourceProperty.currentTypeIndex
    }

    Text {
        id: text3
        x: 10
        y: 55
        color: "#ffffff"
        text: qsTr("Property")
        font.pixelSize: 15
    }
}
