// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Padding")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Vertical")
            tooltip: qsTr("Sets the padding on top and bottom of the item.")
            blockedByTemplate: !backendValues.topPadding.isAvailable
                               && !backendValues.bottomPadding.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.topPadding
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                tooltip: qsTr("Padding between the content and the top edge of the item.")
                enabled: backendValues.topPadding.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.bottomPadding
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 180
                tooltip: qsTr("Padding between the content and the bottom edge of the item.")
                enabled: backendValues.bottomPadding.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Horizontal")
            tooltip: qsTr("Sets the padding on the left and right sides of the item.")
            blockedByTemplate: !backendValues.leftPadding.isAvailable
                               && !backendValues.rightPadding.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.leftPadding
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 270
                tooltip: qsTr("Padding between the content and the left edge of the item.")
                enabled: backendValues.leftPadding.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.rightPadding
                enabled: backendValue.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 90
                tooltip: qsTr("Padding between the content and the right edge of the item.")
                enabled: backendValues.rightPadding.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Global")
            tooltip: qsTr("Sets the padding for all sides of the item.")
            blockedByTemplate: !backendValues.padding.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.padding
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
