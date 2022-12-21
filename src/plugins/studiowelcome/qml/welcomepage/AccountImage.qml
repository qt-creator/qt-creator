// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.9
import welcome 1.0
import StudioFonts 1.0

Image {
    id: account_icon

    source: "images/" + (mouseArea.containsMouse ? "icon_hover.png" : "icon_default.png")

    Text {
        id: account
        color: mouseArea.containsMouse ? Constants.textHoverColor
                                       : Constants.textDefaultColor
        text: qsTr("Account")
        anchors.top: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        font.family: StudioFonts.titilliumWeb_regular
        font.pixelSize: 16
        renderType: Text.NativeRendering
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        anchors.margins: -25
        hoverEnabled: true

        onClicked: Qt.openUrlExternally("https://login.qt.io/login/")
    }
}
