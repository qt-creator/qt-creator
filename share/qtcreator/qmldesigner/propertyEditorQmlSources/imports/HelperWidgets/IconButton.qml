// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    signal clicked(mouse: var)
    signal pressed(mouse: var)
    signal released(mouse: var)

    property alias icon: icon.text
    property alias tooltip: toolTip.text
    property alias iconSize: icon.font.pixelSize
    property alias iconScale: icon.scale
    property alias iconColor: icon.color
    property alias iconStyle: icon.style
    property alias iconStyleColor: icon.styleColor

    property alias containsMouse: mouseArea.containsMouse
    property alias drag: mouseArea.drag

    property bool enabled: true
    property bool transparentBg: false
    property int buttonSize: StudioTheme.Values.height
    property color normalColor: root.transparentBg ? "transparent" : StudioTheme.Values.themeControlBackground
    property color hoverColor: root.transparentBg ? "transparent" : StudioTheme.Values.themeControlBackgroundHover
    property color pressColor: root.transparentBg ? "transparent" : StudioTheme.Values.themeControlBackgroundInteraction

    width: root.buttonSize
    height: root.buttonSize

    color: !root.enabled ? root.normalColor
                         : mouseArea.pressed ? root.pressColor
                                             : mouseArea.containsMouse ? root.hoverColor
                                                                       : root.normalColor

    Text {
        id: icon
        anchors.centerIn: root
        color: root.enabled ? StudioTheme.Values.themeTextColor : StudioTheme.Values.themeTextColorDisabled
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.baseIconFontSize
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: root.visible
        onClicked: function(mouse) {
            // We need to keep mouse area enabled even when button is disabled to make tooltip work
            if (root.enabled)
                root.clicked(mouse)
        }

        onPressed: function(mouse) {
            if (root.enabled)
                root.pressed(mouse)
        }

        onReleased: function(mouse) {
            if (root.enabled)
                root.released(mouse)
        }
    }

    ToolTip {
        id: toolTip

        visible: mouseArea.containsMouse && toolTip.text !== ""
        delay: 1000
    }
}
