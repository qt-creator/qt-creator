// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import WelcomeScreen 1.0
import StudioTheme 1.0 as Theme

Rectangle {
    id: controlPanel
    width: 220
    height: 80
    color: "#9b787878"
    radius: 10

    property bool closeOpen: true

    Text {
        id: closeOpen
        x: 203
        color: "#ffffff"
        text: qsTr("X")
        anchors.right: parent.right
        anchors.top: parent.top
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
        anchors.rightMargin: 9
        anchors.topMargin: 6

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            Connections {
                target: mouseArea
                function onClicked(mouse) { controlPanel.closeOpen = !controlPanel.closeOpen }
            }
        }
    }

    Text {
        id: themeSwitchLabel
        x: 8
        y: 50
        color: "#ffffff"
        text: qsTr("Theme")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
        anchors.rightMargin: 6
    }

    Text {
        id: lightLabel
        x: 172
        y: 26
        color: "#ffffff"
        text: qsTr("light")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
    }

    Text {
        id: darkLabel
        x: 65
        y: 26
        color: "#ffffff"
        text: qsTr("dark")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
    }

    Slider {
        id: themeSlider
        x: 58
        y: 44
        width: 145
        height: 28
        snapMode: RangeSlider.SnapAlways
        value: 0
        from: 0
        to: 1
        stepSize: 1

        Connections {
            target: themeSlider
            function onValueChanged(value) { Theme.Values.style = themeSlider.value }
        }
    }

    CheckBox {
        id: basicCheckBox
        x: 60
        y: 79
        text: qsTr("")
        checked: true
        onToggled: { Constants.basic = !Constants.basic }
    }

    CheckBox {
        id: communityCheckBox
        x: 174
        y: 79
        text: qsTr("")
        checked: false
        onToggled: { Constants.communityEdition = !Constants.communityEdition }
    }

    Text {
        id: basicEditionLabel
        x: 8
        y: 92
        color: "#ffffff"
        text: qsTr("Basic")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
        anchors.rightMargin: 6
    }

    Text {
        id: communityEditionLabel
        x: 116
        y: 92
        color: "#ffffff"
        text: qsTr("Community")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
        anchors.rightMargin: 6
    }

    states: [
        State {
            name: "open"
            when: controlPanel.closeOpen
        },
        State {
            name: "close"
            when: !controlPanel.closeOpen

            PropertyChanges {
                target: closeOpen
                text: qsTr("<")
            }

            PropertyChanges {
                target: controlPanel
                width: 28
                height: 26
                clip: true
            }
        }
    ]
}
