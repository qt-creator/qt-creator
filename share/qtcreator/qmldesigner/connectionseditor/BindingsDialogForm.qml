
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import StudioControls

Item {
    width: 400
    height: 800

    property var backend

    PopupLabel {
        x: 10
        y: 25
        text: qsTr("Target")
    }

    PopupLabel {
        id: text111
        x: 80
        y: 25
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

    PopupLabel {
        x: 13
        y: 111
        text: qsTr("Source Propety")
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

    PopupLabel {
        x: 13
        y: 88
        text: qsTr("Source Node")
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

    PopupLabel {
        x: 10
        y: 55
        text: qsTr("Property")
    }
}
