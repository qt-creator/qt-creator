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
        caption: qsTr("Arc Arrow")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel { text: qsTr("Color") }

            ColorEditor {
                backendValue: backendValues.color
                supportGradient: true
                shapeGradients: true
            }

            PropertyLabel { text: qsTr("Thickness") }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.thickness
                    decimals: 1
                    minimumValue: 1
                    maximumValue: 500
                    stepSize: 1
                }

                ExpandingSpacer {}
            }

            PropertyLabel { text: qsTr("Arrow Size") }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.arrowSize
                    decimals: 1
                    minimumValue: 1
                    maximumValue: 500
                    stepSize: 1
                }

                ExpandingSpacer {}
            }

            PropertyLabel { text: qsTr("Radius") }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.radius
                    decimals: 1
                    minimumValue: 1
                    maximumValue: 500
                    stepSize: 1
                }

                ExpandingSpacer {}
            }

            PropertyLabel { text: qsTr("Arc start") }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.begin
                    decimals: 1
                    minimumValue: -720
                    maximumValue: 720
                    stepSize: 1
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel { text: "°" }

                ExpandingSpacer {}
            }

            PropertyLabel { text: qsTr("Arc end") }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.end
                    decimals: 1
                    minimumValue: -720
                    maximumValue: 720
                    stepSize: 1
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel { text: "°" }

                ExpandingSpacer {}
            }

            PropertyLabel { text: qsTr("Flip") }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.round.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.flip
                }

                ExpandingSpacer {}
            }
        }
    }
}
