// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Margin")

    anchors.left: parent.left
    anchors.right: parent.right

    property alias backendValueTopMargin: spinBoxTopMargin.backendValue
    property alias backendValueBottomMargin: spinBoxBottomMargin.backendValue
    property alias backendValueLeftMargin: spinBoxLeftMargin.backendValue
    property alias backendValueRightMargin: spinBoxRightMargin.backendValue
    property alias backendValueMargins: spinBoxMargins.backendValue

    SectionLayout {
        PropertyLabel { text: qsTr("Vertical") }

        SecondColumnLayout {
            SpinBox {
                id: spinBoxTopMargin
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.topMargin
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                iconColor: StudioTheme.Values.themeTextColor
                tooltip: qsTr("The margin above the item.")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                id: spinBoxBottomMargin
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.bottomMargin
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                iconColor: StudioTheme.Values.themeTextColor
                rotation: 180
                tooltip: qsTr("The margin below the item.")
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Horizontal") }

        SecondColumnLayout {
            SpinBox {
                id: spinBoxLeftMargin
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.leftMargin
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                iconColor: StudioTheme.Values.themeTextColor
                rotation: 270
                tooltip: qsTr("The margin left of the item.")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                id: spinBoxRightMargin
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.rightMargin
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            MultiIconLabel {
                icon0: StudioTheme.Constants.paddingFrame
                icon1: StudioTheme.Constants.paddingEdge
                iconColor: StudioTheme.Values.themeTextColor
                rotation: 90
                tooltip: qsTr("The margin right of the item.")
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Margins")
            tooltip: qsTr("The margins around the item.")
        }

        SecondColumnLayout {
            SpinBox {
                id: spinBoxMargins
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                minimumValue: -10000
                maximumValue: 10000
                decimals: 0
                backendValue: backendValues.margins
            }

            ExpandingSpacer {}
        }
    }
}
