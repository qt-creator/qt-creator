// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

StudioControls.Dialog {
    id: root

    title: qsTr("Add a new Collection")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true

    required property var backendValue

    onOpened: {
        collectionName.text = "Collection"
    }

    onRejected: {
        collectionName.text = ""
    }

    onAccepted: {
        if (collectionName.text !== "")
            root.backendValue.addCollection(collectionName.text)
    }

    contentItem: Column {
        spacing: 10
        Row {
            spacing: 10
            Text {
                text: qsTr("Collection name: ")
                anchors.verticalCenter: parent.verticalCenter
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: collectionName

                anchors.verticalCenter: parent.verticalCenter
                actionIndicator.visible: false
                translationIndicator.visible: false
                validator: HelperWidgets.RegExpValidator {
                    regExp: /^\w+$/
                }

                Keys.onEnterPressed: btnCreate.onClicked()
                Keys.onReturnPressed: btnCreate.onClicked()
                Keys.onEscapePressed: root.reject()
            }
        }

        Text {
            id: fieldErrorText
            color: StudioTheme.Values.themeTextColor
            anchors.right: parent.right
            text: qsTr("Collection name can not be empty")
            visible: collectionName.text === ""
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

                text: qsTr("Create")
                enabled: collectionName.text !== ""
                onClicked: root.accept()
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                anchors.verticalCenter: parent.verticalCenter
                onClicked: root.reject()
            }
        }
    }
}
