// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0
import StudioTheme 1.0 as StudioTheme
import UiTour

Item {
    id: tourButton
    width: 40
    height: 40
    property alias dialogButtonText: dialogButton.text

    signal buttonClicked

    Text {
        id: dialogButton
        color: "#ffffff"
        font.family: StudioTheme.Constants.iconFont.family
        text: StudioTheme.Constants.closeFile_large
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
                font.pixelSize: 28
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: dialogButton
                font.pixelSize: 29
            }
        },
        State {
            name: "press"
            when: mouseArea.pressed

            PropertyChanges {
                target: dialogButton
                font.pixelSize: 29
            }
        }
    ]
}
