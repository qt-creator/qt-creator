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
    property string __currentName

    title: qsTr("Edit Column")

    function openDialog(index, initialPosition) {
        root.__propertyIndex = index

        if (root.__propertyIndex < 0)
            return

        root.__currentName = root.model.propertyName(root.__propertyIndex)
        nameTextField.text = root.__currentName
        nameTextField.selectAll()
        nameTextField.forceActiveFocus()

        typeComboBox.initialType = root.model.propertyType(root.__propertyIndex)
        typeComboBox.currentIndex = typeComboBox.find(typeComboBox.initialType)

        let newPoint = mapFromGlobal(initialPosition.x, initialPosition.y)
        x = newPoint.x
        y = newPoint.y

        root.open()
    }

    onWidthChanged: {
        if (visible && x > parent.width)
            root.close()
    }

    onAccepted: {
        if (nameTextField.text !== "" && nameTextField.text !== root.__currentName)
            root.model.renameColumn(root.__propertyIndex, nameTextField.text)

        if (typeComboBox.initialType !== typeComboBox.currentText)
            root.model.setPropertyType(root.__propertyIndex, typeComboBox.currentText)
    }

    contentItem: Column {
        spacing: 5

        Grid {
            columns: 2
            rows: 2
            rowSpacing: 2
            columnSpacing: 25
            verticalItemAlignment: Grid.AlignVCenter

            Text {
                text: qsTr("Name")
                color: StudioTheme.Values.themeTextColor
                verticalAlignment: Text.AlignVCenter
            }

            StudioControls.TextField {
                id: nameTextField

                actionIndicator.visible: false
                translationIndicator.visible: false

                Keys.onEnterPressed: root.accept()
                Keys.onReturnPressed: root.accept()
                Keys.onEscapePressed: root.reject()

                validator: RegularExpressionValidator {
                    regularExpression: /^\w+$/
                }
            }

            Text {
                text: qsTr("Type")
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.ComboBox {
                id: typeComboBox

                property string initialType

                model: root.model.typesList()
                actionIndicatorVisible: false
            }
        }

        Rectangle {
            id: warningBox

            visible: typeComboBox.initialType !== typeComboBox.currentText
            color: "transparent"
            clip: true
            border.color: StudioTheme.Values.themeWarning
            width: parent.width
            height: warning.height

            Row {
                id: warning

                padding: 5
                spacing: 5

                HelperWidgets.IconLabel {
                    icon: StudioTheme.Constants.warning_medium
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: qsTr("Conversion from %1 to %2 may lead to data loss")
                        .arg(typeComboBox.initialType)
                        .arg(typeComboBox.currentText)

                    width: warningBox.width - 20

                    color: StudioTheme.Values.themeTextColor
                    wrapMode: Text.WordWrap
                }
            }
        }

        Row {
            height: 40
            spacing: 5

            HelperWidgets.Button {
                id: editButton

                text: qsTr("Apply")
                enabled: nameTextField.text !== ""
                anchors.bottom: parent.bottom

                onClicked: root.accept()
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                anchors.bottom: parent.bottom

                onClicked: root.reject()
            }
        }
    }
}
