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
        caption: qsTr("Brightness and Contrast")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Brightness")
                tooltip: qsTr("The increase or decrease of source brightness.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.brightness
                    decimals: 2
                    minimumValue: -1
                    maximumValue: 1
                    stepSize: 0.1
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Contrast")
                tooltip: qsTr("The increase or decrease of source contrast. The decrease of the "
                              + "contrast is linear, but the increase is applied with a "
                              + "non-linear curve to allow very high contrast adjustment at the "
                              + "high end of the value range.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.contrast
                    decimals: 2
                    minimumValue: -1
                    maximumValue: 1
                    stepSize: 0.1
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
