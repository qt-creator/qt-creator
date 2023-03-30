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

    FlickableSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Section {
        caption: qsTr("Grid View")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Cell size")
                tooltip: qsTr("Sets the dimensions of cells in the grid.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.contentWidth
                    minimumValue: 0
                    maximumValue: 10000
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The width of the object
                    text: qsTr("W", "width")
                    tooltip: qsTr("Width")
                }

                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.contentHeight
                    minimumValue: 0
                    maximumValue: 10000
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The height of the object
                    text: qsTr("H", "height")
                    tooltip: qsTr("Height")
                }
/*
                TODO QDS-4836
                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                LinkIndicator2D {}
*/
                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Flow")
                tooltip: qsTr("Sets the directions of the cells.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.flow
                    model: ["FlowLeftToRight", "FlowTopToBottom"]
                    scope: "GridView"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Layout direction")
                tooltip: qsTr("Sets in which direction items in the grid view are placed.")
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
                text: qsTr("Snap mode")
                tooltip: qsTr("Sets how the view scrolling will settle following a drag or flick.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.snapMode
                    model: ["NoSnap", "SnapToRow", "SnapOneRow"]
                    scope: "GridView"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Cache")
                tooltip: qsTr("Sets in pixels how far the components are kept loaded outside the view's visible area.")
                blockedByTemplate: !backendValues.cacheBuffer.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.cacheBuffer
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Navigation wraps")
                tooltip: qsTr("Whether the grid wraps key navigation.")
                blockedByTemplate: !backendValues.keyNavigationWraps.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.keyNavigationWraps.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.keyNavigationWraps
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Grid View Highlight")

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
                    scope: "GridView"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Move duration")
                tooltip: qsTr("Sets the animation duration of the highlight delegate.")
            }

            SectionLayout {
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
                            + "the <b>Preferred end</b>. Note that the user has to add a\n"
                            + "highlight component.")
            }

            SectionLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.preferredHighlightBegin
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Preferred end")
                tooltip: qsTr("Sets the preferred highlight end. It must be larger than\n"
                            + "the <b>Preferred begin</b>. Note that the user has to add\n"
                            + "a highlight component.")
            }

            SectionLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.preferredHighlightEnd
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Follows current")
                tooltip: qsTr("Toggles if the view manages the highlight.")
            }

            SectionLayout {
                CheckBox {
                    text: backendValues.highlightFollowsCurrentItem.valueToString
                    backendValue: backendValues.highlightFollowsCurrentItem
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
