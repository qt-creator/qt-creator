// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property Item __parentControl

    property bool linked: false

    color: "transparent"
    border.color: "transparent"

    implicitWidth: control.style.indicatorIconSize.width
    implicitHeight: control.style.indicatorIconSize.height

    z: 10

    T.Label {
        id: icon
        anchors.fill: parent
        text: control.linked ? StudioTheme.Constants.linked
                             : StudioTheme.Constants.unLinked
        visible: true
        color: control.style.indicator.idle
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: control.style.baseIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onPressed: control.linked = !control.linked
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.containsMouse
            PropertyChanges {
                target: icon
                color: control.style.indicator.idle
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse
            PropertyChanges {
                target: icon
                color: control.style.indicator.hover
            }
        }
    ]
}
