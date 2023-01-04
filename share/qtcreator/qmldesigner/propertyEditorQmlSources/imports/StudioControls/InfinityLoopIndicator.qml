// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: infinityLoopIndicator

    property Item myControl

    property bool infinite: false

    color: "transparent"
    border.color: "transparent"

    implicitWidth: StudioTheme.Values.infinityControlWidth
    implicitHeight: StudioTheme.Values.infinityControlHeight

    z: 10

    T.Label {
        id: infinityLoopIndicatorIcon
        anchors.fill: parent
        text: StudioTheme.Constants.infinity
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
        onClicked: infinityLoopIndicator.infinite = !infinityLoopIndicator.infinite
    }

    states: [
        State {
            name: "active"
            when: infinityLoopIndicator.infinite && !mouseArea.containsMouse
            PropertyChanges {
                target: infinityLoopIndicatorIcon
                color: StudioTheme.Values.themeInfiniteLoopIndicatorColorInteraction
            }
        },
        State {
            name: "default"
            when: !mouseArea.containsMouse
            PropertyChanges {
                target: infinityLoopIndicatorIcon
                color: StudioTheme.Values.themeInfiniteLoopIndicatorColor
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse
            PropertyChanges {
                target: infinityLoopIndicatorIcon
                color: StudioTheme.Values.themeInfiniteLoopIndicatorColorHover
            }
        }
    ]
}
