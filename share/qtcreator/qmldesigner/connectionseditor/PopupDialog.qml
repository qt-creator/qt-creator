// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

Window {
    id: window
    width: 400
    height: 800
    visible: true
    flags: Qt.FramelessWindowHint || Qt.Dialog

    color: Qt.transparent

    property int bw: 5

    function popup(item) {
        print("popup " + item)
        var padding = 12
        var p = item.mapToGlobal(0, 0)
        dialog.x = p.x - dialog.width - padding
        if (dialog.x < 0)
            dialog.x = p.x + item.width + padding
        dialog.y = p.y
        dialog.show()
        dialog.raise()
    }

    Rectangle {
        id: rectangle1
        color: "#d7d7d7"
        border.color: "#232323"
        anchors.fill: parent



        Rectangle {
            id: rectangle
            height: 32
            color: "#797979"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            DragHandler {
                grabPermissions: TapHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) { window.startSystemMove(); }
            }

            Rectangle {
                id: rectangle2
                x: 329
                width: 16
                height: 16
                color: "#ffffff"
                radius: 4
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 6

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    onClicked: window.close()
                }
            }
        }
    }
}
