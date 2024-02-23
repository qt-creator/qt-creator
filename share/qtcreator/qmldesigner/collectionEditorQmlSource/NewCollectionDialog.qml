// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import CollectionEditorBackend

StudioControls.Dialog {
    id: root

    property bool nameExists: false
    readonly property bool isValid: collectionName.text !== "" && !root.nameExists

    title: qsTr("Add a new model")

    closePolicy: Popup.CloseOnEscape
    modal: true

    onOpened: {
        collectionName.text = CollectionEditorBackend.model.getUniqueCollectionName()
    }

    onRejected: {
        collectionName.text = ""
    }

    onAccepted: {
        if (root.isValid)
            root.CollectionEditorBackend.rootView.addCollectionToDataStore(collectionName.text);
    }

    ColumnLayout {
        spacing: 5

        NameField {
            text: qsTr("Name")
        }

        StudioControls.TextField {
            id: collectionName

            Layout.fillWidth: true

            actionIndicator.visible: false
            translationIndicator.visible: false
            validator: RegularExpressionValidator {
                regularExpression: /^[\w ]+$/
            }

            Keys.onEnterPressed: btnCreate.onClicked()
            Keys.onReturnPressed: btnCreate.onClicked()
            Keys.onEscapePressed: root.reject()

            onTextChanged: {
                root.nameExists = CollectionEditorBackend.model.collectionExists(collectionName.text)
            }
        }

        ErrorField {
            id: errorField
            text: {
                if (collectionName.text === "")
                    return qsTr("Name can not be empty")
                else if (root.nameExists)
                    return qsTr("Name '%1' already exists").arg(collectionName.text)
                else
                    return ""
            }
        }

        Spacer {}

        Row {
            spacing: StudioTheme.Values.sectionRowSpacing

            HelperWidgets.Button {
                id: btnCreate

                text: qsTr("Create")
                enabled: root.isValid
                onClicked: root.accept()
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }

    component NameField: Text {
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: StudioTheme.Values.baseIconFontSize
    }

    component ErrorField: Text {
        color: StudioTheme.Values.themeError
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: StudioTheme.Values.baseIconFontSize
    }

    component Spacer: Item {
        width: 1
        height: StudioTheme.Values.columnGap
    }
}
