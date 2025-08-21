// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import QtQuick.Controls.Basic as Basic
import AiAssistantBackend

Rectangle {
    id: root

    required property var rootView

    property StudioTheme.ControlStyle style: StudioTheme.ControlStyle {
        radius: StudioTheme.Values.smallRadius
    }

    color: root.style.background.idle
    border.color: root.style.border.idle
    border.width: root.style.borderWidth
    radius: root.style.radius

    function send() {
        let trimmed = textEdit.text.trim()
        if (trimmed !== "")
            root.rootView.handleMessage(textEdit.text)
        textEdit.text = ""
    }

    RowLayout {
        anchors.fill: parent

        Basic.TextArea {
            id: textEdit

            Layout.fillHeight: true
            Layout.fillWidth: true

            padding: root.style.inputHorizontalPadding
            clip: true
            font.pixelSize: root.style.baseFontSize
            color: root.style.text.idle
            wrapMode: TextEdit.WordWrap

            placeholderText: qsTr("Describe what you want to generate...")
            placeholderTextColor: root.style.text.placeholder

            Keys.onPressed: function(event) {
                if (event.isAutoRepeat) {
                    event.accepted = false
                    return
                }

                switch (event.key) {
                case Qt.Key_Return:
                case Qt.Key_Enter: {
                    if (!(event.modifiers & Qt.ShiftModifier)){
                        root.send()
                        event.accepted = true
                    } else {
                        event.accepted = false
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

        HelperWidgets.IconButton {
            id: sendButton
            objectName: "SendButton"

            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
            Layout.margins: StudioTheme.Values.marginTopBottom

            icon: root.rootView.isGenerating && !sendButton.enabled
                  ? StudioTheme.Constants.more_medium
                  : StudioTheme.Constants.selectFill_medium

            iconColor: sendButton.enabled ? root.style.interaction
                                          : root.style.icon.disabled

            tooltip: qsTr("Send")
            enabled: textEdit.text !== "" && !root.rootView.isGenerating

            onClicked: root.send()
        }
    }
}
