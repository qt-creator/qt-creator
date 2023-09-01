
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Column {
    id: root

    readonly property real horizontalSpacing: 10
    readonly property real verticalSpacing: 16
    readonly property real columnWidth: (root.width - root.horizontalSpacing) / 2

    property var backend

    y: StudioTheme.Values.popupMargin
    width: parent.width
    spacing: root.verticalSpacing

    Row {
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("From") ; tooltip: qsTr("The Property to assign from.")}
        PopupLabel { text: qsTr("To"); tooltip: qsTr("The Property to assign to.") }
    }

    Row {
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: sourceNode
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth

            model: backend.sourceNode.model ?? []
            onModelChanged: sourceNode.currentIndex = sourceNode.currentTypeIndex
            onActivated: backend.sourceNode.activateIndex(sourceNode.currentIndex)
            property int currentTypeIndex: backend.sourceNode.currentIndex ?? 0
            onCurrentTypeIndexChanged: sourceNode.currentIndex = sourceNode.currentTypeIndex
        }

        PopupLabel {
             width: root.columnWidth
             text: backend.targetNode
        }

    }

    Row {
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: sourceProperty
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth

            model: backend.sourceProperty.model ?? []
            onModelChanged: sourceProperty.currentIndex = sourceProperty.currentTypeIndex
            onActivated: backend.sourceProperty.activateIndex(
                             sourceProperty.currentIndex)
            property int currentTypeIndex: backend.sourceProperty.currentIndex ?? 0
            onCurrentTypeIndexChanged: sourceProperty.currentIndex = sourceProperty.currentTypeIndex
        }

        PopupLabel {
             width: root.columnWidth
             text: backend.property.currentText
        }

    }
}
