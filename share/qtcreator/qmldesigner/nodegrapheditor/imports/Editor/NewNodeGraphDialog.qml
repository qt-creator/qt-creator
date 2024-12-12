// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

StudioControls.Dialog {
    id: root

    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true
    title: qsTr("Create Node Graph")

    contentItem: Column {
        spacing: 2

        Row {
            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: StudioTheme.Values.themeTextColor
                text: qsTr("Name: ")
            }

            StudioControls.TextField {
                id: tfName

                actionIndicator.visible: false
                translationIndicator.visible: false
                validator: nameValidator

                Keys.onEnterPressed: btnCreate.onClicked()
                Keys.onEscapePressed: root.reject()
                Keys.onReturnPressed: btnCreate.onClicked()
                onTextChanged: () => {}
            }
        }

        Row {
            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: StudioTheme.Values.themeTextColor
                text: qsTr("Type: ")
            }

            StudioControls.ComboBox {
                id: cbType

                actionIndicatorVisible: false
                model: [
                    {
                        value: "principled_material",
                        text: "Principled Material"
                    },
                ]
                textRole: "text"
                valueRole: "value"

                onActivated: () => {}
            }
        }

        Item {
            height: 20
            width: 1
        }

        Row {
            anchors.right: parent.right

            HelperWidgets.Button {
                id: btnCreate

                text: qsTr("Create")

                onClicked: () => {}
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")

                onClicked: root.reject()
            }
        }
    }

    onOpened: () => {}
    onRejected: () => {}

    HelperWidgets.RegExpValidator {
        id: nameValidator

        regExp: /^(\w[^*/><?\\|:]*)$/
    }
}
