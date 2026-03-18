// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioTheme as StudioTheme
import AiAssistant
import AiAssistantBackend

RowLayout {
    id: root

    required property int messageType
    required property string content
    required property var segments
    required property string toolName
    required property string serverName
    required property var pendingIndices
    required property bool resolved

    readonly property bool hasSegments: root.messageType === ChatMessage.AssistantMessage
                                        && root.segments?.length > 0

    readonly property bool isAssistant: root.messageType === ChatMessage.AssistantMessage
    readonly property bool isUser: root.messageType === ChatMessage.UserMessage

    width: ListView.view.width
    spacing: 0

    function copyContent() {
        const t = Qt.createQmlObject(
            'import QtQuick; TextEdit { visible: false }',
            root, "clipHelper")
        t.text = root.content.replace(/<[^>]*>/g, "")
        t.selectAll()
        t.copy()
        t.destroy()
        copyDoneTimer.restart()
    }

    Timer {
        id: copyDoneTimer

        interval: 1500
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.rightMargin: 5
        Layout.leftMargin: root.isUser ? 40 : 0
        spacing: 0

        HoverHandler { id: bubbleHover }

        Rectangle {
            id: bubble

            Layout.fillWidth: true
            implicitHeight: bubbleColumn.implicitHeight + 16
            radius: StudioTheme.Values.smallRadius

            color: root.isUser
                   ? StudioTheme.Values.themeSubPanelBackground
                   : root.messageType === ChatMessage.IterationMessage
                   ? "transparent"
                   : StudioTheme.Values.themeControlBackground_toolbarIdle

            border.width: 0

            // Inline copy button for user messages — overlaid top-right of bubble
            AiIconButton {
                id: userCopyButton

                visible: root.isUser && bubbleHover.hovered
                z: 1

                anchors.top: bubble.top
                anchors.right: bubble.right
                anchors.margins: 4

                buttonIcon: copyDoneTimer.running
                            ? StudioTheme.Constants.tickMark_small
                            : StudioTheme.Constants.copy_small
                tooltip: qsTr("Copy")
                onClicked: root.copyContent()
            }

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
                    visible: root.isUser
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
                    selectionColor: StudioTheme.Values.themeInteraction
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
                    selectionColor: StudioTheme.Values.themeInteraction
                }

                // Confirmation prompt for destructive tool calls (e.g. delete_qml)
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.messageType === ChatMessage.ToolCallConfirmation
                    spacing: 8

                    TextEdit {
                        Layout.fillWidth: true
                        text: root.content
                        textFormat: TextEdit.PlainText
                        wrapMode: TextEdit.Wrap
                        font.pixelSize: StudioTheme.Values.baseFontSize
                        readOnly: true
                        selectByKeyboard: true
                        color: StudioTheme.Values.themeTextColor
                        selectionColor: StudioTheme.Values.themeInteraction
                    }

                    Row {
                        id: confirmRow

                        spacing: 4
                        visible: !root.resolved

                        PromptButton {
                            id: yesButton

                            label: qsTr("Yes")

                            onClicked: {
                                AiAssistantBackend.rootView.confirmToolsCall(root.pendingIndices, true)
                            }
                        }

                        PromptButton {
                            id: noButton

                            label: qsTr("No")

                            onClicked: {
                                AiAssistantBackend.rootView.confirmToolsCall(root.pendingIndices, false)
                            }
                        }
                    }
                }

                // Assistant message — flat HTML fallback when no segments
                ChatProseSegment {
                    Layout.fillWidth: true
                    visible: root.isAssistant && !root.hasSegments
                    html: root.content
                }

                // Assistant message — segmented (prose + code blocks)
                Repeater {
                    model: root.hasSegments ? root.segments : []

                    delegate: Loader {
                        id: segLoader

                        readonly property var seg: modelData

                        Layout.fillWidth: true
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

        // Hover action bar — assistant messages only
        Row {
            id: actionBar

            visible: root.isAssistant
            opacity: bubbleHover.hovered ? 1.0 : 0.0
            spacing: 2
            Layout.topMargin: 2

            AiIconButton {
                id: likeButton

                buttonIcon: StudioTheme.Constants.thumbs_up_medium
                tooltip: qsTr("Like")
                opacity: root.thumbFeedback === 0 ? 0.3 : 1.0
                onClicked: {
                    if (checked)
                        return
                    checked = true
                    dislikeButton.checked = false
                    AiAssistantBackend.rootView.sendThumbFeedback(true)
                }
            }

            AiIconButton {
                id: dislikeButton

                buttonIcon: StudioTheme.Constants.thumbs_down_medium
                tooltip: qsTr("Dislike")
                opacity: root.thumbFeedback === 1 ? 0.3 : 1.0
                onClicked: {
                    if (checked)
                        return
                    checked = true
                    likeButton.checked = false
                    AiAssistantBackend.rootView.sendThumbFeedback(false)
                }
            }

            AiIconButton {
                buttonIcon: StudioTheme.Constants.copy_small
                tooltip: qsTr("Copy")
                onClicked: root.copyContent()
            }
        }
    }
}




