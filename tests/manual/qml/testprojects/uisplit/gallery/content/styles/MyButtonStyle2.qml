// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.1

ButtonStyle {
    background: Rectangle {
        implicitHeight: 22
        implicitWidth: columnWidth
        color: control.pressed ? "darkGray" : control.activeFocus ? "#cdd" : "#ccc"
        antialiasing: true
        border.color: "gray"
        radius: height/2
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: "transparent"
            antialiasing: true
            visible: !control.pressed
            border.color: "#aaffffff"
            radius: height/2
        }
    }
}
