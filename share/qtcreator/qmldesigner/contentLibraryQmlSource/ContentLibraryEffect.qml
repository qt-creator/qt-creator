// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import QtQuick.Controls
import StudioTheme as StudioTheme
import ContentLibraryBackend

Item {
    id: root

    signal showContextMenu()

    visible: modelData.bundleItemVisible

    MouseArea {
        id: mouseArea

        hoverEnabled: true
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton && !ContentLibraryBackend.effectsModel.importerRunning)
                ContentLibraryBackend.rootView.startDragEffect(modelData, mapToGlobal(mouse.x, mouse.y))
            else if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }
    }

    Column {
        anchors.fill: parent
        spacing: 1

        Image {
            id: img

            width: root.width
            height: img.width
            anchors.horizontalCenter: parent.horizontalCenter
            source: modelData.bundleItemIcon
            cache: false

            Rectangle { // circular indicator for imported bundle effect
                width: 10
                height: 10
                radius: 5
                anchors.right: img.right
                anchors.top: img.top
                anchors.margins: 5
                color: "#00ff00"
                border.color: "#555555"
                border.width: 1
                visible: modelData.bundleItemImported

                ToolTip {
                    visible: indicatorMouseArea.containsMouse
                    text: qsTr("Effect is imported to project")
                    delay: 1000
                }

                MouseArea {
                    id: indicatorMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }
        }

        Text {
            width: img.width
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: TextInput.AlignHCenter

            text: modelData.bundleItemName
            elide: Text.ElideRight
            font.pixelSize: StudioTheme.Values.myFontSize
            color: StudioTheme.Values.themeTextColor
        }
    } // Column
}
