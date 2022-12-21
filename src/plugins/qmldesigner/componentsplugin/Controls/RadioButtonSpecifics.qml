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
        caption: qsTr("Radio Button")

        SectionLayout {

            Label {
                text: qsTr("Text")
                tooltip: qsTr("Text label for the radio button.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.text
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Checked")
                tooltip: qsTr("Determines whether the radio button is checked or not.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.checked.valueToString
                    backendValue: backendValues.checked
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }


            Label {

                text: qsTr("Focus on press")
                tooltip: qsTr("Determines whether the radio button gets focus if pressed.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.activeFocusOnPress.valueToString
                    backendValue: backendValues.activeFocusOnPress
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

        }
    }
}
