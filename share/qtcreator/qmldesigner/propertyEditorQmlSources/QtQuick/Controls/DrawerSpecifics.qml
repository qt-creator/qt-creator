// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    PopupSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Drawer")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Edge")
                tooltip: qsTr("Defines the edge of the window the drawer will open from.")
            }

            SecondColumnLayout {
                ComboBox {
                    backendValue: backendValues.edge
                    scope: "Qt"
                    model: ["TopEdge", "LeftEdge", "RightEdge", "BottomEdge"]
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Drag margin")
                tooltip: qsTr("Defines the distance from the screen edge within which drag actions will open the drawer.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.dragMargin
                    hasSlider: true
                    minimumValue: 0
                    maximumValue: 400
                    stepSize: 1
                    decimals: 0
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    MarginSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    PaddingSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    FontSection {}
}
