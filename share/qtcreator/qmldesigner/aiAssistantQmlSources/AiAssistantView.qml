// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
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

        AiModelsComboBox {
            Layout.fillWidth: true

            onCurrentTextChanged: {
                var supportsImageInput = /llama-4-(maverick|scout)/.test(currentText)
                                         || currentText.startsWith("gemini")
                                         || currentText.startsWith("gpt-")

                promptTextBox.enableAttachImage = supportsImageInput
                if (!supportsImageInput)
                    root.rootView.attachedImageSource = ""
            }
        }

        PromptTextBox {
            id: promptTextBox

            rootView: root.rootView

            Layout.fillHeight: true
            Layout.fillWidth: true
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

        RowLayout {
            ResponseStatePopup {
                id: responseStatePopup

                Layout.fillWidth: true
            }

            Item { // Spacer
                Layout.fillWidth: true
                visible: !responseStatePopup.visible
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
