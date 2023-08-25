// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuickDesignerTheme 1.0
import Qt.labs.platform as PlatformWidgets
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

StudioControls.Dialog {
    id: root

    title: qsTr("Import Collections")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true

    required property var backendValue
    property bool fileExists: false

    onOpened: {
        fileName.text = qsTr("New Json File")
        fileName.selectAll()
        fileName.forceActiveFocus()
    }

    onRejected: {
        fileName.text = ""
    }

    HelperWidgets.RegExpValidator {
        id: fileNameValidator
        regExp: /^(\w[^*><?|]*)[^/\\:*><?|]$/
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

    contentItem: Column {
        spacing: 2

        Row {
            spacing: 10
            Text {
                text: qsTr("File name: ")
                anchors.verticalCenter: parent.verticalCenter
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: fileName

                anchors.verticalCenter: parent.verticalCenter
                actionIndicator.visible: false
                translationIndicator.visible: false
                validator: fileNameValidator

                Keys.onEnterPressed: btnCreate.onClicked()
                Keys.onReturnPressed: btnCreate.onClicked()
                Keys.onEscapePressed: root.reject()

                onTextChanged: {
                    root.fileExists = root.backendValue.isJsonFile(fileName.text)
                }
            }

            HelperWidgets.Button {
                id: fileDialogButton
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Open")
                onClicked: fileDialog.open()
            }
        }

        Text {
            text: qsTr("File name cannot be empty.")
            color: StudioTheme.Values.themeTextColor
            anchors.right: parent.right
            visible: fileName.text === ""
        }

        Item { // spacer
            width: 1
            height: 20
        }

        Row {
            anchors.right: parent.right
            spacing: 10

            HelperWidgets.Button {
                id: btnCreate
                anchors.verticalCenter: parent.verticalCenter

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
                anchors.verticalCenter: parent.verticalCenter
                onClicked: root.reject()
            }
        }
    }
}
