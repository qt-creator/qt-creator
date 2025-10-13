// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import AiAssistantBackend

Rectangle {
    id: root

    property var rootView: AiAssistantBackend.rootView

    color: StudioTheme.Values.themeBackgroundColorNormal

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: StudioTheme.Values.marginTopBottom

        Flow {
            spacing: 3
            Layout.fillWidth: true

            Repeater {
                model: ["Add a rectangle", "Add a button", "Add a Text", "Create a sample UI", "Remove all objects"]

                delegate: PromptButton {
                    required property string modelData

                    label: modelData
                    enabled: !root.rootView.isGenerating

                    onClicked: promptTextBox.text = modelData
                }
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

            HelperWidgets.IconButton {
                id: settingsButton
                objectName: "SettingsButton"

                icon: StudioTheme.Constants.settings_medium

                iconColor: settingsButton.enabled ? StudioTheme.Values.controlStyle.text.idle
                                                  : StudioTheme.Values.controlStyle.text.disabled

                tooltip: qsTr("Open AI assistant settings.")
                enabled: !root.rootView.isGenerating

                onClicked: root.rootView.openModelSettings()
            }
        }
    }
}
