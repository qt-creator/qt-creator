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
    required property var segments
    required property string toolName
    required property string serverName
    required property bool success

    width: ListView.view.width
    spacing: 0

    readonly property bool hasSegments: root.messageType === ChatMessage.AssistantMessage
                                        && root.segments?.length > 0

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

        ColumnLayout {
            id: bubbleColumn
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: 8
            }
            spacing: 4

            // User message — plain prose
            ChatProseSegment {
                Layout.fillWidth: true
                visible: root.messageType === ChatMessage.UserMessage
                html: root.content
                color: StudioTheme.Values.themeTextColor
            }

            // System / iteration message — plain text, dimmed and italic
            TextEdit {
                Layout.fillWidth: true
                visible: root.messageType === ChatMessage.SystemMessage
                      || root.messageType === ChatMessage.IterationMessage

                text: root.content
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                font.pixelSize: StudioTheme.Values.baseFontSize
                font.italic: true
                readOnly: true
                selectByKeyboard: true
                color: StudioTheme.Values.themeTextColorDisabled
            }

            // Tool call message — plain text
            TextEdit {
                Layout.fillWidth: true
                visible: root.messageType === ChatMessage.ToolCallStarted
                      || root.messageType === ChatMessage.ToolCallCompleted
                      || root.messageType === ChatMessage.ToolCallFailed

                text: root.content
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                font.pixelSize: StudioTheme.Values.baseFontSize
                readOnly: true
                selectByKeyboard: true
                color: StudioTheme.Values.themeTextColor
            }

            // Assistant message — flat HTML fallback when no segments
            ChatProseSegment {
                Layout.fillWidth: true
                visible: root.messageType === ChatMessage.AssistantMessage && !root.hasSegments
                html: root.content
            }

            // Assistant message — segmented (prose + code blocks)
            Repeater {
                model: root.hasSegments ? root.segments : []

                delegate: Loader {
                    id: segLoader
                    Layout.fillWidth: true
                    readonly property var seg: modelData
                    sourceComponent: seg.type === "code" ? codeSegComp : proseSegComp

                    Component {
                        id: proseSegComp
                        ChatProseSegment {
                            html: segLoader.seg.html ?? ""
                        }
                    }

                    Component {
                        id: codeSegComp
                        ChatCodeSegment {
                            code: segLoader.seg.code ?? ""
                            language: segLoader.seg.language ?? ""
                        }
                    }
                }
            }
        }
    }
}
