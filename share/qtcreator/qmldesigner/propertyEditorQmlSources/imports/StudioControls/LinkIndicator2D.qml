// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: linkIndicator

    property Item myControl

    property bool linked: false

    color: "transparent"
    border.color: "transparent"

    implicitWidth: StudioTheme.Values.linkControlWidth
    implicitHeight: StudioTheme.Values.linkControlHeight

    z: 10

    T.Label {
        id: linkIndicatorIcon
        anchors.fill: parent
        text: linkIndicator.linked ? StudioTheme.Constants.linked
                                   : StudioTheme.Constants.unLinked
        visible: true
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onPressed: linkIndicator.linked = !linkIndicator.linked
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.containsMouse
            PropertyChanges {
                target: linkIndicatorIcon
                color: StudioTheme.Values.themeLinkIndicatorColor
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse
            PropertyChanges {
                target: linkIndicatorIcon
                color: StudioTheme.Values.themeLinkIndicatorColorHover
            }
        }
    ]
}
