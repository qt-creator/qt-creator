// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.TabButton {
    id: myButton

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 4

    background: Rectangle {
        id: buttonBackground
        color: myButton.checked ? StudioTheme.Values.themeInteraction
                                : "transparent"
        border.width: StudioTheme.Values.border
        border.color: StudioTheme.Values.themeInteraction
    }

    contentItem: T.Label {
        id: buttonIcon
        color: myButton.checked ? StudioTheme.Values.themeControlBackground
                                : StudioTheme.Values.themeInteraction
        //font.weight: Font.Bold
        //font.family: StudioTheme.Constants.font.family
        font.pixelSize: StudioTheme.Values.myFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.fill: parent
        renderType: Text.QtRendering

        text: myButton.text
    }
}
