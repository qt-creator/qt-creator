// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioTheme 1.0 as StudioTheme
import StudioControls 1.0 as StudioControls
import HelperWidgets 2.0 as HelperWidgets

StudioControls.Dialog {
    id: root

    required property var model
    property int __propertyIndex: -1
    property string __oldName

    title: qsTr("Edit Property")
    clip: true

    function editProperty(index, initialPosition) {
        root.__propertyIndex = index

        if (root.__propertyIndex < 0)
            return

        let previousName = root.model.propertyName(root.__propertyIndex)
        let previousType = root.model.propertyType(root.__propertyIndex)

        root.__oldName = previousName
        newNameField.text = previousName

        propertyType.initialType = previousType

        forceChangeType.checked = false

        let newPoint = mapFromGlobal(initialPosition.x, initialPosition.y)
        x = newPoint.x
        y = newPoint.y

        root.open()
    }

    onAccepted: {
        if (newNameField.text !== "" && newNameField.text !== root.__oldName)
            root.model.renameColumn(root.__propertyIndex, newNameField.text)

        if (propertyType.changed || forceChangeType.checked)
            root.model.setPropertyType(root.__propertyIndex, propertyType.currentText, forceChangeType.checked)
    }

    onRejected: {
        let currentDatatype = propertyType.initialType
        propertyType.currentIndex = propertyType.find(currentDatatype)
    }

    component Spacer: Item {
        implicitWidth: 1
        implicitHeight: StudioTheme.Values.columnGap
    }

    contentItem: ColumnLayout {
        spacing: 2

        Text {
            text: qsTr("Name")
            color: StudioTheme.Values.themeTextColor
        }

        StudioControls.TextField {
            id: newNameField

            Layout.fillWidth: true

            actionIndicator.visible: false
            translationIndicator.visible: false

            Keys.onEnterPressed: root.accept()
            Keys.onReturnPressed: root.accept()
            Keys.onEscapePressed: root.reject()

            validator: RegularExpressionValidator {
                regularExpression: /^\w+$/
            }

            onTextChanged: {
                editButton.enabled = newNameField.text !== ""
            }
        }

        Spacer {}

        Text {
            text: qsTr("Type")
            color: StudioTheme.Values.themeTextColor
        }

        StudioControls.ComboBox {
            id: propertyType

            Layout.fillWidth: true

            property string initialType
            readonly property bool changed: propertyType.initialType !== propertyType.currentText

            model: root.model.typesList()
            actionIndicatorVisible: false

            onInitialTypeChanged: propertyType.currentIndex = propertyType.find(initialType)
        }

        Spacer {}

        RowLayout {
            spacing: StudioTheme.Values.sectionRowSpacing

            StudioControls.CheckBox {
                id: forceChangeType
                actionIndicatorVisible: false
            }

            Text {
                text: qsTr("Force update values")
                color: StudioTheme.Values.themeTextColor
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            }
        }

        Spacer {
            visible: warningBox.visible
            implicitHeight: StudioTheme.Values.controlLabelGap
        }

        Rectangle {
            id: warningBox

            Layout.fillWidth: true
            Layout.preferredHeight: warning.height

            visible: propertyType.initialType !== propertyType.currentText
            color: "transparent"
            clip: true
            border.color: StudioTheme.Values.themeWarning

            RowLayout {
                id: warning

                width: parent.width

                HelperWidgets.IconLabel {
                    icon: StudioTheme.Constants.warning_medium
                    Layout.leftMargin: 10
                }

                Text {
                    text: qsTr("Conversion from %1 to %2 may lead to irreversible data loss")
                        .arg(propertyType.initialType)
                        .arg(propertyType.currentText)

                    color: StudioTheme.Values.themeTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.margins: 8
                }
            }
        }

        Spacer {}

        RowLayout {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            spacing: StudioTheme.Values.sectionRowSpacing

            HelperWidgets.Button {
                id: editButton

                text: qsTr("Edit")
                onClicked: root.accept()
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
