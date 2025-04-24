// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    caption: qsTr("Physics Material")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: "Static Friction"
            tooltip: "The friction coefficient of the material when it is not moving."
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 9999999
                decimals: 2
                stepSize: 0.01
                backendValue: backendValues.staticFriction
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: "Dynamic Friction"
            tooltip: "The friction coefficient of the material when it is moving."
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 9999999
                decimals: 2
                stepSize: 0.01
                backendValue: backendValues.dynamicFriction
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: "Restitution"
            tooltip: "The coefficient of restitution of the material."
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.01
                sliderIndicatorVisible: true
                backendValue: backendValues.restitution
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }
            ExpandingSpacer {}
        }
    }
}
