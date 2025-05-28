// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Corner Radiuses")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Top")
            tooltip: qsTr("Toggles the top left or right corner to a rounded shape.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.topLeftRadius
                decimals: 0
                minimumValue: 0
                maximumValue: 0xffff
                stepSize: 1
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.cornerA
                icon1: StudioTheme.Constants.cornerB
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.topRightRadius
                decimals: 0
                minimumValue: 0
                maximumValue: 0xffff
                stepSize: 1
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.cornerA
                icon1: StudioTheme.Constants.cornerB
                rotation: 90
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Bottom")
            tooltip: qsTr("Toggles the bottom left or right corner to a rounded shape.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.bottomLeftRadius
                decimals: 0
                minimumValue: 0
                maximumValue: 0xffff
                stepSize: 1
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.cornerA
                icon1: StudioTheme.Constants.cornerB
                rotation: 270
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.bottomRightRadius
                decimals: 0
                minimumValue: 0
                maximumValue: 0xffff
                stepSize: 1
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.cornerA
                icon1: StudioTheme.Constants.cornerB
                rotation: 180
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Global")
            tooltip: qsTr("Toggles all the corners into a rounded shape.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.radius
                decimals: 0
                minimumValue: 0
                maximumValue: 0xffff
                stepSize: 1
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            IconLabel {
                icon: StudioTheme.Constants.cornersAll
            }

            ExpandingSpacer {}
        }
    }
}
