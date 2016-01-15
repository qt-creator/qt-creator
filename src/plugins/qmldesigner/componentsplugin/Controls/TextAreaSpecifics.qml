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
        caption: qsTr("Color")


        ColorEditor {
            caption: qsTr("Color")
            backendValue: backendValues.textColor
            supportGradient: false
        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Text Area")

        SectionLayout {

            Label {
                text: qsTr("Text")
                tooltip: qsTr("Text shown on the text area.")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.text
                    Layout.fillWidth: true
                }

            }

            Label {
                text: qsTr("Read only")
                tooltip: qsTr("Determines whether the text area is read only.")
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
                text: qsTr("Document margins")
                tooltip: qsTr("Margins of the text area.")
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.documentMargins
                    minimumValue: 0;
                    maximumValue: 1000;
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }


            Label {
                text: qsTr("Frame width")
                tooltip: qsTr("Width of the frame.")
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.frameWidth
                    minimumValue: 0;
                    maximumValue: 1000;
                    decimals: 0
                }

                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Contents frame")
                tooltip: qsTr("Determines whether the frame around contents is shown.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.frameVisible.valueToString
                    backendValue: backendValues.frameVisible
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

        }

    }

    FontSection {
        showStyle: false
    }


    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Focus Handling")

        SectionLayout {

            Label {
                text: qsTr("Highlight on focus")
                tooltip: qsTr("Determines whether the text area is highlighted on focus.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.highlightOnFocus.valueToString
                    backendValue: backendValues.highlightOnFocus
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Tab changes focus")
                tooltip: qsTr("Determines whether tab changes the focus of the text area.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.tabChangesFocus.valueToString
                    backendValue: backendValues.tabChangesFocus
                    implicitWidth: 180
                }
                ExpandingSpacer {

                }
            }

            Label {
                text: qsTr("Focus on press")
                tooltip: qsTr("Determines whether the text area gets focus if pressed.")
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
