// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme

Rectangle {
    id: root

    signal clicked()

    property alias icon: icon.text
    property alias name: name.text
    property bool selected: false

    width: 100
    height: 100
    color: root.selected ? StudioTheme.Values.themePanelBackground
                         : mouseArea.containsMouse ? Qt.lighter(StudioTheme.Values.themeSectionHeadBackground, 1.3)
                                                   : StudioTheme.Values.themeSectionHeadBackground

    Text {
        id: icon

        color: root.selected ? StudioTheme.Values.themeInteraction : StudioTheme.Values.themeTextColor

        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.mediumIconFontSize
        anchors.horizontalCenter: parent.horizontalCenter
        y: 8
    }

    Text {
        id: name

        font.weight: Font.DemiBold
        font.pixelSize: StudioTheme.Values.baseFontSize
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6

        color: root.selected ? StudioTheme.Values.themeInteraction : StudioTheme.Values.themeTextColor
    }

    Rectangle { // strip
        width: root.width
        height: 4
        color: root.selected ? StudioTheme.Values.themeInteraction : "transparent"
        anchors.bottom: parent.bottom
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
