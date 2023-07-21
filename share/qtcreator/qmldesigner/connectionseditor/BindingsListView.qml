// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import ConnectionsEditor
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme
import ConnectionsEditorEditorBackend

ListView {
    id: listView
    width: 606
    height: 160
    interactive: false
    highlightMoveDuration: 0

    onVisibleChanged: {
        dialog.hide()
    }

    property int modelCurrentIndex: listView.model.currentIndex ?? 0

    /* Something weird with currentIndex happens when items are removed added.
  listView.model.currentIndex contains the persistent index.
  */
    onModelCurrentIndexChanged: {
        listView.currentIndex = listView.model.currentIndex
    }

    onCurrentIndexChanged: {
        listView.currentIndex = listView.model.currentIndex
        dialog.backend.currentRow = listView.currentIndex
    }

    data: [
        BindingsDialog {
            id: dialog
            visible: false
            backend: listView.model.delegate
        }
    ]
    delegate: Item {

        width: 600
        height: 18

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
                listView.model.currentIndex = index
                listView.currentIndex = index
                dialog.backend.currentRow = index
                dialog.popup(mouseArea)
            }

            property int currentIndex: listView.currentIndex
        }

        Row {
            id: row1
            x: 0
            y: 0
            width: 600
            height: 16
            spacing: 10

            Text {
                width: 120
                color: "#ffffff"
                text: target ?? ""
                anchors.verticalCenter: parent.verticalCenter
                font.bold: false
            }

            Text {
                width: 120
                text: targetProperty ?? ""
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                font.bold: false
            }

            Text {
                width: 120
                text: source ?? ""
                anchors.verticalCenter: parent.verticalCenter
                color: "#ffffff"
                font.bold: false
            }

            Text {
                width: 120
                text: sourceProperty ?? ""
                anchors.verticalCenter: parent.verticalCenter
                color: "#ffffff"
                font.bold: false
            }

            Text {
                width: 120

                text: "-"
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
                font.pointSize: 14
                color: "#ffffff"
                font.bold: true
                MouseArea {
                    anchors.fill: parent
                    onClicked: listView.model.remove(index)
                }
            }
        }
    }

    highlight: Rectangle {
        color: "#2a5593"
        width: 600
    }
}
