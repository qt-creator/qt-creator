

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.15
import QtQuick.Templates 2.15
import ExampleCheckout 1.0
import StudioTheme 1.0

Button {
    id: control

    implicitWidth: Math.max(
                       buttonBackground ? buttonBackground.implicitWidth : 0,
                       textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        buttonBackground ? buttonBackground.implicitHeight : 0,
                        textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    text: "My Button"
    property alias fontpixelSize: textItem.font.pixelSize
    //property bool decorated: false
    state: "normal"

    background: buttonBackground
    Rectangle {
        id: buttonBackground
        color: "#00000000"
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        radius: 2
        border.color: "#047eff"
        anchors.fill: parent
    }

    contentItem: textItem

    Text {
        id: textItem
        text: control.text
        font.pixelSize: 18

        opacity: enabled ? 1.0 : 0.3
        color: Values.themeTextColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    states: [
        State {
            name: "normal"
            when: !control.down && !control.hovered

            PropertyChanges {
                target: buttonBackground
                color: Values.themeControlBackground
                border.color: Values.themeControlOutline
            }

            PropertyChanges {
                target: textItem
                color: Values.themeTextColor
            }
        },
        State {
            name: "hover"
            when: control.hovered && !control.down
            PropertyChanges {
                target: textItem
                color: Values.themeTextColor
            }

            PropertyChanges {
                target: buttonBackground
                color: Values.themeControlBackgroundHover
                border.color: Values.themeControlOutline
            }
        },
        State {
            name: "activeQds"
            when: control.down
            PropertyChanges {
                target: textItem
                color: "#ffffff"
            }

            PropertyChanges {
                target: buttonBackground
                color: "#2e769e"
                border.color: "#00000000"
            }
        }
    ]
}
