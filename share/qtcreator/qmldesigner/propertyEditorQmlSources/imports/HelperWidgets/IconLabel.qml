// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    property alias tooltip: toolTipArea.tooltip
    property alias icon: icon.text
    property alias iconColor: icon.color
    property alias pixelSize: icon.font.pixelSize
    property alias horizontalAlignment: icon.horizontalAlignment

    implicitWidth: StudioTheme.Values.controlLabelWidth
    implicitHeight: StudioTheme.Values.controlLabelWidth

    color: "transparent"
    border.color: "transparent"

    T.Label {
        id: icon
        anchors.fill: parent
        color: StudioTheme.Values.themeIconColor
        text: ""
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
    }
}
