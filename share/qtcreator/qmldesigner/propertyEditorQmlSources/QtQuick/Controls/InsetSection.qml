// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Inset")

    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Vertical")
            tooltip: qsTr("Sets the space from the top and bottom of the area to the background top and bottom.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.topInset
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                tooltip: qsTr("Top inset for the background.")
                enabled: !backendValues.topPadding.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.bottomInset
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 180
                tooltip: qsTr("Bottom inset for the background.")
                enabled: !backendValues.bottomPadding.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Horizontal")
            tooltip: qsTr("Sets the space from the left and right of the area to the background left and right.")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.leftInset
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 270
                tooltip: qsTr("Left inset for the background.")
                enabled: !backendValues.leftPadding.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                maximumValue: 10000
                minimumValue: -10000
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.rightInset
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                rotation: 90
                tooltip: qsTr("Right inset for the background.")
                enabled: !backendValues.rightPadding.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}
