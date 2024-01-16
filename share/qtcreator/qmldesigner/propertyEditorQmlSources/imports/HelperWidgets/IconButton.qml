// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    signal clicked()
    signal pressed()
    signal released()

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

    width: buttonSize
    height: buttonSize

    color: !enabled ? normalColor
                    : mouseArea.pressed ? pressColor
                                        : mouseArea.containsMouse ? hoverColor
                                                                  : normalColor

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
        onClicked: {
            // We need to keep mouse area enabled even when button is disabled to make tooltip work
            if (root.enabled)
                root.clicked()
        }

        onPressed: {
            if (root.enabled)
                root.pressed()
        }

        onReleased: {
            if (root.enabled)
                root.released()
        }
    }

    ToolTip {
        id: toolTip

        visible: mouseArea.containsMouse && text !== ""
        delay: 1000
    }
}
