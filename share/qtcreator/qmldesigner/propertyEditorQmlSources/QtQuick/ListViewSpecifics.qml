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

    FlickableSection {}

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("List View")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Orientation")
                tooltip: qsTr("Orientation of the list.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.orientation
                    model: ["Horizontal", "Vertical"]
                    scope: "ListView"
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Layout direction")
                blockedByTemplate: !backendValues.layoutDirection.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.layoutDirection
                    model: ["LeftToRight", "RightToLeft"]
                    scope: "Qt"
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Snap mode")
                tooltip: qsTr("Determines how the view scrolling will settle following a drag or flick.")
                blockedByTemplate: !backendValues.snapMode.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.snapMode
                    model: ["NoSnap", "SnapToItem", "SnapOneItem"]
                    scope: "ListView"
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Spacing")
                tooltip: qsTr("Spacing between components.")
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
                text: qsTr("Cache")
                tooltip: qsTr("Cache buffer.")
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
        caption: qsTr("List View Highlight")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Range")
                tooltip: qsTr("Highlight range.")
                blockedByTemplate: !backendValues.highlightRangeMode.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.highlightRangeMode
                    model: ["NoHighlightRange", "ApplyRange", "StrictlyEnforceRange"]
                    scope: "ListView"
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Move duration")
                tooltip: qsTr("Move animation duration of the highlight delegate.")
                blockedByTemplate: !backendValues.highlightMoveDuration.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.highlightMoveDuration
                    minimumValue: -1
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Move velocity")
                tooltip: qsTr("Move animation velocity of the highlight delegate.")
                blockedByTemplate: !backendValues.highlightMoveVelocity.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.highlightMoveVelocity
                    minimumValue: -1
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Resize duration")
                tooltip: qsTr("Resize animation duration of the highlight delegate.")
                blockedByTemplate: !backendValues.highlightResizeDuration.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.highlightResizeDuration
                    minimumValue: -1
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Resize velocity")
                tooltip: qsTr("Resize animation velocity of the highlight delegate.")
                blockedByTemplate: !backendValues.highlightResizeVelocity.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.highlightResizeVelocity
                    minimumValue: -1
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Preferred begin")
                tooltip: qsTr("Preferred highlight begin - must be smaller than Preferred end.")
                blockedByTemplate: !backendValues.preferredHighlightBegin.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.preferredHighlightBegin
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Preferred end")
                tooltip: qsTr("Preferred highlight end - must be larger than Preferred begin.")
                blockedByTemplate: !backendValues.preferredHighlightEnd.isAvailable
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.preferredHighlightEnd
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Follows current")
                tooltip: qsTr("Whether the highlight is managed by the view.")
                blockedByTemplate: !backendValues.highlightFollowsCurrentItem.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.highlightFollowsCurrentItem.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.highlightFollowsCurrentItem
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }
        }
    }
}
