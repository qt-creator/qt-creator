// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StudioTheme as StudioTheme
import AiGenerationState
import AiAssistantBackend

Item {
    id: root

    property var rootView: AiAssistantBackend.rootView

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: StudioTheme.Values.marginTopBottom

        PresetPrompts {
            Layout.fillWidth: true

            onTriggered: (prompt) => promptTextBox.text = prompt
        }

        SplitView {
            id: splitView

            Layout.fillWidth: true
            Layout.fillHeight: true

            orientation: Qt.Vertical

            handle: Rectangle {
                implicitHeight: 8
                color: SplitHandle.hovered || SplitHandle.pressed
                       ? StudioTheme.Values.themeControlOutlineHover
                       : "transparent"

                Text {
                    color: StudioTheme.Values.themeTextColorDisabled
                    font.family: StudioTheme.Constants.iconFont.family
                    text: StudioTheme.Constants.dragmarks
                    rotation: 90
                    anchors.centerIn: parent
                }
            }

            ChatView {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.minimumHeight: 60

                chatModel: root.rootView.chatHistory
            }

            ColumnLayout {
                SplitView.fillWidth: true
                SplitView.preferredHeight: 120
                SplitView.minimumHeight: 70 + (attachedImage.visible ? attachedImage.height : 0)

                AssetImage {
                    id: attachedImage

                    Layout.alignment: Qt.AlignRight

                    visible: root.rootView.attachedImageSource !== ""
                    closable: true

                    onCloseRequest: {
                        root.rootView.attachedImageSource = ""
                    }
                }

                PromptTextBox {
                    id: promptTextBox

                    rootView: root.rootView

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    onModelChanged: (modelName) => {
                        var supportsImageInput = /llama-4-(maverick|scout)/.test(modelName)
                                                 || modelName.startsWith("gemini")
                                                 || modelName.startsWith("gpt-")

                        promptTextBox.enableAttachImage = supportsImageInput
                        if (!supportsImageInput)
                            root.rootView.attachedImageSource = ""
                    }
                }
            }
        }

        // Bottom bar
        Row {
            Layout.alignment: Qt.AlignRight

            AiIconButton {
                id: undoButton

                buttonIcon: StudioTheme.Constants.undo
                tooltip: qsTr("Undo last AI changes")
                enabled: root.rootView.canUndoTransaction
                         && root.rootView.generationState == GenerationState.Idle

                onClicked: root.rootView.undoTransaction()
            }

            AiIconButton {
                id: clearButton

                buttonIcon: StudioTheme.Constants.close_small

                tooltip: qsTr("Clear conversation")
                enabled: root.rootView.termsAccepted && root.rootView.generationState == GenerationState.Idle

                onClicked: root.rootView.clearChat()
            }

            AiIconButton {
                id: settingsButton
                objectName: "SettingsButton"

                buttonIcon: StudioTheme.Constants.settings_medium

                tooltip: qsTr("Open AI Assistant settings.")
                enabled: root.rootView.termsAccepted && root.rootView.generationState == GenerationState.Idle

                onClicked: root.rootView.openModelSettings()
            }
        }
    }
}
