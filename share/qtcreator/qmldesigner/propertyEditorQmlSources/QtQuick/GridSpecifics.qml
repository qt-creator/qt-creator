// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("Grid")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Columns")
                tooltip: qsTr("Sets the number of columns in the grid.")
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

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Rows")
                tooltip: qsTr("Sets the number of rows in the grid.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.rows
                    minimumValue: 0
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Spacing")
                tooltip: qsTr("Sets the space between grid items. The same space is applied for both rows and columns.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.spacing
                    minimumValue: -4000
                    maximumValue: 4000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Flow")
                tooltip: qsTr("Sets in which direction items in the grid are placed.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.flow
                    model: ["LeftToRight", "TopToBottom"]
                    scope: "Grid"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Layout direction")
                tooltip: qsTr("Sets in which direction items in the grid are placed.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.layoutDirection
                    model: ["LeftToRight", "RightToLeft"]
                    scope: "Qt"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Alignment H")
                tooltip: qsTr("Sets the horizontal alignment of items in the grid.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.horizontalItemAlignment
                    model: ["AlignLeft", "AlignRight" ,"AlignHCenter"]
                    scope: "Grid"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Alignment V")
                tooltip: qsTr("Sets the vertical alignment of items in the grid.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.verticalItemAlignment
                    model: ["AlignTop", "AlignBottom" ,"AlignVCenter"]
                    scope: "Grid"
                }

                ExpandingSpacer {}
            }
        }
    }

    PaddingSection {
    }
}
