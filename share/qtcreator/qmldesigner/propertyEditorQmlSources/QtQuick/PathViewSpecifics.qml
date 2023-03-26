// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        caption: qsTr("Path View")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Interactive")
                tooltip: qsTr("Toggles if the path view allows drag or flick.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.interactive.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.interactive
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Drag margin")
                tooltip: qsTr("Sets a margin within which the drag function also works even without clicking the item itself.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.dragMargin
                    minimumValue: 0
                    maximumValue: 100
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Flick deceleration")
                tooltip: qsTr("Sets the rate by which a flick action slows down after performing.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.flickDeceleration
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Offset")
                tooltip: qsTr("Sets how far along the path the items are from their initial position.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.offset
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Item count")
                tooltip: qsTr("Sets the number of items visible at once along the path.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.pathItemCount
                    minimumValue: -1
                    maximumValue: 1000
                    decimals: 0
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Path View Highlight")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Range")
                tooltip: qsTr("Sets the highlight range mode.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.highlightRangeMode
                    model: ["NoHighlightRange", "ApplyRange", "StrictlyEnforceRange"]
                    scope: "PathView"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Move duration")
                tooltip: qsTr("Sets the animation duration of the highlight delegate when\n"
                            + "it is moved.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.highlightMoveDuration
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Preferred begin")
                tooltip: qsTr("Sets the preferred highlight beginning. It must be smaller than\n"
                            + "the <b>Preferred end</b>. Note that the user has to add\n"
                            + "a highlight component.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.preferredHighlightBegin
                    minimumValue: 0
                    maximumValue: 1
                    stepSize: 0.1
                    decimals: 2
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Preferred end")
                tooltip: qsTr("Sets the preferred highlight end. It must be larger than\n"
                            + "the <b>Preferred begin</b>. Note that the user has to add\n"
                            + "a highlight component.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.preferredHighlightEnd
                    minimumValue: 0
                    maximumValue: 1
                    stepSize: 0.1
                    decimals: 2
                }

                ExpandingSpacer {}
            }
        }
    }
}
