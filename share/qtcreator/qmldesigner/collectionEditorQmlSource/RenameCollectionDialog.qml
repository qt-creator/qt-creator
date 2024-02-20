// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

StudioControls.Dialog {
    id: root

    required property string collectionName
    readonly property alias newCollectionName: newNameField.text
    readonly property bool isValid: newNameField.text !== ""

    title: qsTr("Rename model")

    onOpened: {
        newNameField.text = root.collectionName
        newNameField.forceActiveFocus()
    }

    function acceptIfVerified() {
        if (root.isValid)
            root.accept()
    }

    contentItem: ColumnLayout {
        id: renameDialogContent

        spacing: 2

        Text {
            text: qsTr("Previous name: " + root.collectionName)
            color: StudioTheme.Values.themeTextColor
        }

        Spacer {}

        Text {
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            text: qsTr("New name:")
            color: StudioTheme.Values.themeTextColor
        }

        StudioControls.TextField {
            id: newNameField

            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.fillWidth: true

            actionIndicator.visible: false
            translationIndicator.visible: false
            validator: RegularExpressionValidator {
                regularExpression: /^\w+$/
            }

            Keys.onEnterPressed: root.acceptIfVerified()
            Keys.onReturnPressed: root.acceptIfVerified()
            Keys.onEscapePressed: root.reject()
        }

        Spacer {}

        RowLayout {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            spacing: StudioTheme.Values.sectionRowSpacing

            HelperWidgets.Button {
                text: qsTr("Rename")
                enabled: root.isValid
                onClicked: root.acceptIfVerified()
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }

    component Spacer: Item {
        implicitWidth: 1
        implicitHeight: StudioTheme.Values.columnGap
    }
}
