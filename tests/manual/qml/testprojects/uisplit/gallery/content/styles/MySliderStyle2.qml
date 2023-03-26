// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.1

SliderStyle {
    handle: Rectangle {
        width: 18
        height: 18
        color: control.pressed ? "darkGray" : "lightGray"
        border.color: "gray"
        antialiasing: true
        radius: height/2
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: "transparent"
            antialiasing: true
            border.color: "#eee"
            radius: height/2
        }
    }

    groove: Rectangle {
        height: 8
        implicitWidth: columnWidth
        implicitHeight: 22

        antialiasing: true
        color: "#ccc"
        border.color: "#777"
        radius: height/2
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: "transparent"
            antialiasing: true
            border.color: "#66ffffff"
            radius: height/2
        }
    }
}
