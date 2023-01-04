// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        caption: qsTr("Combo Box")

        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Text role")
                tooltip: qsTr("The model role used for displaying text.")
            }

            SecondColumnLayout {
                LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.textRole
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Display text")
                tooltip: qsTr("Holds the text that is displayed on the combo box button.")
            }

            SecondColumnLayout {
                LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.displayText
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Current index")
                tooltip: qsTr("The index of the current item.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    minimumValue: -9999999
                    maximumValue: 9999999
                    decimals: 0
                    backendValue: backendValues.currentIndex
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Flat")
                tooltip: qsTr("Whether the combo box button is flat.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.flat.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.flat
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Editable")
                tooltip: qsTr("Whether the combo box is editable.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.editable.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.editable
                }

                ExpandingSpacer {}
            }
        }
    }

    ControlSection {}

    FontSection {}

    PaddingSection {}

    InsetSection {}
}
