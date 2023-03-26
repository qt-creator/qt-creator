// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    property Item myControl
    property alias icon: indicatorIcon.text
    property alias iconColor: indicatorIcon.color
    property alias pixelSize: indicatorIcon.font.pixelSize
    property alias tooltip: toolTipArea.tooltip

    property bool hovered: toolTipArea.containsMouse && root.enabled

    signal clicked()

    color: "transparent"
    border.color: "transparent"

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
