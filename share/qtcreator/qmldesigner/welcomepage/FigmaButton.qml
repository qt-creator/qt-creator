// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import WelcomeScreen 1.0
import StudioTheme 1.0 as StudioTheme

Item {
    id: figmaButton
    state: "darkNormal"

    property bool isHovered: mouseArea.containsMouse

    Image {
        id: figmaDarkNormal
        anchors.fill: parent
        source: "images/figmaDarkNormal.png"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: figmaLightNormal
        anchors.fill: parent
        source: "images/figmaLightNormal.png"
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: figmaHover
        anchors.fill: parent
        source: "images/figmaHover.png"
        fillMode: Image.PreserveAspectFit
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        Connections {
            target: mouseArea
            function onClicked(mouse) { Qt.openUrlExternally("https://www.figma.com/@qtdesignstudio/") }
        }
    }

    states: [
        State {
            name: "darkNormal"
            when: !StudioTheme.Values.isLightTheme && !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: figmaDarkNormal
                visible: true
            }

            PropertyChanges {
                target: figmaLightNormal
                visible: false
            }

            PropertyChanges {
                target: figmaHover
                visible: false
            }
        },
        State {
            name: "lightNormal"
            when: StudioTheme.Values.isLightTheme && !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: figmaHover
                visible: false
            }

            PropertyChanges {
                target: figmaLightNormal
                visible: true
            }

            PropertyChanges {
                target: figmaDarkNormal
                visible: false
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: figmaHover
                visible: true
            }

            PropertyChanges {
                target: figmaLightNormal
                visible: false
            }

            PropertyChanges {
                target: figmaDarkNormal
                visible: false
            }
        },
        State {
            name: "press"
            when: (mouseArea.containsMouse || !mouseArea.containsMouse) && mouseArea.pressed

            PropertyChanges {
                target: figmaHover
                visible: true
                scale: 1.1
            }

            PropertyChanges {
                target: figmaLightNormal
                visible: false
            }

            PropertyChanges {
                target: figmaDarkNormal
                visible: false
            }
        }
    ]
}
