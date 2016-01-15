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
        caption: qsTr("Button")

        SectionLayout {

            Label {
                text: qsTr("Text")
                tooltip:  qsTr("Text displayed on the button.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.text
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Checked")
                tooltip: qsTr("State of the button.")
            }

            SecondColumnLayout {
                CheckBox {
                    enabled: backendValues.checkable.value
                    text: backendValues.checked.valueToString
                    backendValue: backendValues.checked
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Checkable")
                tooltip: qsTr("Determines whether the button is checkable or not.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.checkable.valueToString
                    backendValue: backendValues.checkable
                    property bool backEndValueValue: backendValues.checkable.value
                    onTextChanged: {
                        if (!backendValues.checkable.value) {
                            backendValues.checked.resetValue()
                        }
                    }

                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Enabled")
                tooltip: qsTr("Determines whether the button is enabled or not.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.enabled.valueToString
                    backendValue: backendValues.enabled
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }


            Label {
                text: qsTr("Default button")
                tooltip: qsTr("Sets the button as the default button in a dialog.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.isDefault.valueToString
                    backendValue: backendValues.isDefault
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Tool tip")
                tooltip: qsTr("The tool tip shown for the button.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.tooltip
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Focus on press")
                tooltip: qsTr("Determines whether the button gets focus if pressed.")
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


            Label {
                text: qsTr("Icon source")
                tooltip: qsTr("The URL of an icon resource.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.iconSource
                    Layout.fillWidth: true
                }
            }


        }
    }
}
