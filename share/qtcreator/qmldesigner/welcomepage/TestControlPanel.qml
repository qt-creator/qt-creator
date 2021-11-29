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

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0
import QtQuick.Layouts 1.0
import StudioTheme 1.0 as Theme

Item {
    id: controlPanel
    width: 220
    height: 120
    property bool closeOpen: true

    Rectangle {
        id: controlPanBack
        color: "#9b787878"
        radius: 10
        border.color: "#00000000"
        anchors.fill: parent
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0

        CheckBox {
            id: checkBox
            x: 60
            y: 119
            text: qsTr("")
            checked: true
            onToggled: { Constants.basic = !Constants.basic }
        }

        CheckBox {
            id: checkBox1
            x: 174
            y: 119
            text: qsTr("")
            checked: false
            onToggled: { Constants.communityEdition = !Constants.communityEdition }
        }
    }

    Slider {
        id:themeSlider
        x: 58
        y: 84
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

    Text {
        id: themeSwitchLabel
        x: 8
        y: 90
        color: "#ffffff"
        text: qsTr("Theme")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
        anchors.rightMargin: 6
    }

    Switch {
        id: brandSwitch
        x: 92
        y: 23
        width: 59
        height: 31
        display: AbstractButton.IconOnly
        Connections {
            target: brandSwitch
            function onToggled(bool) { Constants.defaultBrand = !Constants.defaultBrand }
        }
    }

    Text {
        id: brandSwitchLabel
        x: 11
        y: 31
        color: "#ffffff"
        text: qsTr("Brand")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
    }

    Text {
        id: qdslabel
        x: 65
        y: 31
        color: "#ffffff"
        text: qsTr("qds")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
    }

    Text {
        id: creatorLabel
        x: 155
        y: 31
        color: "#ffffff"
        text: qsTr("Creator")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
    }

    Text {
        id: lightLabel
        x: 172
        y: 66
        color: "#ffffff"
        text: qsTr("light")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
    }

    Text {
        id: lightLabel1
        x: 124
        y: 66
        color: "#ffffff"
        text: qsTr("mid")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
    }

    Text {
        id: lightLabel2
        x: 65
        y: 66
        color: "#ffffff"
        text: qsTr("dark")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
    }

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
        id: themeSwitchLabel1
        x: 8
        y: 132
        color: "#ffffff"
        text: qsTr("Basic")
        font.pixelSize: 12
        horizontalAlignment: Text.AlignRight
        anchors.rightMargin: 6
    }

    Text {
        id: themeSwitchLabel2
        x: 116
        y: 132
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

/*##^##
Designer {
    D{i:0;formeditorZoom:4}D{i:3}D{i:19}
}
##^##*/
