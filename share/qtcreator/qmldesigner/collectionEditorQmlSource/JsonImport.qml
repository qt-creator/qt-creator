// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme 1.0
import Qt.labs.platform as PlatformWidgets
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

StudioControls.Dialog {
    id: root

    title: qsTr("Import Models")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true

    required property var backendValue
    property bool fileExists: false

    onOpened: {
        fileName.text = qsTr("New JSON File")
        fileName.selectAll()
        fileName.forceActiveFocus()
    }

    onRejected: {
        fileName.text = ""
    }

    RegularExpressionValidator {
        id: fileNameValidator
        regularExpression: /^(\w[^*><?|]*)[^/\\:*><?|]$/
    }

    PlatformWidgets.FileDialog {
        id: fileDialog
        onAccepted: fileName.text = fileDialog.file
    }

    Message {
        id: creationFailedDialog

        title: qsTr("Could not load the file")
        message: qsTr("An error occurred while trying to load the file.")

        onClosed: root.reject()
    }

    contentItem: ColumnLayout {
        spacing: 2

        Text {
            text: qsTr("File name: ")
            color: StudioTheme.Values.themeTextColor
        }

        RowLayout {
            spacing: StudioTheme.Values.sectionRowSpacing

            Layout.fillWidth: true

            StudioControls.TextField {
                id: fileName

                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.fillWidth: true

                actionIndicator.visible: false
                translationIndicator.visible: false
                validator: fileNameValidator

                Keys.onEnterPressed: btnCreate.onClicked()
                Keys.onReturnPressed: btnCreate.onClicked()
                Keys.onEscapePressed: root.reject()

                onTextChanged: root.fileExists = root.backendValue.isJsonFile(fileName.text)
            }

            HelperWidgets.Button {
                id: fileDialogButton

                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                text: qsTr("Open")
                onClicked: fileDialog.open()
            }
        }

        Item { // spacer
            implicitWidth: 1
            implicitHeight: StudioTheme.Values.controlLabelGap
        }

        Label {
            Layout.fillWidth: true

            padding: 5
            text: qsTr("File name cannot be empty.")
            wrapMode: Label.WordWrap
            color: StudioTheme.Values.themeTextColor
            visible: fileName.text === ""

            background: Rectangle {
                color: "transparent"
                border.width: StudioTheme.Values.border
                border.color: StudioTheme.Values.themeWarning
            }
        }

        Item { // spacer
            implicitWidth: 1
            implicitHeight: StudioTheme.Values.sectionColumnSpacing
        }

        RowLayout {
            spacing: StudioTheme.Values.sectionRowSpacing

            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

            HelperWidgets.Button {
                id: btnCreate

                text: qsTr("Import")
                enabled: root.fileExists
                onClicked: {
                    let jsonLoaded = root.backendValue.loadJsonFile(fileName.text)

                    if (jsonLoaded)
                        root.accept()
                    else
                        creationFailedDialog.open()
                }
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
