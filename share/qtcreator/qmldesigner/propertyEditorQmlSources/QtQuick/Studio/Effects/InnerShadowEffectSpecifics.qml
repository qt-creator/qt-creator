// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Inner Shadow Color")

        SectionLayout {
            PropertyLabel { text: qsTr("Inner shadow color") }

            ColorEditor {
                backendValue: backendValues.color
                supportGradient: false
            }
        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Inner Shadow")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Radius")
                tooltip: qsTr("The softness of the shadow. A larger radius causes the edges of the "
                              + "shadow to appear more blurry.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.radius
                    decimals: 1
                    minimumValue: 0
                    maximumValue: 100
                    stepSize: 1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Samples")
                tooltip: qsTr("Samples per pixel for edge softening blur calculation. Ideally, "
                              + "this value should be twice as large as the highest required "
                              + "radius value plus one.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.samples
                    decimals: 0
                    minimumValue: 0
                    maximumValue: 201
                    stepSize: 1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Spread")
                tooltip: qsTr("The part of the shadow color that is strengthened near the source "
                              + "edges.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.spread
                    decimals: 2
                    minimumValue: 0
                    maximumValue: 1
                    stepSize: 0.1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Fast")
                tooltip: qsTr("The blurring algorithm that is used to produce the softness for the "
                              + "effect.")
            }

            SecondColumnLayout {
                CheckBox {
                    backendValue: backendValues.fast
                    text: backendValues.fast.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Offsets")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Offset")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.horizontalOffset
                    decimals: 1
                    minimumValue: -1000
                    maximumValue: 1000
                    stepSize: 1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The horizontal offset
                    text: qsTr("H", "horizontal")
                    tooltip: qsTr("The horizontal offset for the rendered shadow compared to the " +
                                  "inner shadow component's horizontal position.")
                }

                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                SpinBox {
                    backendValue: backendValues.verticalOffset
                    decimals: 1
                    minimumValue: -1000
                    maximumValue: 1000
                    stepSize: 1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The vertical offset
                    text: qsTr("V", "vertical")
                    tooltip: qsTr("The vertical offset for the rendered shadow compared to the " +
                                  "inner shadow component's vertical position. ")
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Caching")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Cached")
                tooltip: qsTr("Caches the effect output pixels to improve the rendering "
                              + "performance.")
            }

            SecondColumnLayout {
                CheckBox {
                    backendValue: backendValues.cached
                    text: backendValues.cached.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
