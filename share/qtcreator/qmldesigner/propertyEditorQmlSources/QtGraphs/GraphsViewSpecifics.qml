// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import QtQuick.Layouts
import StudioTheme as StudioTheme

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("Background")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Color")
            }

            ColorEditor {
                backendValue: backendValues.backgroundColor
                supportGradient: false
            }
        }
    }

    Section {
        width: parent.width
        caption: qsTr("Margins")

        SectionLayout {
            rows: 4
            PropertyLabel {
                text: qsTr("Top")
                tooltip: qsTr("The amount of empty space on the top of the graph.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.marginTop
                    minimumValue: 0.0
                    maximumValue: 9999.0
                    stepSize: 1.0
                    decimals: 1
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
            }

            PropertyLabel {
                text: qsTr("Bottom")
                tooltip: qsTr("The amount of empty space on the bottom of the graph.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.marginBottom
                    minimumValue: 0.0
                    maximumValue: 9999.0
                    stepSize: 1.0
                    decimals: 1
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
            }

            PropertyLabel {
                text: qsTr("Left")
                tooltip: qsTr("The amount of empty space on the left of the graph.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.marginLeft
                    minimumValue: 0.0
                    maximumValue: 9999.0
                    stepSize: 1.0
                    decimals: 1
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
            }

            PropertyLabel {
                text: qsTr("Right")
                tooltip: qsTr("The amount of empty space on the right of the graph.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.marginRight
                    minimumValue: 0.0
                    maximumValue: 9999.0
                    stepSize: 1.0
                    decimals: 1
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
            }
        }
    }
}
