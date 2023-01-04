// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        caption: qsTr("Dial")

        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Value")
                tooltip: qsTr("The current value of the dial and whether it provides live value updates.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    minimumValue: Math.min(backendValues.from.value, backendValues.to.value)
                    maximumValue: Math.max(backendValues.from.value, backendValues.to.value)
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.value
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                CheckBox {
                    text: qsTr("Live")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.live
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("From")
                tooltip: qsTr("The starting value of the dial range.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.from
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("To")
                tooltip: qsTr("The ending value of the dial range.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.to
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Step size")
                tooltip: qsTr("The step size of the dial.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.stepSize
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Snap mode")
                tooltip: qsTr("The snap mode of the dial.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.snapMode
                    model: [ "NoSnap", "SnapOnRelease", "SnapAlways" ]
                    scope: "Dial"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Input mode")
                tooltip: qsTr("How the dial tracks movement.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.inputMode
                    model: [ "Circular", "Horizontal", "Vertical" ]
                    scope: "Dial"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Wrap")
                tooltip: qsTr("Whether the dial wraps when dragged.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.wrap.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.wrap
                }

                ExpandingSpacer {}
            }
        }
    }

    ControlSection {}

    PaddingSection {}

    InsetSection {}
}
