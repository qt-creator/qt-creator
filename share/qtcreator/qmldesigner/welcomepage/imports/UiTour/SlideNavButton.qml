// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0
import StudioTheme 1.0 as StudioTheme
import UiTour

Item {
    id: tourButton
    width: 120
    height: 120
    property alias dialogButtonRotation: dialogButton.rotation
    property alias dialogButtonFontpixelSize: dialogButton.font.pixelSize
    property alias dialogButtonText: dialogButton.text

    signal buttonClicked

    Text {
        id: dialogButton
        color: "#ffffff"
        text: StudioTheme.Constants.nextFile_large
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: 32
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        Connections {
            target: mouseArea
            onClicked: tourButton.buttonClicked()
        }
    }

    states: [
        State {
            name: "normal"
            when: !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: dialogButton
                color: "#ecebeb"
                font.pixelSize: 92
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: dialogButton
                font.pixelSize: 96
            }
        },
        State {
            name: "press"
            when: mouseArea.pressed

            PropertyChanges {
                target: dialogButton
                font.pixelSize: 98
            }
        }
    ]
}
