// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StudioTheme as StudioTheme

Rectangle {
    id: root

    required property string text
    required property string icon

    property alias tooltip: toolTip.text
    property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle
    property int fontSize: StudioTheme.Values.baseFontSize

    implicitHeight: style.squareControlSize.height
    implicitWidth: rowAlign.width

    signal clicked()

    RowLayout {
        id: rowAlign

        anchors.verticalCenter: parent.verticalCenter
        spacing: StudioTheme.Values.inputHorizontalPadding

        Text {
            id: iconItem

            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            text: root.icon
            color: StudioTheme.Values.themeTextColor
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.bigFont
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            leftPadding: StudioTheme.Values.inputHorizontalPadding
        }

        Text {
            id: textItem

            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            text: root.text
            color: StudioTheme.Values.themeTextColor
            font.family: StudioTheme.Constants.font.family
            font.pixelSize: StudioTheme.Values.baseFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            rightPadding: StudioTheme.Values.inputHorizontalPadding
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    ToolTip {
        id: toolTip

        visible: mouseArea.containsMouse && text !== ""
        delay: 1000
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.pressed && !mouseArea.containsMouse
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeControlBackground
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
