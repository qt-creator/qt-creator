// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Text Field")

        SectionLayout {

            Label {
                text: qsTr("Text")
                tooltip: qsTr("Text shown on the text field.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.text
                    implicitWidth: 180
                    Layout.fillWidth: true

                }
            }

            Label {
                text:  qsTr("Placeholder text")
                tooltip: qsTr("Placeholder text.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.placeholderText
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Read only")
                tooltip: qsTr("Determines whether the text field is read only.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.readOnly.valueToString
                    backendValue: backendValues.readOnly
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Input mask")
                tooltip: qsTr("Restricts the valid text in the text field.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.inputMask
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Echo mode")
                tooltip: qsTr("Specifies how the text is displayed in the text field.")
            }

            SecondColumnLayout {
                ComboBox {
                    useInteger: true
                    backendValue: backendValues.echoMode
                    implicitWidth: 180
                    model:  ["Normal", "Password", "PasswordEchoOnEdit", "NoEcho"]
                    scope: "TextInput"
                }
                ExpandingSpacer {

                }
            }


        }

    }

    FontSection {
        showStyle: false
    }
}
