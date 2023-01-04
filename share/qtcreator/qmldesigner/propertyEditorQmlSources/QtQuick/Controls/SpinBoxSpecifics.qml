// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("Spin Box")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Value")
                tooltip: qsTr("The current value of the spin box.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    minimumValue: Math.min(backendValues.from.value, backendValues.to.value)
                    maximumValue: Math.max(backendValues.from.value, backendValues.to.value)
                    decimals: 2
                    backendValue: backendValues.value
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("From")
                tooltip: qsTr("The starting value of the spin box range.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 2
                    backendValue: backendValues.from
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("To")
                tooltip: qsTr("The ending value of the spin box range.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 2
                    backendValue: backendValues.to
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Step size")
                tooltip: qsTr("The step size of the spin box.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 2
                    backendValue: backendValues.stepSize
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Editable")
                tooltip: qsTr("Whether the spin box is editable.")
            }

            SecondColumnLayout {
                CheckBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    text: backendValues.editable.valueToString
                    backendValue: backendValues.editable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Wrap")
                tooltip: qsTr("Whether the spin box values wrap.")
            }

            SecondColumnLayout {
                CheckBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    text: backendValues.wrap.valueToString
                    backendValue: backendValues.wrap
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
