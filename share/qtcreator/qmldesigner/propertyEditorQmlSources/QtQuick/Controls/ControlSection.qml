// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Control")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Enable")
            tooltip: qsTr("Toggles if the component can receive hover events.")
        }

        SecondColumnLayout {
            CheckBox {
                text: qsTr("Control")
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.enabled
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            CheckBox {
                text: qsTr("Hover")
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.hoverEnabled
                enabled: backendValues.hoverEnabled.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Focus policy")
            tooltip: qsTr("Sets focus method.")
            blockedByTemplate: !backendValues.focusPolicy.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.focusPolicy
                model: [ "TabFocus", "ClickFocus", "StrongFocus", "WheelFocus", "NoFocus" ]
                scope: "Qt"
                enabled: backendValues.focusPolicy.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Spacing")
            tooltip: qsTr("Sets the spacing between internal elements of the component.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.spacing
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Wheel")
            tooltip: qsTr("Toggles if the component supports mouse wheel events.")
            blockedByTemplate: !backendValues.wheelEnabled.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.wheelEnabled.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.wheelEnabled
                enabled: backendValues.wheelEnabled.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
