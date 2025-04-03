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
        caption: qsTr("Radial Blur")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Angle")
                tooltip: qsTr("The direction for the blur and at the same time the level of "
                              + "blurring. The larger the angle, the more the result becomes "
                              + "blurred. The quality of the blur depends on samples property.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.angle
                    decimals: 1
                    minimumValue: 0
                    maximumValue: 360
                    stepSize: 1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Samples")
                tooltip: qsTr("Samples per pixel for blur calculation. A larger value produces "
                              + "better quality, but is slower to render. This property is not "
                              + "intended to be animated. Changing this property may cause the "
                              + "underlying OpenGL shaders to be recompiled.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.samples
                    decimals: 0
                    minimumValue: 0
                    maximumValue: 200
                    stepSize: 1
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
        caption: qsTr("Caching and Border")

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

            PropertyLabel {
                text: qsTr("Transparent border")
                tooltip: qsTr("Pads the exterior of the component with a transparent edge, making "
                              + "sampling outside the source texture use transparency instead of "
                              + "the edge pixels.")
            }

            SecondColumnLayout {
                CheckBox {
                    backendValue: backendValues.transparentBorder
                    text: backendValues.transparentBorder.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
