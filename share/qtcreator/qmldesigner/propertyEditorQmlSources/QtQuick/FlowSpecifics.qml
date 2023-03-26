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
        caption: qsTr("Flow")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Spacing")
                tooltip: qsTr("Sets the spacing between flow items.")
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
                tooltip: qsTr("Sets the direction of flow items.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.flow
                    model: ["LeftToRight", "TopToBottom"]
                    scope: "Flow"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Layout direction")
                tooltip: qsTr("Sets in which direction items in the flow are placed.")
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
        }
    }

    PaddingSection {}
}
