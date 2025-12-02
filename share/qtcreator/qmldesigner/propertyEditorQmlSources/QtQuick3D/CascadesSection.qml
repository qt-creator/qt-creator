// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    caption: qsTr("Cascades")
    width: parent.width

    SectionLayout {

        PropertyLabel {
            text: qsTr("No. Splits")
            tooltip: qsTr("Sets the number of cascade splits for this light.")
        }

        SecondColumnLayout {
            ComboBox {
                id: numSplitsComboBox
                valueType: ComboBox.ValueType.Integer
                model: [0, 1, 2, 3]
                backendValue: backendValues.csmNumSplits
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: numSplitsComboBox.currentIndex > 0
            text: qsTr("Blend ratio")
            tooltip: qsTr("Sets how much of the shadow of any cascade should be blended together with the previous one.")
        }

        SecondColumnLayout {
            visible: numSplitsComboBox.currentIndex > 0
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.01
                backendValue: backendValues.csmBlendRatio
                sliderIndicatorVisible: true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: numSplitsComboBox.currentIndex > 0
            text: qsTr("Split 1")
            tooltip: qsTr("Sets where the first cascade of the shadow map split occurs.")
        }

        SecondColumnLayout {
            visible: numSplitsComboBox.currentIndex > 0
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.01
                backendValue: backendValues.csmSplit1
                sliderIndicatorVisible: true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: numSplitsComboBox.currentIndex > 1
            text: qsTr("Split 2")
            tooltip: qsTr("Sets where the second cascade of the shadow map split occurs.")
        }

        SecondColumnLayout {
            visible: numSplitsComboBox.currentIndex > 1
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.01
                backendValue: backendValues.csmSplit2
                sliderIndicatorVisible: true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: numSplitsComboBox.currentIndex > 2
            text: qsTr("Split 3")
            tooltip: qsTr("Sets where the third cascade of the shadow map split occurs.")
        }

        SecondColumnLayout {
            visible: numSplitsComboBox.currentIndex > 2
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.01
                backendValue: backendValues.csmSplit3
                sliderIndicatorVisible: true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
