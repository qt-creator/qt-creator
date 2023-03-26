// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

Item {
    Rectangle{
        color: styleData.value.get(0).color
        anchors.top:parent.top
        anchors.right:parent.right
        anchors.bottom:parent.bottom
        anchors.margins: 4
        width:32
        border.color:"#666"
    }
    Text {
        width: parent.width
        anchors.margins: 4
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        elide: styleData.elideMode
        text: styleData.value.get(0).description
        color: styleData.textColor
    }
}
