// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates
import WelcomeScreen 1.0
import StudioTheme 1.0 as StudioTheme

Item {
    id: youtubeButton
    state: "darkNormal"

    property bool isHovered: mouseArea.containsMouse

    Image {
        id: youtubeDarkNormal
        anchors.fill: parent
        source: "images/youtubeDarkNormal.png"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: youtubeLightNormal
        anchors.fill: parent
        source: "images/youtubeLightNormal.png"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: youtubeLightHover
        anchors.fill: parent
        source: "images/youtubeLightHover.png"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: youtubeDarkHover
        anchors.fill: parent
        source: "images/youtubeDarkHover.png"
        fillMode: Image.PreserveAspectFit
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        Connections {
            target: mouseArea
            function onClicked(mouse) { Qt.openUrlExternally("https://www.youtube.com/user/QtStudios/") }
        }
    }
    states: [
        State {
            name: "darkNormal"
            when: !StudioTheme.Values.isLightTheme && !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: youtubeDarkNormal
                visible: true
            }

            PropertyChanges {
                target: youtubeLightNormal
                visible: false
            }

            PropertyChanges {
                target: youtubeLightHover
                visible: false
            }

            PropertyChanges {
                target: youtubeDarkHover
                visible: false
            }
        },
        State {
            name: "lightNormal"
            when: StudioTheme.Values.isLightTheme && !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: youtubeDarkHover
                visible: false
            }

            PropertyChanges {
                target: youtubeLightHover
                visible: false
            }

            PropertyChanges {
                target: youtubeLightNormal
                visible: true
            }

            PropertyChanges {
                target: youtubeDarkNormal
                visible: false
            }
        },
        State {
            name: "darkHover"
            when: !StudioTheme.Values.isLightTheme && mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: youtubeDarkNormal
                visible: false
            }

            PropertyChanges {
                target: youtubeLightNormal
                visible: false
            }

            PropertyChanges {
                target: youtubeLightHover
                visible: false
            }

            PropertyChanges {
                target: youtubeDarkHover
                visible: true
            }
        },
        State {
            name: "lightHover"
            when: StudioTheme.Values.isLightTheme && mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: youtubeDarkHover
                visible: false
            }

            PropertyChanges {
                target: youtubeLightHover
                visible: true
            }

            PropertyChanges {
                target: youtubeLightNormal
                visible: false
            }

            PropertyChanges {
                target: youtubeDarkNormal
                visible: false
            }
        },
        State {
            name: "darkPress"
            when: !StudioTheme.Values.isLightTheme && mouseArea.pressed

            PropertyChanges {
                target: youtubeDarkHover
                visible: true
                scale: 1.1
            }

            PropertyChanges {
                target: youtubeLightHover
                visible: false
            }

            PropertyChanges {
                target: youtubeLightNormal
                visible: false
            }

            PropertyChanges {
                target: youtubeDarkNormal
                visible: false
            }
        },
        State {
            name: "lightPress"
            when: StudioTheme.Values.isLightTheme && mouseArea.pressed

            PropertyChanges {
                target: youtubeDarkHover
                visible: false
            }

            PropertyChanges {
                target: youtubeLightHover
                visible: true
                scale: 1.1
            }

            PropertyChanges {
                target: youtubeLightNormal
                visible: false
            }

            PropertyChanges {
                target: youtubeDarkNormal
                visible: false
            }
        }
    ]
}
