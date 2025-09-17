// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Basic
import StudioControls as StudioControls
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import AiAssistantBackend

Rectangle {
    id: root

    required property var rootView

    property alias text: textEdit.text

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
                enabled: !root.rootView.isGenerating

                placeholderText: qsTr("Describe what you want to generate...")
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

            HelperWidgets.IconButton {
                id: attachImageButton
                objectName: "AttachImageButton"

                icon: StudioTheme.Constants.link_medium

                iconColor: attachImageButton.enabled ? root.style.text.idle
                                                     : root.style.text.disabled

                tooltip: qsTr("Attach an image.\nThe attached image will be included in the prompt for the AI to analyze and use its content in the response generation.")
                enabled: !root.rootView.isGenerating

                onClicked: assetImagesView.showWindow()
            }

            HelperWidgets.IconButton {
                id: sendButton
                objectName: "SendButton"

                icon: StudioTheme.Constants.send_medium

                iconColor: sendButton.enabled ? root.style.text.idle
                                              : root.style.text.disabled

                tooltip: qsTr("Send")
                enabled: textEdit.text !== "" && !root.rootView.isGenerating

                onClicked: root.send()
            }
        }
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
