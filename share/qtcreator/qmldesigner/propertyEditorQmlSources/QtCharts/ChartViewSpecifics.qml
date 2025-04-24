// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Column {
    width: parent.width

    Section {
        caption: qsTr("Title")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Title")
            }

            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.title
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Color")
            }

            ColorEditor {
                backendValue: backendValues.titleColor
                supportGradient: false
            }
        }
    }

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

            PropertyLabel {
                text: qsTr("Roundness")
                tooltip: qsTr("Diameter of the rounding circle at the corners")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.backgroundRoundness
                    minimumValue: 0.1
                    maximumValue: 100.0
                    stepSize: 0.1
                    decimals: 1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
            }

            PropertyLabel {
                text: qsTr("Drop Shadow")
                tooltip: qsTr("Enable border drop shadow")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.dropShadowEnabled.valueToString
                    backendValue: backendValues.dropShadowEnabled
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
            }
        }
    }

    Section {
        width: parent.width
        caption: qsTr("Plot Area")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Color")
            }

            ColorEditor {
                backendValue: backendValues.plotAreaColor
                supportGradient: false
            }
        }
    }

    Section {
        width: parent.width
        caption: qsTr("Localization")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Localize Numbers")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.localizeNumbers.valueToString
                    backendValue: backendValues.localizeNumbers
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }
            }
        }
    }
}
