// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import WelcomeScreen 1.0
import StudioTheme 1.0 as StudioTheme

Item {
    id: twitterButton
    state: "darkNormal"

    property bool isHovered: mouseArea.containsMouse

    Image {
        id: twitterDarkNormal
        anchors.fill: parent
        source: "images/twitterDarkNormal.png"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: twitterLightNormal
        anchors.fill: parent
        source: "images/twitterLightNormal.png"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: twitterHover
        anchors.fill: parent
        source: "images/twitterHover.png"
        fillMode: Image.PreserveAspectFit
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        Connections {
            target: mouseArea
            function onClicked(mouse) { Qt.openUrlExternally("https://twitter.com/qtproject/") }
        }
    }

    states: [
        State {
            name: "darkNormal"
            when: !StudioTheme.Values.isLightTheme && !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: twitterDarkNormal
                visible: true
            }

            PropertyChanges {
                target: twitterLightNormal
                visible: false
            }

            PropertyChanges {
                target: twitterHover
                visible: false
            }
        },
        State {
            name: "lightNormal"
            when: StudioTheme.Values.isLightTheme && !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: twitterHover
                visible: false
            }

            PropertyChanges {
                target: twitterLightNormal
                visible: true
            }

            PropertyChanges {
                target: twitterDarkNormal
                visible: false
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: twitterHover
                visible: true
            }

            PropertyChanges {
                target: twitterLightNormal
                visible: false
            }

            PropertyChanges {
                target: twitterDarkNormal
                visible: false
            }
        },
        State {
            name: "press"
            when: (mouseArea.containsMouse || !mouseArea.containsMouse) && mouseArea.pressed

            PropertyChanges {
                target: twitterHover
                visible: true
                scale: 1.1
            }

            PropertyChanges {
                target: twitterLightNormal
                visible: false
            }

            PropertyChanges {
                target: twitterDarkNormal
                visible: false
            }
        }
    ]
}
