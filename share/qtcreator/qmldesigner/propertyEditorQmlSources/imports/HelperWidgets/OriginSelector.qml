// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    property Item myControl
    property string value

    width: StudioTheme.Values.height
    height: StudioTheme.Values.height

    border.width: StudioTheme.Values.border
    border.color: "transparent"

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: myControl.originSelectorClicked(root.value)
    }

    states: [
        State {
            name: "default"
            when: !mouseArea.containsMouse && !mouseArea.pressed && myControl.origin !== root.value
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeTextColorDisabled // TODO
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed && myControl.origin !== root.value
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeControlBackgroundInteraction // TODO
            }
        },
        State {
            name: "press"
            when: mouseArea.containsPress && myControl.origin !== root.value
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeControlBackgroundInteraction // TODO
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "active"
            when: myControl.origin === root.value
            PropertyChanges {
                target: root
                color: StudioTheme.Values.themeInteraction
            }
        }
    ]
}
