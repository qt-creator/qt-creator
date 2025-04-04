// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("Flipped Status")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Rotational axis")
                tooltip: qsTr("Sets the rotation along with the x-axis or y-axis.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.rotationalAxis
                    model: ["X Axis", "Y Axis"]
                    useInteger: true
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Flip angle")
                tooltip: qsTr("Sets the angle of the components to produce the flipping effect.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.flipAngle
                    minimumValue: -360
                    maximumValue: 360
                    stepSize: 10
                    sliderIndicatorVisible: true
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel { text: "°" }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Opacity")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Front")
                tooltip: qsTr("Sets the visibility percentage of the front side component within the Flipable component.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.opacityFront // TODO convert to %
                    decimals: 2
                    minimumValue: 0
                    maximumValue: 1
                    stepSize: 0.1
                    sliderIndicatorVisible: true
                }

                // Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                // ControlLabel { text: "%" }

                ExpandingSpacer {}
            }
            PropertyLabel {
                text: qsTr("Back")
                tooltip: qsTr("Sets the visibility percentage of the back side component within the Flipable component.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.opacityBack // TODO convert to %
                    decimals: 2
                    minimumValue: 0
                    maximumValue: 1
                    stepSize: 0.1
                    sliderIndicatorVisible: true
                }

                // Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                // ControlLabel { text: "%" }

                ExpandingSpacer {}
            }
        }
    }
}
