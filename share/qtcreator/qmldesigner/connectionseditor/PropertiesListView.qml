// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme
import ConnectionsEditorEditorBackend

ListView {
    id: listView
    width: 606
    height: 160
    clip: true
    interactive: true
    highlightMoveDuration: 0
    highlightResizeDuration: 0

    onVisibleChanged: {
        dialog.hide()
    }

    property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

    property int modelCurrentIndex: listView.model.currentIndex ?? 0

    // Something weird with currentIndex happens when items are removed added.
    // listView.model.currentIndex contains the persistent index.

    onModelCurrentIndexChanged: {
        listView.currentIndex = listView.model.currentIndex
    }

    onCurrentIndexChanged: {
        listView.currentIndex = listView.model.currentIndex
        dialog.backend.currentRow = listView.currentIndex
    }

    readonly property int rowSpacing: StudioTheme.Values.toolbarHorizontalMargin
    readonly property int rowSpace: listView.width - (listView.rowSpacing * 4)
                                    - listView.style.squareControlSize.width
    property int rowWidth: listView.rowSpace / 4
    property int rowRest: listView.rowSpace % 4

    data: [
        PropertiesDialog {
            id: dialog
            visible: false
            backend: listView.model.delegate
        }
    ]

    delegate: Rectangle {
        id: itemDelegate
        width: ListView.view.width
        height: listView.style.squareControlSize.height
        color: mouseArea.containsMouse ?
                   itemDelegate.ListView.isCurrentItem ? listView.style.interactionHover
                                                       : listView.style.background.hover
                                       : "transparent"

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            property int currentIndex: listView.currentIndex

            Connections {
                target: mouseArea
                function onClicked() {
                    listView.model.currentIndex = index
                    listView.currentIndex = index
                    dialog.backend.currentRow = index
                    dialog.popup(mouseArea)
                }
            }
        }

        Row {
            id: row

            height: itemDelegate.height
            spacing: listView.rowSpacing

            property color textColor: itemDelegate.ListView.isCurrentItem ? listView.style.text.selectedText
                                                                          : listView.style.icon.idle
            Text {
                width: listView.rowWidth + listView.rowRest
                height: itemDelegate.height
                color: row.textColor
                text: target ?? ""
                anchors.verticalCenter: parent.verticalCenter
                font.bold: false
                elide: Text.ElideMiddle
                leftPadding: listView.rowSpacing
            }

            Text {
                width: listView.rowWidth
                height: itemDelegate.height
                color: row.textColor
                text: name ?? ""
                anchors.verticalCenter: parent.verticalCenter
                font.bold: false
                elide: Text.ElideMiddle
            }

            Text {
                width: listView.rowWidth + listView.rowRest
                height: itemDelegate.height
                color: row.textColor
                text: type ?? ""
                anchors.verticalCenter: parent.verticalCenter
                font.bold: false
                elide: Text.ElideMiddle
            }

            Text {
                width: listView.rowWidth + listView.rowRest
                height: itemDelegate.height
                color: row.textColor
                text: value ?? ""
                anchors.verticalCenter: parent.verticalCenter
                font.bold: false
                elide: Text.ElideMiddle
            }

            Rectangle {
                width: listView.style.squareControlSize.width
                height: listView.style.squareControlSize.height

                color: toolTipArea.containsMouse ?
                           itemDelegate.ListView.isCurrentItem ? listView.style.interactionHover
                                                               : listView.style.background.hover
                                               : "transparent"

                Text {
                    anchors.fill: parent

                    text: StudioTheme.Constants.remove_medium
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: listView.style.baseIconFontSize

                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter

                    color: row.textColor
                    renderType: Text.QtRendering
                }

                HelperWidgets.ToolTipArea {
                    id: toolTipArea
                    tooltip: qsTr("This is a test.")
                    anchors.fill: parent
                    onClicked: listView.model.remove(index)
                }
            }

        }
    }

    highlight: Rectangle {
        color: listView.style.interaction
        width: 600
    }
}
