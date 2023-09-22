// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Advanced")

    SectionLayout {
        PropertyLabel {
            visible: majorQtQuickVersion > 1
            text: qsTr("Enabled")
            tooltip: qsTr("Toggles if the component is enabled to receive mouse and keyboard input.")
        }

        SecondColumnLayout {
            visible: majorQtQuickVersion > 1

            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.enabled
                text: backendValues.enabled.valueToString
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Smooth")
            tooltip: qsTr("Toggles if the smoothing is performed using linear interpolation method. Keeping it unchecked would follow non-smooth method using nearest neighbor. It is mostly applicable on image based items. ")
            blockedByTemplate: !backendValues.smooth.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.smooth
                text: backendValues.smooth.valueToString
                enabled: backendValues.smooth.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Antialiasing")
            tooltip: qsTr("Refines the edges of the image.")
            blockedByTemplate: !backendValues.antialiasing.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.antialiasing
                text: backendValues.antialiasing.valueToString
                enabled: backendValues.antialiasing.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Focus")
            tooltip: qsTr("Sets focus on the component within the enclosing focus scope.")
            blockedByTemplate: !backendValues.focus.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.focus
                text: backendValues.focus.valueToString
                enabled: backendValues.focus.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Focus on tab")
            tooltip: qsTr("Adds the component to the tab focus chain.")
            blockedByTemplate: !backendValues.activeFocusOnTab.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.activeFocusOnTab
                text: backendValues.activeFocusOnTab.valueToString
                enabled: backendValues.activeFocusOnTab.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Baseline offset")
            tooltip: qsTr("Sets the position of the component's baseline in local coordinates.")
            blockedByTemplate: !backendValues.baselineOffset.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                sliderIndicatorVisible: true
                backendValue: backendValues.baselineOffset
                hasSlider: true
                decimals: 0
                minimumValue: -1000
                maximumValue: 1000
                enabled: backendValues.baselineOffset.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
