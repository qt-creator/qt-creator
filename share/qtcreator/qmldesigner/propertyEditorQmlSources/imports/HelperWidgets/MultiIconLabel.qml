// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root
    property alias tooltip: toolTipArea.tooltip
    property alias icon0: icon0.text
    property alias icon1: icon1.text
    // TODO second alias
    property alias iconColor: icon1.color

    implicitWidth: StudioTheme.Values.controlLabelWidth
    implicitHeight: StudioTheme.Values.controlLabelWidth

    color: "transparent"
    border.color: "transparent"

    T.Label {
        id: icon0
        anchors.fill: parent
        text: ""
        color: Qt.rgba(icon1.color.r,
                       icon1.color.g,
                       icon1.color.b,
                       0.5)
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    T.Label {
        id: icon1
        anchors.fill: parent
        text: ""
        color: StudioTheme.Values.themeIconColor
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
    }

    states: [
        State {
            name: "disabled"
            when: !root.enabled
            PropertyChanges {
                target: icon1
                color: StudioTheme.Values.themeIconColorDisabled
            }
        }
    ]
}
