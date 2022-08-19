// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import Loginui1

Rectangle {
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor

    Image {
        id: adventurePage
        x: 0
        y: 0
        source: "images/adventurePage.jpg"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: qt_logo_green_128x128px
        x: 296
        y: 0
        source: "images/qt_logo_green_128x128px.png"
        fillMode: Image.PreserveAspectFit
    }
    Text {
        color: "#ffffff"
        text: qsTr("Are you ready to explore?")
        font.pixelSize: 50
        font.family: "Titillium Web ExtraLight"
        anchors.verticalCenterOffset: -430
        anchors.horizontalCenterOffset: 0
        anchors.centerIn: parent
    }

    EntryField {
        id: username
        x: 110
        y: 470
        text: qsTr("Username or Email")
    }

    EntryField {
        id: password
        x: 110
        y: 590
        text: qsTr("Password")
    }

    PushButton {
        id: login
        x: 101
        y: 944
        text: qsTr("Continue")
    }

    PushButton {
        id: creteAccount
        x: 101
        y: 1088
        text: qsTr("Create Account")
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.5}D{i:1}D{i:2}D{i:3}D{i:4}D{i:5}D{i:6}D{i:7}
}
##^##*/

