// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Grid Layout")

    SectionLayout {
        PropertyLabel { text: qsTr("Columns & Rows") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.columns
                minimumValue: 0
                maximumValue: 2000
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            IconLabel { icon: StudioTheme.Constants.columnsAndRows }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.rows
                minimumValue: 0
                maximumValue: 2000
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            IconLabel {
                icon: StudioTheme.Constants.columnsAndRows
                rotation: 90
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Spacing") }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.columnSpacing
                minimumValue: -4000
                maximumValue: 4000
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            IconLabel { icon: StudioTheme.Constants.columnsAndRows }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.rowSpacing
                minimumValue: -4000
                maximumValue: 4000
                decimals: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            IconLabel {
                icon: StudioTheme.Constants.columnsAndRows
                rotation: 90
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Flow") }

        SecondColumnLayout {
            ComboBox {
                model: ["LeftToRight", "TopToBottom"]
                backendValue: backendValues.flow
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                scope: "GridLayout"
            }

            ExpandingSpacer {}
        }

        PropertyLabel { text: qsTr("Layout direction") }

        SecondColumnLayout {
            ComboBox {
                model: ["LeftToRight", "RightToLeft"]
                backendValue: backendValues.layoutDirection
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                scope: "Qt"
            }

            ExpandingSpacer {}
        }
    }
}
