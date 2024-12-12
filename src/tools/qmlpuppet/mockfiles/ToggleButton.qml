// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12
import QtQuick 2.0
import QtQuick.Controls 2.0

Rectangle {
    property bool toggled: false
    property string tooltip
    property bool toggleBackground: false // show a black background for the toggled state
    property var states: [] // array of 2 state-objects, idx 0: untoggled, idx 1: toggled

    id: root
    color: toggleBackground && toggled ? "#aa000000" : mouseArea.containsMouse ? "#44000000" : "#00000000"
    width: txt.width + 5
    height: 16

    Text {
        id: txt
        color: "#b5b5b5"
        anchors.verticalCenter: parent.verticalCenter
        text: root.states[toggled ? 1 : 0].text
    }

    ToolTip {
        text: tooltip
        visible: mouseArea.containsMouse
        delay: 1000
    }

    MouseArea {
        id: mouseArea
        cursorShape: "PointingHandCursor"
        hoverEnabled: true
        anchors.fill: parent
        onClicked: root.toggled = !root.toggled
    }
}


