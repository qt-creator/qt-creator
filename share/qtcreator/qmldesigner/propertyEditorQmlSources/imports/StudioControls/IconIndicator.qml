// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme as StudioTheme

Item {
    id: root

    property Item myControl
    property alias icon: indicatorIcon.text
    property alias iconColor: indicatorIcon.color
    property alias pixelSize: indicatorIcon.font.pixelSize
    property alias toolTip: toolTipArea.text

    property bool hovered: toolTipArea.containsMouse && root.enabled

    signal clicked()

    implicitWidth: StudioTheme.Values.linkControlWidth
    implicitHeight: StudioTheme.Values.linkControlHeight

    z: 10

    T.Label {
        id: indicatorIcon
        anchors.fill: parent
        text: "?"
        visible: true
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        onClicked: root.clicked()
    }

    states: [
        State {
            name: "default"
            when: !toolTipArea.containsMouse && root.enabled
            PropertyChanges {
                target: indicatorIcon
                color: StudioTheme.Values.themeLinkIndicatorColor
            }
        },
        State {
            name: "hover"
            when: toolTipArea.containsMouse && root.enabled
            PropertyChanges {
                target: indicatorIcon
                color: StudioTheme.Values.themeLinkIndicatorColorHover
            }
        },
        State {
            name: "disable"
            when: !root.enabled
            PropertyChanges {
                target: indicatorIcon
                color: StudioTheme.Values.themeLinkIndicatorColorDisabled
            }
        }
    ]
}
