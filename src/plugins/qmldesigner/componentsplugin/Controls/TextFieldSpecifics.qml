/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
