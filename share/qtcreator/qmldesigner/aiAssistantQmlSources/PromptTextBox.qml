// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Basic
import StudioControls as StudioControls
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import AiGenerationState
import AiAssistantBackend

Rectangle {
    id: root

    required property var rootView

    property alias text: textEdit.text

    property var startPlaceholderOptions: [
        qsTr("What would you like to create today?"),
        qsTr("What would you like to design today?"),
        qsTr("What should we build today?"),
        qsTr("What are we creating today?"),
        qsTr("What should we work on?"),
        qsTr("Got an idea? Let’s build it."),
        qsTr("Have an idea? Let’s create it."),
        qsTr("Tell me what you’d like to create."),
        qsTr("What would you like to start with?"),
        qsTr("Let’s create something. What’s your idea?"),
        qsTr("What should we design together?")
    ]
    property string startPlaceholder: startPlaceholderOptions[Math.floor(Math.random() * startPlaceholderOptions.length)]

    readonly property bool isGenerating: root.rootView.generationState !== GenerationState.Idle

    property var style: StudioTheme.ControlStyle {
        radius: StudioTheme.Values.smallRadius
    }

    signal modelChanged(string modelName)

    color: root.style.background.idle
    border.color: StudioTheme.Values.themeControlOutlineHover
    border.width: root.style.borderWidth
    radius: root.style.radius

    function send() {
        let trimmed = textEdit.text.trim()
        if (trimmed !== "")
            root.rootView.handleMessage(textEdit.text)
        textEdit.text = ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        HelperWidgets.ScrollView {
            id: scrollView

            clip: true
            hideHorizontalScrollBar: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.rightMargin: 2
            Layout.topMargin: 2

            Basic.TextArea {
                id: textEdit

                width: scrollView.width

                font.pixelSize: root.style.baseFontSize
                color: root.style.text.idle
                wrapMode: TextEdit.WordWrap
                enabled: !root.isGenerating && root.rootView.hasValidModel && root.rootView.termsAccepted

                placeholderText: enabled ? root.rootView.chatHistory.isEmpty ? root.startPlaceholder
                                                                             : qsTr("Enter a message...")
                                         : ""
                placeholderTextColor: root.style.text.placeholder

                Keys.onPressed: function(event) {
                    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                            && !(event.modifiers & Qt.ShiftModifier)) {
                        root.send()
                        event.accepted = true
                    }
                }
            }
        }

        RowLayout {
            Layout.margins: StudioTheme.Values.marginTopBottom
            Layout.alignment: Qt.AlignRight

            StudioControls.ComboBox {
                id: modelComboBox

                readonly property real minWidth: 20
                property real fitWidth: 100

                function updateWidth() {
                    let maxWidth = 0
                    for (let i = 0; i < count; ++i) {
                        metrics.text = textAt(i)
                        maxWidth = Math.max(maxWidth, metrics.width)
                    }
                    if (maxWidth > 0)
                        modelComboBox.fitWidth = maxWidth + 40 // 40: arrow area
                }

                TextMetrics {
                    id: metrics
                    font: modelComboBox.font
                }

                Layout.preferredWidth: modelComboBox.fitWidth
                Layout.minimumWidth: modelComboBox.minWidth
                Layout.maximumWidth: modelComboBox.fitWidth
                Layout.fillWidth: true

                model: AiAssistantBackend.rootView.modelsModel
                textRole: "modelName"
                actionIndicatorVisible: false
                enabled: modelComboBox.count > 0

                Binding on currentIndex {
                    value: modelComboBox.model.selectedIndex
                    delayed: true
                }

                onCurrentIndexChanged: {
                    modelComboBox.model.selectedIndex = modelComboBox.currentIndex
                    root.modelChanged(currentText)
                }

                onCountChanged: modelComboBox.updateWidth()

                Connections {
                    target: modelComboBox.model
                    function onModelReset() { modelComboBox.updateWidth() }
                }
            }

            AiIconButton {
                id: attachImageButton
                objectName: "AttachImageButton"

                buttonIcon: StudioTheme.Constants.link_medium
                tooltip: qsTr("Attach an image.\nThe attached image will be included in the prompt for the AI to analyze and use its content in the response generation.")
                enabled: !root.isGenerating && root.rootView.hasValidModel && root.rootView.termsAccepted

                onClicked: {
                    if (assetImagesView.isTransitioning) return;

                    if (assetImagesView.visible)
                        assetImagesView.hideWindow();
                    else
                        assetImagesView.showWindow();

                    // Lock the button briefly so the "Window Close" event and the "Button Click"
                    // event don't fight.
                    assetImagesView.isTransitioning = true;
                    debounceTimer.start();
                }
            }

            AiIconButton {
                id: sendButton
                objectName: "SendButton"

                buttonIcon: root.isGenerating ? StudioTheme.Constants.stop_medium
                                              : StudioTheme.Constants.send_medium
                tooltip: root.isGenerating ? qsTr("Stop generation") : qsTr("Send")
                enabled: root.isGenerating || textEdit.text !== ""

                onClicked: {
                    if (root.isGenerating)
                        root.rootView.cancelRequest()
                    else
                        root.send()
                }
            }
        }
    }

    AssetImagesPopup {
        id: assetImagesView

        snapItem: attachImageButton

        property bool isTransitioning: false

        Timer {
            id: debounceTimer

            interval: 100
            onTriggered: assetImagesView.isTransitioning = false
        }

        onVisibleChanged: {
            if (!assetImagesView.visible) {
                assetImagesView.isTransitioning = true;
                debounceTimer.restart();
            }
        }

        onWindowShown: {
            if (!assetImagesView.model.includes(root.rootView.attachedImageSource))
                root.rootView.attachedImageSource = ""
        }

        onImageClicked: (imageSource) => root.rootView.attachedImageSource = imageSource
    }
}
