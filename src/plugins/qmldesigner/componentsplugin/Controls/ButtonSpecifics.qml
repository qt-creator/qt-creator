// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("Button")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Text")
                tooltip:  qsTr("Text displayed on the button.")
            }

            SecondColumnLayout {
                LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.text
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Checked")
                tooltip: qsTr("State of the button.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.checked.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.checked
                    enabled: backendValues.checkable.value
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Checkable")
                tooltip: qsTr("Determines whether the button is checkable or not.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.checkable.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.checkable
                    property bool backEndValueValue: backendValues.checkable.value
                    onTextChanged: {
                        if (!backendValues.checkable.value) {
                            backendValues.checked.resetValue()
                        }
                    }
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Enabled")
                tooltip: qsTr("Determines whether the button is enabled or not.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.enabled.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.enabled
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Default button")
                tooltip: qsTr("Sets the button as the default button in a dialog.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.isDefault.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.isDefault
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Tool tip")
                tooltip: qsTr("The tool tip shown for the button.")
            }

            SecondColumnLayout {
                LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.tooltip
                }


            }

            PropertyLabel {
                text: qsTr("Focus on press")
                tooltip: qsTr("Determines whether the button gets focus if pressed.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.activeFocusOnPress.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.activeFocusOnPress
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Icon source")
                tooltip: qsTr("The URL of an icon resource.")
            }

            SecondColumnLayout {
                LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.iconSource
                }
            }
        }
    }
}
