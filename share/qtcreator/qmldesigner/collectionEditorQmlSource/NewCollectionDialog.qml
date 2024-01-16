// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as PlatformWidgets
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import CollectionEditor

StudioControls.Dialog {
    id: root

    enum SourceType { NewJson, NewCsv, ExistingCollection }

    required property var backendValue
    required property var sourceModel

    readonly property bool isValid: collectionName.isValid

    title: qsTr("Add a new Model")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true

    onOpened: {
        collectionName.text = qsTr("Model")
        updateCollectionExists()
    }

    onRejected: {
        collectionName.text = ""
    }

    onAccepted: {
        if (root.isValid)
            root.backendValue.addCollectionToDataStore(collectionName.text);
    }

    function updateCollectionExists() {
        collectionName.alreadyExists = sourceModel.collectionExists(backendValue.dataStoreNode(),
                                                                    collectionName.text)
    }

    component NameField: Text {
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
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

    component Spacer: Item {
        Layout.minimumWidth: 1
        Layout.preferredHeight: StudioTheme.Values.columnGap
    }

    contentItem: ColumnLayout {
        spacing: 5

        NameField {
            text: qsTr("The model name")
            visible: collectionName.enabled
        }

        StudioControls.TextField {
            id: collectionName

            readonly property bool isValid: !collectionName.enabled
                                            || (collectionName.text !== "" && !collectionName.alreadyExists)
            property bool alreadyExists

            Layout.fillWidth: true

            visible: collectionName.enabled
            actionIndicator.visible: false
            translationIndicator.visible: false
            validator: RegularExpressionValidator {
                regularExpression: /^[\w ]+$/
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

        Spacer { visible: collectionName.visible }

        RowLayout {
            spacing: StudioTheme.Values.sectionRowSpacing
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
