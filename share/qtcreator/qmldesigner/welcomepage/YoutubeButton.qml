/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Controls 2.12
import WelcomeScreen 1.0
import StudioTheme 1.0 as StudioTheme

Item {
    id: youtubeButton
    property bool isHovered: false
    state: "darkNormal"

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
            onClicked: Qt.openUrlExternally("https://www.youtube.com/user/QtStudios/")
        }
    }
    states: [
        State {
            name: "darkNormal"
            when: StudioTheme.Values.style === 0 && !mouseArea.containsMouse
                  && !mouseArea.pressed

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
            when: StudioTheme.Values.style !== 0 && !mouseArea.containsMouse
                  && !mouseArea.pressed

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
            when: StudioTheme.Values.style === 0 && mouseArea.containsMouse
                  && !mouseArea.pressed

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
            PropertyChanges {
                target: youtubeButton
                isHovered: true
            }
        },
        State {
            name: "lightHover"
            when: StudioTheme.Values.style !== 0 && mouseArea.containsMouse
                  && !mouseArea.pressed

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

            PropertyChanges {
                target: youtubeButton
                isHovered: true
            }
        },
        State {
            name: "darkPressed"
            when: StudioTheme.Values.style === 0
                  && (mouseArea.containsMouse || !mouseArea.containsMouse)
                  && mouseArea.pressed

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

            PropertyChanges {
                target: youtubeButton
                isHovered: true
            }
        },
        State {
            name: "lightPressed"
            when: StudioTheme.Values.style !== 0
                  && (mouseArea.containsMouse || !mouseArea.containsMouse)
                  && mouseArea.pressed

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

            PropertyChanges {
                target: youtubeButton
                isHovered: true
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorColor:"#000000";formeditorZoom:1.5;height:480;width:640}
}
##^##*/

