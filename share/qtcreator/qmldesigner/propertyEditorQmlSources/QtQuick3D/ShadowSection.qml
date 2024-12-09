// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Shadows")
    width: parent.width

    SectionLayout {

        PropertyLabel {
            text: qsTr("Casts Shadow")
            tooltip: qsTr("Enables shadow casting for this light.")
        }

        SecondColumnLayout {
            CheckBox {
                id: shadowCheckBox
                text: backendValues.castsShadow.valueToString
                backendValue: backendValues.castsShadow
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        // ### all the following should only be shown when shadows are enabled
        PropertyLabel {
            visible: shadowCheckBox.checked
            text: qsTr("Amount")
            tooltip: qsTr("Sets how dark the cast shadows should be.")
        }

        SecondColumnLayout {
            visible: shadowCheckBox.checked
            SpinBox {
                minimumValue: 0.0
                maximumValue: 100.0
                decimals: 0
                sliderIndicatorVisible: true
                backendValue: backendValues.shadowFactor
                enabled: shadowCheckBox.backendValue.value === true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: shadowCheckBox.checked
            text: qsTr("Filter")
            tooltip: qsTr("Sets how much blur is applied to the shadows.")
        }

        SecondColumnLayout {
            visible: shadowCheckBox.checked
            SpinBox {
                minimumValue: 1.0
                maximumValue: 100.0
                decimals: 0
                sliderIndicatorVisible: true
                backendValue: backendValues.shadowFilter
                enabled: shadowCheckBox.backendValue.value === true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: shadowCheckBox.checked
            text: qsTr("Quality")
            tooltip: qsTr("Sets the quality of the shadow map created for shadow rendering.")
        }

        SecondColumnLayout {
            visible: shadowCheckBox.checked
            ComboBox {
                scope: "Light"
                model: ["ShadowMapQualityLow", "ShadowMapQualityMedium", "ShadowMapQualityHigh", "ShadowMapQualityVeryHigh"]
                backendValue: backendValues.shadowMapQuality
                enabled: shadowCheckBox.backendValue.value === true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: shadowCheckBox.checked
            text: qsTr("Bias")
            tooltip: qsTr("Sets a slight offset to avoid self-shadowing artifacts.")
        }

        SecondColumnLayout {
            visible: shadowCheckBox.checked
            SpinBox {
                minimumValue: -1.0
                maximumValue: 1.0
                decimals: 3
                stepSize: 0.001
                sliderIndicatorVisible: true
                backendValue: backendValues.shadowBias
                enabled: shadowCheckBox.backendValue.value === true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: shadowCheckBox.checked
            text: qsTr("Far Distance")
            tooltip: qsTr("Sets the maximum distance for the shadow map.")
        }

        SecondColumnLayout {
            visible: shadowCheckBox.checked
            SpinBox {
                minimumValue: 0
                maximumValue: Infinity
                decimals: 0
                stepSize: 10
                backendValue: backendValues.shadowMapFar
                enabled: shadowCheckBox.backendValue.value === true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
