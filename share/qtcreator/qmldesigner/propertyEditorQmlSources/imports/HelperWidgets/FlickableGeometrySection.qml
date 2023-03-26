// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    caption: qsTr("Flickable Geometry")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Content size")
            tooltip: qsTr("Sets the size of the content (the surface controlled by the flickable).")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.contentWidth
                minimumValue: 0
                maximumValue: 10000
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The width of the object
                text: qsTr("W", "width")
                tooltip: qsTr("Content width used for calculating the total implicit width.")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.contentHeight
                minimumValue: 0
                maximumValue: 10000
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                //: The height of the object
                text: qsTr("H", "height")
                tooltip: qsTr("Content height used for calculating the total implicit height.")
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Content")
            tooltip: qsTr("Sets the current position of the component.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.contentX
                minimumValue: -8000
                maximumValue: 8000
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "X"
                tooltip: qsTr("Horizontal position.")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.contentY
                minimumValue: -8000
                maximumValue: 8000
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Y"
                tooltip: qsTr("Vertical position.")
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Origin")
            tooltip: qsTr("Sets the origin point of the content.")
            blockedByTemplate: !backendValues.originX.isAvailable
                               && !backendValues.originY.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.originX
                minimumValue: -8000
                maximumValue: 8000
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "X"
                tooltip: qsTr("Horizontal position.")
                enabled: backendValues.originX.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.originY
                minimumValue: -8000
                maximumValue: 8000
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel {
                text: "Y"
                tooltip: qsTr("Vertical position.")
                enabled: backendValues.originY.isAvailable
            }
/*
            TODO QDS-4836
            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            LinkIndicator2D {}
*/
            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Left margin")
            tooltip: qsTr("Sets an additional left margin in the flickable area.")
            blockedByTemplate: !backendValues.leftMargin.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.leftMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Right margin")
            tooltip: qsTr("Sets an additional right margin in the flickable area.")
            blockedByTemplate: !backendValues.rightMargin.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.rightMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Top margin")
            tooltip: qsTr("Sets an additional top margin in the flickable area.")
            blockedByTemplate: !backendValues.topMargin.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.topMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Bottom margin")
            tooltip: qsTr("Sets an additional bottom margin in the flickable area.")
            blockedByTemplate: !backendValues.bottomMargin.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.bottomMargin
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
