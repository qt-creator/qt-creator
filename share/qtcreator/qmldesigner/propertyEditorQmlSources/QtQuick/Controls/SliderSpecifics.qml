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
        caption: qsTr("Slider")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Value")
                tooltip: qsTr("The current value of the slider.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    minimumValue: Math.min(backendValues.from.value, backendValues.to.value)
                    maximumValue: Math.max(backendValues.from.value, backendValues.to.value)
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.value
                }

                Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

                CheckBox {
                    text: qsTr("Live")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.live
                    enabled: backendValues.live.isAvailable
                    tooltip: qsTr("Whether the slider provides live value updates.")
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("From")
                tooltip: qsTr("The starting value of the slider range.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
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
                tooltip: qsTr("The ending value of the slider range.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
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
                tooltip: qsTr("The step size of the slider.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.stepSize
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Drag threshold")
                tooltip: qsTr("The threshold (in logical pixels) at which a drag event will be initiated.")
                blockedByTemplate: !backendValues.touchDragThreshold.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    minimumValue: 0
                    maximumValue: 10000
                    decimals: 0
                    backendValue: backendValues.touchDragThreshold
                    enabled: backendValues.touchDragThreshold.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Snap mode")
                tooltip: qsTr("The snap mode of the slider.")
                blockedByTemplate: !backendValues.snapMode.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.snapMode
                    model: [ "NoSnap", "SnapOnRelease", "SnapAlways" ]
                    scope: "RangeSlider"
                    enabled: backendValues.snapMode.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Orientation")
                tooltip: qsTr("The orientation of the slider.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.orientation
                    model: [ "Horizontal", "Vertical" ]
                    scope: "Qt"
                }

                ExpandingSpacer {}
            }
        }
    }

    ControlSection {}

    PaddingSection {}

    InsetSection {}
}
