// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0
import QtQuick.Layouts
import StudioTheme 1.0 as StudioTheme

Item {
    id: twirlButton
    width: 25
    height: 25
    state: "normal"

    property bool parentHovered: false
    property bool isHovered: mouseArea.containsMouse
    signal triggerRelease()

    Rectangle {
        id: background
        color: "#eab336"
        border.width: 0
        anchors.fill: parent
    }

    Text {
        id: twirlIcon
        width: 23
        height: 23
        color: Constants.currentGlobalText
        font.family: StudioTheme.Constants.iconFont.family
        text: StudioTheme.Constants.adsDropDown
        anchors.verticalCenter: parent.verticalCenter
        anchors.bottom: parent.bottom
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors.horizontalCenter: parent.horizontalCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        Connections {
            target: mouseArea
            onReleased: twirlButton.triggerRelease()
        }
    }

    states: [
        State {
            name: "hidden"
            when: !mouseArea.containsMouse && !mouseArea.pressed && !twirlButton.parentHovered

            PropertyChanges {
                target: background
                visible: false
            }
            PropertyChanges {
                target: twirlIcon
                visible: false
            }
        },
        State {
            name: "normal"
            when: !mouseArea.containsMouse && !mouseArea.pressed && twirlButton.parentHovered

            PropertyChanges {
                target: background
                visible: false
            }
            PropertyChanges {
                target: twirlIcon
                visible: true
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: twirlIcon
                scale: 1.4
            }
            PropertyChanges {
                target: background
                visible: true
                color: Constants.currentHoverThumbnailLabelBackground
            }
            PropertyChanges {
                target: twirlIcon
                visible: true
            }
        },
        State {
            name: "press"
            when: mouseArea.pressed

            PropertyChanges {
                target: twirlIcon
                color: Constants.currentGlobalText
                scale: 1.8
            }
            PropertyChanges {
                target: background
                visible: true
                color: Constants.currentBrand
            }
            PropertyChanges {
                target: twirlIcon
                visible: true
            }
        }
    ]
}
