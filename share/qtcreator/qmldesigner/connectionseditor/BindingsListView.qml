// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme
import ConnectionsEditorEditorBackend

ListView {
    id: root

    property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

    clip: true
    interactive: true
    highlightMoveDuration: 0
    highlightResizeDuration: 0

    onVisibleChanged: {
        dialog.hide()
    }

    property int modelCurrentIndex: root.model.currentIndex ?? 0

    // Something weird with currentIndex happens when items are removed added.
    // listView.model.currentIndex contains the persistent index.

    onModelCurrentIndexChanged: {
        root.currentIndex = root.model.currentIndex
    }

    onCurrentIndexChanged: {
        root.currentIndex = root.model.currentIndex
        dialog.backend.currentRow = root.currentIndex
    }

    // Number of columns
    readonly property int numColumns: 4
    // Proper row width calculation
    readonly property int rowSpacing: StudioTheme.Values.toolbarHorizontalMargin
    readonly property int rowSpace: root.width - (root.rowSpacing * (root.numColumns + 1))
                                    - root.style.squareControlSize.width
    property int rowWidth: root.rowSpace / root.numColumns
    property int rowRest: root.rowSpace % root.numColumns

    data: [
        BindingsDialog {
            id: dialog
            visible: false
            backend: root.model.delegate
        }
    ]

    delegate: Rectangle {
        id: itemDelegate

        required property int index

        required property string target
        required property string targetProperty
        required property string source
        required property string sourceProperty

        width: ListView.view.width
        height: root.style.squareControlSize.height
        color: mouseArea.containsMouse ?
                   itemDelegate.ListView.isCurrentItem ? root.style.interactionHover
                                                       : root.style.background.hover
                                       : "transparent"

        MouseArea {
            id: mouseArea

            anchors.fill: parent
            hoverEnabled: true

            onClicked: {
                root.model.currentIndex = itemDelegate.index
                root.currentIndex = itemDelegate.index
                dialog.backend.currentRow = itemDelegate.index
                dialog.popup(mouseArea)
            }
        }

        Row {
            id: row

            property color textColor: itemDelegate.ListView.isCurrentItem ? root.style.text.selectedText
                                                                          : root.style.icon.idle

            height: itemDelegate.height
            spacing: root.rowSpacing

            Text {
                width: root.rowWidth + root.rowRest
                height: itemDelegate.height
                color: row.textColor
                text: itemDelegate.target ?? ""
                verticalAlignment: Text.AlignVCenter
                font.bold: false
                elide: Text.ElideMiddle
                leftPadding: root.rowSpacing
            }

            Text {
                width: root.rowWidth
                height: itemDelegate.height
                text: itemDelegate.targetProperty ?? ""
                color: row.textColor
                verticalAlignment: Text.AlignVCenter
                font.bold: false
                elide: Text.ElideMiddle
            }

            Text {
                width: root.rowWidth
                height: itemDelegate.height
                text: itemDelegate.source ?? ""
                verticalAlignment: Text.AlignVCenter
                color: row.textColor
                font.bold: false
                elide: Text.ElideMiddle
            }

            Text {
                width: root.rowWidth
                height: itemDelegate.height
                text: itemDelegate.sourceProperty ?? ""
                verticalAlignment: Text.AlignVCenter
                color: row.textColor
                font.bold: false
                elide: Text.ElideMiddle
            }

            Rectangle {
                width: root.style.squareControlSize.width
                height: root.style.squareControlSize.height

                color: toolTipArea.containsMouse ?
                           itemDelegate.ListView.isCurrentItem ? root.style.interactionHover
                                                               : root.style.background.hover
                                               : "transparent"

                Text {
                    anchors.fill: parent

                    text: StudioTheme.Constants.remove_medium
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: root.style.baseIconFontSize

                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter

                    color: row.textColor
                    renderType: Text.QtRendering
                }

                HelperWidgets.ToolTipArea {
                    id: toolTipArea
                    tooltip: qsTr("This is a test.")
                    anchors.fill: parent
                    onClicked: root.model.remove(itemDelegate.index)
                }
            }
        }
    }

    highlight: Rectangle {
        color: root.style.interaction
        width: 600
    }
}
