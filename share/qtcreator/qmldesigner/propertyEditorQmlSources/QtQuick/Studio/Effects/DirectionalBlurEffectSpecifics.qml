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
        caption: qsTr("Directional Blur")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Angle")
                tooltip: qsTr("The direction for the blur. Blur is applied to both sides of each "
                              + "pixel. Therefore, setting the direction to 0 and 180 produces "
                              + "the same result.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.angle
                    decimals: 1
                    minimumValue: -180
                    maximumValue: 180
                    stepSize: 1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Samples")
                tooltip: qsTr("Samples per pixel for blur calculation. A larger value produces "
                              + "better quality, but is slower to render.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.samples
                    decimals: 0
                    minimumValue: 0
                    maximumValue: 1000
                    stepSize: 1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Length")
                tooltip: qsTr("The perceived amount of movement for each pixel. The movement is "
                              + "divided evenly to both sides of each pixel.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.length
                    decimals: 1
                    minimumValue: 0
                    maximumValue: 1000
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
