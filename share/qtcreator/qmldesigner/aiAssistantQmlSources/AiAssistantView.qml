// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import StudioTheme as StudioTheme
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

        ChatView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 100
            Layout.preferredHeight: 400

            chatModel: root.rootView.chatHistory
        }

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
            Layout.preferredHeight: 80
            Layout.minimumHeight: 50
            Layout.maximumHeight: 200

            onModelChanged: (modelName) => {
                var supportsImageInput = /llama-4-(maverick|scout)/.test(modelName)
                                         || modelName.startsWith("gemini")
                                         || modelName.startsWith("gpt-")

                promptTextBox.enableAttachImage = supportsImageInput
                if (!supportsImageInput)
                    root.rootView.attachedImageSource = ""
            }
        }

        // Bottom bar
        RowLayout {
            Layout.fillWidth: true

            ResponseStatePopup {
                id: responseStatePopup

                Layout.fillWidth: true
            }

            Item { // Spacer
                Layout.fillWidth: true
                visible: !responseStatePopup.visible
            }

            AiIconButton {
                id: clearButton

                buttonIcon: StudioTheme.Constants.close_small

                tooltip: qsTr("Clear conversation")
                enabled: root.rootView.termsAccepted && !root.rootView.isGenerating

                onClicked: root.rootView.clearChat()
            }

            AiIconButton {
                id: settingsButton
                objectName: "SettingsButton"

                buttonIcon: StudioTheme.Constants.settings_medium

                tooltip: qsTr("Open AI Assistant settings.")
                enabled: root.rootView.termsAccepted && !root.rootView.isGenerating

                onClicked: root.rootView.openModelSettings()
            }
        }
    }
}
