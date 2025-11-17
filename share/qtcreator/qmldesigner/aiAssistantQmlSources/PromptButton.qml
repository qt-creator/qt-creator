// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import StudioControls as StudioControls

Rectangle {
    id: root

    signal clicked()

    property alias label: labelText.text
    property alias enabled: mouseArea.enabled
    property string tooltip: ""

    width: labelText.width + 12
    height: labelText.height + 8
    radius: StudioTheme.Values.smallRadius

    objectName: label // for Squish identification

    border.width: 1
    border.color: StudioTheme.Values.themeControlOutlineHover

    color: enabled ? mouseArea.containsMouse ? mouseArea.pressed ? StudioTheme.Values.themePopoutButtonBackground_interaction
                                                                 : StudioTheme.Values.themePopoutButtonBackground_hover
                                             : StudioTheme.Values.themePopoutButtonBackground_idle
                   : StudioTheme.Values.themePopoutButtonBackground_disabled

    StudioControls.ToolTip {
        visible: root.tooltip !== "" && mouseArea.containsMouse
        text: root.tooltip
    }

    Text {
        id: labelText

        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize

        verticalAlignment: Text.AlignVCenter

        anchors.centerIn: parent
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
