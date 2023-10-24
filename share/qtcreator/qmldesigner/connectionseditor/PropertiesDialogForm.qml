// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Column {
    id: root

    readonly property real horizontalSpacing: 10
    readonly property real verticalSpacing: 12
    readonly property real columnWidth: (root.width - root.horizontalSpacing) / 2

    property var backend

    width: parent.width
    spacing: root.verticalSpacing

    PopupLabel {
        text: qsTr("Type")
        tooltip: qsTr("Sets the category of the <b>Local Custom Property</b>.")
    }

    StudioControls.TopLevelComboBox {
        id: type
        style: StudioTheme.Values.connectionPopupControlStyle
        width: root.columnWidth

        model: backend.type.model ?? []
        onActivated: backend.type.activateIndex(type.currentIndex)
        property int currentTypeIndex: backend.type.currentIndex ?? 0
        onCurrentTypeIndexChanged: type.currentIndex = type.currentTypeIndex
    }

    Row {
        spacing: root.horizontalSpacing

        PopupLabel {
            width: root.columnWidth
            text: qsTr("Name")
            tooltip: qsTr("Sets a name for the <b>Local Custom Property</b>.")
        }

        PopupLabel {
            width: root.columnWidth
            text: qsTr("Value")
            tooltip: qsTr("Sets a valid <b>Local Custom Property</b> value.")
        }
    }

    Row {
        spacing: root.horizontalSpacing

        StudioControls.TextField {
            id: name

            width: root.columnWidth
            actionIndicatorVisible: false
            translationIndicatorVisible: false

            text: backend.name.text ?? ""
            onEditingFinished: backend.name.activateText(name.text)
        }

        StudioControls.TextField {
            id: value

            width: root.columnWidth
            actionIndicatorVisible: false
            translationIndicatorVisible: false


            text: backend.value.text ?? ""
            onEditingFinished: backend.value.activateText(value.text)
        }
    }
}
