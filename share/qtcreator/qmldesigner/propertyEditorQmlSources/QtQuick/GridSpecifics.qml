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
            PropertyLabel { text: qsTr("Columns") }

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

            PropertyLabel { text: qsTr("Rows") }

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

            PropertyLabel { text: qsTr("Spacing") }

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

            PropertyLabel { text: qsTr("Flow") }

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

            PropertyLabel { text: qsTr("Layout direction") }

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

            PropertyLabel { text: qsTr("Alignment H") }

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

            PropertyLabel { text: qsTr("Alignment V") }

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
