// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import QtQuick.Layouts
import StudioTheme as StudioTheme

Section {
    width: parent.width
    caption: qsTr("Graph")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Render Mode")
            tooltip: qsTr("Rendering mode")
        }
        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.renderingMode
                model: ["Indirect", "DirectToBackground"]
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                scope: "Graphs3D"
            }
        }
        PropertyLabel {
            text: qsTr("Shadow Quality")
            tooltip: qsTr("Quality and style of the shadows")
        }
        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.shadowQuality
                model: ["None", "Low", "Medium",
                    "High", "SoftLow", "SoftMedium",
                    "SoftHigh"]
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                scope: "Graphs3D"
            }
        }
        PropertyLabel {
            text: qsTr("Optimization")
            tooltip: qsTr("Optimization hint")
        }
        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.optimizationHint
                model: ["Default", "Legacy"]
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                scope: "Graphs3D"
            }
        }
        PropertyLabel {
            text: qsTr("MSAA")
            tooltip: qsTr("Multisample anti-aliasing sample count")
        }
        SpinBox {
            backendValue: backendValues.msaaSamples
            minimumValue: 0
            maximumValue: 8
            implicitWidth: StudioTheme.Values.singleControlColumnWidth
                           + StudioTheme.Values.actionIndicatorWidth
        }
        PropertyLabel {
            text: qsTr("Aspect Ratio")
            tooltip: qsTr("Horizontal to vertical aspect ratio")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.aspectRatio
                minimumValue: 0.1
                maximumValue: 10.0
                stepSize: 0.1
                decimals: 1
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Horizontal AR")
            tooltip: qsTr("Horizontal aspect ratio")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.horizontalAspectRatio
                minimumValue: 0.1
                maximumValue: 10.0
                stepSize: 0.1
                decimals: 1
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Margin")
            tooltip: qsTr("Graph background margin")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.margin
                minimumValue: -1.0
                maximumValue: 100.0
                stepSize: 0.1
                decimals: 1
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
        PropertyLabel {
            text: qsTr("Measure FPS")
            tooltip: qsTr("Measure rendering speed as Frames Per Second")
        }
        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.measureFps
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }
        }
    }
}

