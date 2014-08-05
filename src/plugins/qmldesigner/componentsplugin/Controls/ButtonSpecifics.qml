/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
                toolTip:  qsTr("The text shown on the button")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.text
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Checked")
                toolTip: qsTr("The state of the button")
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
                text: qsTr("Checkable")
                toolTip: qsTr("Determines whether the button is checkable or not.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.checkable.valueToString
                    backendValue: backendValues.checkable
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Enabled")
                toolTip: qsTr("Determines whether the button is enabled or not.")
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
                toolTip: qsTr("Sets the button as the default button in a dialog.")
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
                toolTip: qsTr("The tool tip shown for the button.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.tooltip
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Focus on press")
                toolTip: "Determines whether the button gets focus if pressed."
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
                toolTip: qsTr("The URL of an icon resource.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.iconSource
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }


        }
    }
}
