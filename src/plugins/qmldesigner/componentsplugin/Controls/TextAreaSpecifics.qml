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
