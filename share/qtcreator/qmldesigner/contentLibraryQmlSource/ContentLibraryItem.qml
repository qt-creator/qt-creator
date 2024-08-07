// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import ContentLibraryBackend

Item {
    id: root

    signal showContextMenu()
    signal addToProject()

    visible: modelData.bundleItemVisible

    MouseArea {
        id: mouseArea

        enabled: !ContentLibraryBackend.rootView.importerRunning
        hoverEnabled: true
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                ContentLibraryBackend.rootView.startDragItem(modelData, mapToGlobal(mouse.x, mouse.y))
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

            Rectangle { // circular indicator for imported bundle item
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
                    text: qsTr("Item is imported to the project")
                    delay: 1000
                }

                MouseArea {
                    id: indicatorMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }

            HelperWidgets.IconButton {
                id: addToProjectButton

                icon: StudioTheme.Constants.plus
                tooltip: qsTr("Add an instance to project")
                buttonSize: 22
                property color c: "white"
                normalColor: Qt.hsla(c.hslHue, c.hslSaturation, c.hslLightness, .2)
                hoverColor: Qt.hsla(c.hslHue, c.hslSaturation, c.hslLightness, .3)
                pressColor: Qt.hsla(c.hslHue, c.hslSaturation, c.hslLightness, .4)
                anchors.right: img.right
                anchors.bottom: img.bottom
                enabled: !ContentLibraryBackend.rootView.importerRunning
                visible: containsMouse || mouseArea.containsMouse

                onClicked: {
                    root.addToProject()
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
