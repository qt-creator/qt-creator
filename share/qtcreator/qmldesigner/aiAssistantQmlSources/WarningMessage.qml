// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import HelperWidgets as HelperWidgets

Column {
    id: root

    property string message
    property alias buttonLabel: actionButton.label
    signal action;

    anchors.centerIn: parent
    spacing: 5
    width: parent.width - 40 // 40: side paddings

    Row {
        spacing: 5
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, messageTextOneLine.width + messageText.x)

        HelperWidgets.IconLabel {
            icon: StudioTheme.Constants.warning_medium
            iconColor: StudioTheme.Values.notification_alertDefault
        }

        Text {
            id: messageText

            text: root.message
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.mediumFontSize
            wrapMode: Text.WordWrap
            width: parent.width - x
        }

        // Used for measuring the width of the text when in a single line
        Text {
            id: messageTextOneLine

            visible: false
            text: root.message
            font.pixelSize: StudioTheme.Values.mediumFontSize
        }
    }

    PromptButton {
        id: actionButton

        onClicked: root.action()
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
