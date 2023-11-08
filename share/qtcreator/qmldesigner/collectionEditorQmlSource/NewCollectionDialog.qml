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
import CollectionEditor 1.0

StudioControls.Dialog {
    id: root

    enum SourceType { NewJson, NewCsv, ExistingCollection, NewCollectionToJson }

    required property var backendValue
    required property var sourceModel

    readonly property alias collectionType: typeMode.collectionType
    readonly property bool isValid: collectionName.isValid
                                    && jsonCollections.isValid
                                    && newCollectionPath.isValid

    title: qsTr("Add a new Model")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true

    onOpened: {
        collectionName.text = qsTr("Model")
        updateType()
        updateJsonSourceIndex()
        updateCollectionExists()
    }

    onRejected: {
        collectionName.text = ""
    }

    onAccepted: {
        if (root.isValid) {
            root.backendValue.addCollection(collectionName.text,
                                            root.collectionType,
                                            newCollectionPath.text,
                                            jsonCollections.currentValue)

        }
    }

    function updateType() {
        newCollectionPath.text = ""
        if (typeMode.currentValue === NewCollectionDialog.SourceType.NewJson) {
            newCollectionFileDialog.nameFilters = ["JSON Files (*.json)"]
            newCollectionFileDialog.fileMode = PlatformWidgets.FileDialog.SaveFile
            newCollectionPath.enabled = true
            jsonCollections.enabled = false
            typeMode.collectionType = "json"
        } else if (typeMode.currentValue === NewCollectionDialog.SourceType.NewCsv) {
            newCollectionFileDialog.nameFilters = ["Comma-Separated Values (*.csv)"]
            newCollectionFileDialog.fileMode = PlatformWidgets.FileDialog.SaveFile
            newCollectionPath.enabled = true
            jsonCollections.enabled = false
            typeMode.collectionType = "csv"
        } else if (typeMode.currentValue === NewCollectionDialog.SourceType.ExistingCollection) {
            newCollectionFileDialog.nameFilters = ["All Model Group Files (*.json *.csv)",
                                                   "JSON Files (*.json)",
                                                   "Comma-Separated Values (*.csv)"]
            newCollectionFileDialog.fileMode = PlatformWidgets.FileDialog.OpenFile
            newCollectionPath.enabled = true
            jsonCollections.enabled = false
            typeMode.collectionType = "existing"
        } else if (typeMode.currentValue === NewCollectionDialog.SourceType.NewCollectionToJson) {
            newCollectionFileDialog.nameFilters = [""]
            newCollectionPath.enabled = false
            jsonCollections.enabled = true
            typeMode.collectionType = "json"
        }
    }

    function updateJsonSourceIndex() {
        if (!jsonCollections.enabled) {
            jsonCollections.currentIndex = -1
            return
        }

        if (jsonCollections.currentIndex === -1 && jsonCollections.model.rowCount())
            jsonCollections.currentIndex = 0
    }

    function updateCollectionExists() {
        collectionName.alreadyExists = sourceModel.collectionExists(jsonCollections.currentValue,
                                                                    collectionName.text)
    }

    component NameField: Text {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        horizontalAlignment: Qt.AlignRight
        verticalAlignment: Qt.AlignCenter
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: StudioTheme.Values.baseIconFontSize
    }

    component ErrorField: Text {
        Layout.columnSpan: 2
        color: StudioTheme.Values.themeError
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: StudioTheme.Values.baseIconFontSize
    }

    contentItem: ColumnLayout {
        spacing: 10
        GridLayout {
            columns: 2
            rowSpacing: 10

            NameField {
                text: qsTr("Type")
            }

            StudioControls.ComboBox {
                id: typeMode

                property string collectionType

                Layout.minimumWidth: 300
                Layout.fillWidth: true

                model: ListModel {
                    ListElement { text: qsTr("New JSON model group"); value: NewCollectionDialog.SourceType.NewJson}
                    ListElement { text: qsTr("New CSV model"); value: NewCollectionDialog.SourceType.NewCsv}
                    ListElement { text: qsTr("Import an existing model group"); value: NewCollectionDialog.SourceType.ExistingCollection}
                    ListElement { text: qsTr("Add a model to an available JSON model group"); value: NewCollectionDialog.SourceType.NewCollectionToJson}
                }

                textRole: "text"
                valueRole: "value"
                actionIndicatorVisible: false

                onCurrentValueChanged: root.updateType()
            }

            NameField {
                text: qsTr("File location")
                visible: newCollectionPath.enabled
            }

            RowLayout {
                visible: newCollectionPath.enabled

                Text {
                    id: newCollectionPath

                    readonly property bool isValid: !enabled || text !== ""

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    elide: Text.ElideRight
                    font.family: StudioTheme.Constants.font.family
                    font.pixelSize: StudioTheme.Values.baseIconFontSize
                    color: StudioTheme.Values.themePlaceholderTextColor
                }

                HelperWidgets.Button {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    text: qsTr("Select")

                    onClicked: newCollectionFileDialog.open()

                    PlatformWidgets.FileDialog {
                        id: newCollectionFileDialog

                        title: "Select source file"
                        fileMode: PlatformWidgets.FileDialog.OpenFile
                        acceptLabel: fileMode === PlatformWidgets.FileDialog.OpenFile ? qsTr("Open") : qsTr("Add")

                        onAccepted: newCollectionPath.text = newCollectionFileDialog.currentFile
                    }
                }
            }

            ErrorField {
                visible: !newCollectionPath.isValid
                text: qsTr("Select a file to continue")
            }

            NameField {
                text: qsTr("JSON model group")
                visible: jsonCollections.enabled
            }

            StudioControls.ComboBox {
                id: jsonCollections

                readonly property bool isValid: !enabled || currentIndex !== -1

                implicitWidth: 300
                textRole: "sourceName"
                valueRole: "sourceNode"
                visible: enabled
                actionIndicatorVisible: false

                model: CollectionJsonSourceFilterModel {
                    sourceModel: root.sourceModel
                    onRowsInserted: root.updateJsonSourceIndex()
                    onModelReset: root.updateJsonSourceIndex()
                    onRowsRemoved: root.updateJsonSourceIndex()
                }

                onEnabledChanged: root.updateJsonSourceIndex()
                onCurrentValueChanged: root.updateCollectionExists()
            }

            ErrorField {
                visible: !jsonCollections.isValid
                text: qsTr("Add a JSON resource to continue")
            }

            NameField {
                text: qsTr("Model name")
                visible: collectionName.enabled
            }

            StudioControls.TextField {
                id: collectionName

                readonly property bool isValid: !enabled || (text !== "" && !alreadyExists)
                property bool alreadyExists

                visible: enabled
                actionIndicator.visible: false
                translationIndicator.visible: false
                validator: RegularExpressionValidator {
                    regularExpression: /^\w+$/
                }

                Keys.onEnterPressed: btnCreate.onClicked()
                Keys.onReturnPressed: btnCreate.onClicked()
                Keys.onEscapePressed: root.reject()

                onTextChanged: root.updateCollectionExists()
            }

            ErrorField {
                text: qsTr("The model name can not be empty")
                visible: collectionName.enabled && collectionName.text === ""
            }

            ErrorField {
                text: qsTr("The model name already exists %1").arg(collectionName.text)
                visible: collectionName.enabled && collectionName.alreadyExists
            }
        }

        Item { // spacer
            Layout.fillHeight: true
            Layout.preferredWidth: 1
        }

        RowLayout {
            spacing: 10
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom

            HelperWidgets.Button {
                id: btnCreate

                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                text: qsTr("Create")
                enabled: root.isValid
                onClicked: root.accept()
            }

            HelperWidgets.Button {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
