// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    caption: qsTr("Character Controller")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: "Gravity"
            tooltip: "The gravitational acceleration that applies to the character."
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                backendValue: backendValues.gravity_x
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "X"
                color: StudioTheme.Values.theme3DAxisXColor
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                backendValue: backendValues.gravity_y
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Y"
                color: StudioTheme.Values.theme3DAxisYColor
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                backendValue: backendValues.gravity_z
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Z"
                color: StudioTheme.Values.theme3DAxisZColor
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: "Movement"
            tooltip: "The controlled motion of the character."
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                backendValue: backendValues.movement_x
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "X"
                color: StudioTheme.Values.theme3DAxisXColor
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                backendValue: backendValues.movement_y
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Y"
                color: StudioTheme.Values.theme3DAxisYColor
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 2
                backendValue: backendValues.movement_z
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Z"
                color: StudioTheme.Values.theme3DAxisZColor
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: "Mid Air Control"
            tooltip: "Enables movement property to have an effect when the character is in free fall."
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.midAirControl.valueToString
                backendValue: backendValues.midAirControl
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: "Enable ShapeHit Callback"
            tooltip: "Enables the shapeHit callback for this character controller."
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.midAirControl.valueToString
                backendValue: backendValues.enableShapeHitCallback
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
