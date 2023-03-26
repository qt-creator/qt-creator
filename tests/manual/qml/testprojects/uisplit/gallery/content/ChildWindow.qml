// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

Window {
    id: window1

    width: 400
    height: 400

    title: "child window"
    flags: Qt.Dialog

    Rectangle {
        color: syspal.window
        anchors.fill: parent

        Label {
            id: dimensionsText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            id: availableDimensionsText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: dimensionsText.bottom
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            id: closeText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: availableDimensionsText.bottom
            text: "This is a new Window, press the\nbutton below to close it again."
        }
        Button {
            anchors.horizontalCenter: closeText.horizontalCenter
            anchors.top: closeText.bottom
            id: closeWindowButton
            text:"Close"
            width: 98
            tooltip:"Press me, to close this window again"
            onClicked: window1.visible = false
        }
        Button {
            anchors.horizontalCenter: closeText.horizontalCenter
            anchors.top: closeWindowButton.bottom
            id: maximizeWindowButton
            text:"Maximize"
            width: 98
            tooltip:"Press me, to maximize this window again"
            onClicked: window1.visibility = Window.Maximized;
        }
        Button {
            anchors.horizontalCenter: closeText.horizontalCenter
            anchors.top: maximizeWindowButton.bottom
            id: normalizeWindowButton
            text:"Normalize"
            width: 98
            tooltip:"Press me, to normalize this window again"
            onClicked: window1.visibility = Window.Windowed;
        }
        Button {
            anchors.horizontalCenter: closeText.horizontalCenter
            anchors.top: normalizeWindowButton.bottom
            id: minimizeWindowButton
            text:"Minimize"
            width: 98
            tooltip:"Press me, to minimize this window again"
            onClicked: window1.visibility = Window.Minimized;
        }
    }
}

