// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Item {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias icon: icon
    property bool hover: mouseArea.containsMouse
    property bool pressed: mouseArea.pressed

    implicitWidth: control.style.squareControlSize.width
    implicitHeight: control.style.squareControlSize.height

    signal clicked
    z: 10

    T.Label {
        id: icon
        anchors.fill: parent
        text: StudioTheme.Constants.actionIcon
        color: control.style.icon.idle
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: control.style.baseIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        states: [
            State {
                name: "hover"
                when: control.hover && !control.pressed && control.enabled
                PropertyChanges {
                    target: icon
                    scale: 1.2
                    visible: true
                }
            },
            State {
                name: "disable"
                when: !control.enabled
                PropertyChanges {
                    target: icon
                    color: control.style.icon.disabled
                }
            }
        ]
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: control.clicked()
    }
}
