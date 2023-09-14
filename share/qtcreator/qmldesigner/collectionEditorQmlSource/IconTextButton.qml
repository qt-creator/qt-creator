// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme

Rectangle {
    id: root

    required property string text
    required property string icon

    property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

    implicitHeight: style.squareControlSize.height
    implicitWidth: rowAlign.width

    signal clicked()

    Row {
        id: rowAlign
        spacing: 0
        leftPadding: StudioTheme.Values.inputHorizontalPadding
        rightPadding: StudioTheme.Values.inputHorizontalPadding

        Text {
            id: iconItem
            width: root.style.squareControlSize.width
            height: root.height
            anchors.verticalCenter: parent.verticalCenter
            text: root.icon
            color: StudioTheme.Values.themeTextColor
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: root.style.baseIconFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            id: textItem
            height: root.height
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: StudioTheme.Values.themeTextColor
            font.family: StudioTheme.Constants.font.family
            font.pixelSize: root.style.baseIconFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.pressed && !mouseArea.containsMouse
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeBackgroundColorNormal
            }
        },
        State {
            name: "Pressed"
            when: mouseArea.pressed
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }
        },
        State {
            name: "Hovered"
            when: !mouseArea.pressed && mouseArea.containsMouse
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        }
    ]
}
