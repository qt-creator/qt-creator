// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import Qt 4.7

TabWidget {
    id: tabs
    width: 640; height: 480

    Rectangle {
        property string title: "Red"
        anchors.fill: parent
        color: "#e3e3e3"

        Rectangle {
            anchors { fill: parent; topMargin: 20; leftMargin: 20; rightMargin: 20; bottomMargin: 20 }
            color: "#ff7f7f"
            Text {
                width: parent.width - 20
                anchors.centerIn: parent; horizontalAlignment: Qt.AlignHCenter
                text: "Roses are red"
                font.pixelSize: 20
                wrapMode: Text.WordWrap
            }
        }
    }

    Rectangle {
        property string title: "Green"
        anchors.fill: parent
        color: "#e3e3e3"

        Rectangle {
            anchors { fill: parent; topMargin: 20; leftMargin: 20; rightMargin: 20; bottomMargin: 20 }
            color: "#7fff7f"
            Text {
                width: parent.width - 20
                anchors.centerIn: parent; horizontalAlignment: Qt.AlignHCenter
                text: "Flower stems are green"
                font.pixelSize: 20
                wrapMode: Text.WordWrap
            }
        }
    }

    Rectangle {
        property string title: "Blue"
        anchors.fill: parent; color: "#e3e3e3"

        Rectangle {
            anchors { fill: parent; topMargin: 20; leftMargin: 20; rightMargin: 20; bottomMargin: 20 }
            color: "#7f7fff"
            Text {
                width: parent.width - 20
                anchors.centerIn: parent; horizontalAlignment: Qt.AlignHCenter
                text: "Violets are blue"
                font.pixelSize: 20
                wrapMode: Text.WordWrap
            }
        }
    }
}
