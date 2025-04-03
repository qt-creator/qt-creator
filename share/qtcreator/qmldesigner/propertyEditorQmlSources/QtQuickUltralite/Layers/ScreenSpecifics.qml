// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

//! [Screen compatibility]
Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Screen")

    SectionLayout {
        PropertyLabel { text: qsTr("Output Device") }

        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.outputDevice
                showTranslateCheckBox: false
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Background color") }

        ColorEditor {
            backendValue: backendValues.backgroundColor
            supportGradient: false
        }

        PropertyLabel { text: qsTr("Application size") }

        SecondColumnLayout {
            SpinBox {
                id: widthSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.defaultApplicationWidth
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                enabled: true
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
                tooltip: widthSpinBox.enabled ? qsTr("Width") : root.disbaledTooltip
                enabled: widthSpinBox.enabled
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                id: heightSpinBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.defaultApplicationHeight
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
                enabled: true
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
                tooltip: heightSpinBox.enabled ? qsTr("Height") : root.disbaledTooltip
                enabled: heightSpinBox.enabled
            }

            ExpandingSpacer {}
        }
    }
}
//! [Screen compatibility]
