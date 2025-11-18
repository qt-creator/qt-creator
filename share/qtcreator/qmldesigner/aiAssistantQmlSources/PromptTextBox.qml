// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Basic
import StudioControls as StudioControls
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import AiAssistantBackend
import "../misc"

Rectangle {
    id: root

    required property var rootView

    property alias text: textEdit.text
    property alias enableAttachImage: attachImageButton.visible

    property StudioTheme.ControlStyle style: StudioTheme.ControlStyle {
        radius: StudioTheme.Values.smallRadius
    }

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
                enabled: !root.rootView.isGenerating && root.rootView.hasValidModel && root.rootView.termsAccepted

                placeholderText: enabled ? qsTr("Describe what you want to generate...") : ""
                placeholderTextColor: root.style.text.placeholder

                Keys.onPressed: function(event) {
                    switch (event.key) {
                    case Qt.Key_Return:
                    case Qt.Key_Enter: {
                        if (!(event.modifiers & Qt.ShiftModifier)) {
                            root.send()
                            event.accepted = true
                        }
                    } break
                    // TODO: Up/Down keys navigate between lines in the multiline text field.
                    // This should be replaced with a new history navigation method.
                    // case Qt.Key_Up: {
                    //     textEdit.text = root.rootView.getPreviousCommand()
                    //     event.accepted = true
                    // } break
                    // case Qt.Key_Down: {
                    //     textEdit.text = root.rootView.getNextCommand()
                    //     event.accepted = true
                    // } break
                    default:
                        event.accepted = false
                    }
                }
            }
        }

        Row {
            Layout.alignment: Qt.AlignRight
            Layout.margins: StudioTheme.Values.marginTopBottom

            AiIconButton {
                id: attachImageButton
                objectName: "AttachImageButton"

                buttonIcon: StudioTheme.Constants.link_medium
                tooltip: qsTr("Attach an image.\nThe attached image will be included in the prompt for the AI to analyze and use its content in the response generation.")
                enabled: !root.rootView.isGenerating && root.rootView.hasValidModel && root.rootView.termsAccepted

                onClicked: assetImagesView.showWindow()
            }

            AiIconButton {
                id: sendButton
                objectName: "SendButton"

                buttonIcon: StudioTheme.Constants.send_medium
                tooltip: qsTr("Send")
                enabled: textEdit.text !== "" && !root.rootView.isGenerating

                onClicked: root.send()
            }
        }
    }

    Row {
        x: 10
        y: 2
        visible: root.rootView.isGenerating

        Text {
            text: qsTr("Generating...")
            color: StudioTheme.Values.themeTextColor
            anchors.verticalCenter: parent.verticalCenter
        }

        BusyIndicator {
            radius: 16
            dotSize: 4
            color: StudioTheme.Values.themeTextColor
        }
    }

    WarningMessage {
        id: termsWarning

        visible: !root.rootView.termsAccepted
        message: qsTr("Before using the AI Assistant you must read and accept the terms and conditions.")
        buttonLabel: qsTr("Read and accept")
        onAction: root.rootView.openTermsDialog()
    }

    WarningMessage {
        id: noValidModelWarning

        visible: !root.rootView.hasValidModel && root.rootView.termsAccepted
        message: qsTr("No model configured. To use the AI assistant, add an API key in the AI Provider settings.")
        buttonLabel: qsTr("Configure now")
        onAction: root.rootView.openModelSettings()
    }

    AssetImagesPopup {
        id: assetImagesView

        snapItem: attachImageButton

        onWindowShown: {
            if (!assetImagesView.model.includes(root.rootView.attachedImageSource))
                root.rootView.attachedImageSource = ""
        }

        onImageClicked: (imageSource) => root.rootView.attachedImageSource = imageSource
    }
}
