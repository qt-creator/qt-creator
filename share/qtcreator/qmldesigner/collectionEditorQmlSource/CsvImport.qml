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

    title: qsTr("Import A CSV File")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true

    required property var backendValue

    property bool fileExists: false

    onOpened: {
        collectionName.text = "Collection_"
        fileName.text = qsTr("New CSV File")
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

    component Spacer: Item {
        implicitWidth: 1
        implicitHeight: StudioTheme.Values.columnGap
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

                onTextChanged: root.fileExists = root.backendValue.isCsvFile(fileName.text)
            }

            HelperWidgets.Button {
                id: fileDialogButton

                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                text: qsTr("Open")
                onClicked: fileDialog.open()
            }
        }

        Spacer {}

        Text {
            text: qsTr("The model name: ")
            color: StudioTheme.Values.themeTextColor
        }

        StudioControls.TextField {
            id: collectionName

            Layout.fillWidth: true

            actionIndicator.visible: false
            translationIndicator.visible: false
            validator: RegularExpressionValidator {
                regularExpression: /^\w+$/
            }

            Keys.onEnterPressed: btnCreate.onClicked()
            Keys.onReturnPressed: btnCreate.onClicked()
            Keys.onEscapePressed: root.reject()
        }

        Spacer { implicitHeight: StudioTheme.Values.controlLabelGap }

        Label {
            id: fieldErrorText

            Layout.fillWidth: true

            color: StudioTheme.Values.themeTextColor
            wrapMode: Label.WordWrap
            padding: 5

            background: Rectangle {
                color: "transparent"
                border.width: StudioTheme.Values.border
                border.color: StudioTheme.Values.themeWarning
            }

            states: [
                State {
                    name: "default"
                    when: fileName.text !== "" && collectionName.text !== ""

                    PropertyChanges {
                        target: fieldErrorText
                        text: ""
                        visible: false
                    }
                },
                State {
                    name: "fileError"
                    when: fileName.text === ""

                    PropertyChanges {
                        target: fieldErrorText
                        text: qsTr("File name can not be empty")
                        visible: true
                    }
                },
                State {
                    name: "collectionNameError"
                    when: collectionName.text === ""

                    PropertyChanges {
                        target: fieldErrorText
                        text: qsTr("The model name can not be empty")
                        visible: true
                    }
                }
            ]
        }

        Spacer {}

        RowLayout {
            spacing: StudioTheme.Values.sectionRowSpacing

            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

            HelperWidgets.Button {
                id: btnCreate

                text: qsTr("Import")
                enabled: root.fileExists && collectionName.text !== ""

                onClicked: {
                    let csvLoaded = root.backendValue.loadCsvFile(fileName.text, collectionName.text)

                    if (csvLoaded)
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
