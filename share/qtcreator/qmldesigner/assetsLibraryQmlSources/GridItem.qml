// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioTheme as StudioTheme
import AssetsLibraryBackend

Rectangle {
    id: root

    property alias icon: icon
    property alias mouseArea: mouseArea
    property alias tooltip: tooltip

    property Item assetsView
    property Item assetsRoot

    property var assetsModel: AssetsLibraryBackend.assetsModel
    property var rootView: AssetsLibraryBackend.rootView

    property bool isSelected: false

    function refreshToolTip() {
        let txt = tooltip.text
        tooltip.text = txt
    }

    color: "transparent"
    border.width: root.isSelected ? StudioTheme.Values.border : 0
    border.color: StudioTheme.Values.themeInteraction

    Column {
        Image {
            id: icon

            width: 48
            height: 48
            cache: false
            sourceSize.width: 48
            sourceSize.height: 48
            asynchronous: true
            fillMode: Image.Pad
            source: "image://qmldesigner_assets/"
                    + (AssetsLibraryBackend.assetsModel.isDirectory(model.filePath) ? "folder" : model.filePath)
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: model.fileName
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.width
            clip: true
            horizontalAlignment: Text.AlignHCenter
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            mouseArea.forceActiveFocus()
        }
    }

    ToolTip {
        id: tooltip
        visible: mouseArea.containsMouse
        delay: 1000
    }
}
