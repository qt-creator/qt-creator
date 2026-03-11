// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioTheme as StudioTheme
import AiAssistant

RowLayout {
    id: root

    required property int messageType
    required property string content
    required property string toolName
    required property string serverName
    required property bool success

    width: ListView.view.width
    spacing: 0

    Rectangle {
        Layout.fillWidth: true
        Layout.rightMargin: 5
        Layout.leftMargin: root.messageType === ChatMessage.UserMessage ? 40 : 0

        implicitHeight: bubbleColumn.implicitHeight + 16
        radius: StudioTheme.Values.smallRadius

        color: root.messageType === ChatMessage.UserMessage
               ? StudioTheme.Values.themeSubPanelBackground
               : root.messageType === ChatMessage.IterationMessage
               ? "transparent"
               : StudioTheme.Values.themeControlBackground_toolbarIdle

        border.width: 0

        // Content layout
        ColumnLayout {
            id: bubbleColumn
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: 8
            }
            spacing: 4

            // Content
            TextEdit {
                id: textContent
                Layout.fillWidth: true

                text: root.content
                textFormat: root.messageType === ChatMessage.AssistantMessage ? TextEdit.RichText
                                                                              : TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                font.pixelSize: StudioTheme.Values.baseFontSize

                readOnly: true
                selectByKeyboard: true

                color: root.messageType === ChatMessage.SystemMessage
                    || root.messageType === ChatMessage.IterationMessage
                        ? StudioTheme.Values.themeTextColorDisabled
                        : StudioTheme.Values.themeTextColor

                font.italic: root.messageType === ChatMessage.SystemMessage
                          || root.messageType === ChatMessage.IterationMessage

                // Open links that the LLM may include in its responses
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
            }
        }
    }
}
