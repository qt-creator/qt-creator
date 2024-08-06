// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Grid Layout")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Columns & Rows")
            tooltip: qsTr("Sets the number of columns and rows in the <b>Grid Layout</b>.")
        }

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

        PropertyLabel {
            text: qsTr("Spacing")
            tooltip: qsTr("Sets the space between the items in pixels in the rows and columns in the <b>Grid Layout</b>.")
        }

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

        PropertyLabel {
            text: qsTr("Flow")
            tooltip: qsTr("Set the direction of dynamic items to flow in rows or columns in the <b>Grid Layout</b>.")
        }

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

        PropertyLabel {
            text: qsTr("Layout direction")
            tooltip: qsTr("Sets the direction of the dynamic items left to right or right to left in the <b>Grid Layout</b>.")

        }

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

        PropertyLabel {
            text: qsTr("Uniform cell sizes")
            tooltip: qsTr("Toggles all cells to have a uniform height or width.")
            visible: majorQtQuickVersion === 6 && minorQtQuickVersion >= 6
            blockedByTemplate: !(backendValues.uniformCellHeights.isAvailable
                                 && backendValues.uniformCellWidths.isAvailable)
        }

        SecondColumnLayout {
            CheckBox {
                text: qsTr("Heights")
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.uniformCellHeights
                visible: majorQtQuickVersion === 6 && minorQtQuickVersion >= 6
                enabled: backendValues.uniformCellHeights.isAvailable
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            CheckBox {
                text: qsTr("Widths")
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.uniformCellWidths
                visible: majorQtQuickVersion === 6 && minorQtQuickVersion >= 6
                enabled: backendValues.uniformCellWidths.isAvailable
            }

            ExpandingSpacer {}
        }
    }
}

