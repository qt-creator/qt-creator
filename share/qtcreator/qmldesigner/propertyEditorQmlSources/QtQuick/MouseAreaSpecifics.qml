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
        caption: qsTr("Mouse Area")

        anchors.left: parent.left
        anchors.right: parent.right

        SectionLayout {
            PropertyLabel {
                text: qsTr("Enable")
                tooltip: qsTr("Sets how the mouse can interact with the area.")
                blockedByTemplate: !backendValues.enabled.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: qsTr("Area")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.enabled
                    enabled: backendValue.isAvailable
                }

                Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

                CheckBox {
                    text: qsTr("Hover")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.hoverEnabled
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Accepted buttons")
                tooltip: qsTr("Sets which mouse buttons the area reacts to.")
                blockedByTemplate: !backendValues.acceptedButtons.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.acceptedButtons
                    model: ["LeftButton", "RightButton", "MiddleButton", "BackButton", "ForwardButton", "AllButtons"]
                    scope: "Qt"
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Cursor shape")
                tooltip: qsTr("Sets which mouse cursor to display on this area.")
                blockedByTemplate: !backendValues.cursorShape.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    backendValue: backendValues.cursorShape
                    model: ["ArrowCursor", "UpArrowCursor", "CrossCursor", "WaitCursor",
                            "IBeamCursor", "SizeVerCursor", "SizeHorCursor", "SizeBDiagCursor",
                            "SizeFDiagCursor", "SizeAllCursor", "BlankCursor", "SplitVCursor",
                            "SplitHCursor", "PointingHandCursor", "ForbiddenCursor", "WhatsThisCursor",
                            "BusyCursor", "OpenHandCursor", "ClosedHandCursor", "DragCopyCursor",
                            "DragMoveCursor", "DragLinkCursor"]
                    scope: "Qt"
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Hold interval")
                tooltip: qsTr("Sets the time before the pressAndHold signal is registered when you press the area.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.pressAndHoldInterval
                    minimumValue: 0
                    maximumValue: 2000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Scroll gesture")
                tooltip: qsTr("Toggles if scroll gestures from non-mouse devices are supported.")
                blockedByTemplate: !backendValues.scrollGestureEnabled.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: qsTr("Enabled")
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.scrollGestureEnabled
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Prevent stealing")
                tooltip: qsTr("Toggles if mouse events can be stolen from this area.")
                blockedByTemplate: !backendValues.preventStealing.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.preventStealing.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.preventStealing
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Propagate events")
                tooltip: qsTr("Toggles if composed mouse events should be propagated to other mouse areas overlapping this area.")
                blockedByTemplate: !backendValues.propagateComposedEvents.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.propagateComposedEvents.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.propagateComposedEvents
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        caption: qsTr("Drag")

        anchors.left: parent.left
        anchors.right: parent.right
        visible: backendValues.drag_target.isAvailable

        SectionLayout {
            PropertyLabel {
                text: qsTr("Target")
                tooltip: qsTr("Sets the component to have drag functionalities.")
            }

            SecondColumnLayout {
                ItemFilterComboBox {
                    typeFilter: "QtQuick.QtObject"
                    backendValue: backendValues.drag_target
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Axis")
                tooltip: qsTr("Sets in which directions the dragging work.")
            }

            SecondColumnLayout {
                ComboBox {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    width: implicitWidth
                    scope: "Drag"
                    model: ["XAxis", "YAxis", "XAndYAxis"]
                    backendValue: backendValues.drag_axis
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Threshold")
                tooltip: qsTr("Sets a threshold after which the drag starts to work.")
            }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.drag_threshold
                    minimumValue: 0
                    maximumValue: 5000
                    decimals: 0
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Filter children")
                tooltip: qsTr("Toggles if the dragging overrides descendant mouse areas.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.drag_filterChildren.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.drag_filterChildren
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Smoothed")
                tooltip: qsTr("Toggles if the move is smoothly animated.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: backendValues.drag_smoothed.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.drag_smoothed
                }

                ExpandingSpacer {}
            }
        }
    }
}
