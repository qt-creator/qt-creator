// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Popup")

    SectionLayout {
        PropertyLabel { text: qsTr("Size") }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.width
                minimumValue: 0
                maximumValue: 10000
                decimals: 0
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                backendValue: backendValues.height
                minimumValue: 0
                maximumValue: 10000
                decimals: 0
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Visibility") }

        SecondColumnLayout {
            CheckBox {
                text: qsTr("Visible")
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.visible
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            CheckBox {
                text: qsTr("Clip")
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.clip
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Behavior") }

        SecondColumnLayout {
            CheckBox {
                text: qsTr("Modal")
                tooltip: qsTr("Defines the modality of the popup.")
                backendValue: backendValues.modal
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            CheckBox {
                text: qsTr("Dim")
                tooltip: qsTr("Defines whether the popup dims the background.")
                backendValue: backendValues.dim
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Opacity") }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.opacity
                hasSlider: true
                minimumValue: 0
                maximumValue: 1
                stepSize: 0.1
                decimals: 2
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Scale") }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.scale
                hasSlider: true
                minimumValue: 0
                maximumValue: 1
                stepSize: 0.1
                decimals: 2
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Spacing")
            tooltip: qsTr("Spacing between internal elements of the control.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: -4000
                maximumValue: 4000
                decimals: 0
                backendValue: backendValues.spacing
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
