/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
                tooltip: qsTr("Accepts mouse events.")
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
                tooltip: qsTr("Mouse buttons that the mouse area reacts to.")
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
                tooltip: qsTr("Cursor shape for this mouse area.")
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
                tooltip: qsTr("Overrides the elapsed time in milliseconds before pressAndHold signal is emitted.")
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
                tooltip: qsTr("Responds to scroll gestures from non-mouse devices.")
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
                tooltip: qsTr("Stops mouse events from being stolen from this mouse area.")
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
                tooltip: qsTr("Automatically propagates composed mouse events to other mouse areas.")
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
                tooltip: qsTr("ID of the component to drag.")
            }

            SecondColumnLayout {
                ItemFilterComboBox {
                    typeFilter: "QtQuick.QtObject"
                    validator: RegExpValidator { regExp: /(^$|^[a-z_]\w*)/ }
                    backendValue: backendValues.drag_target
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Axis")
                tooltip: qsTr("Whether dragging can be done horizontally, vertically, or both.")
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
                tooltip: qsTr("Threshold in pixels of when the drag operation should start.")
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
                tooltip: qsTr("Whether dragging overrides descendant mouse areas.")
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
                tooltip: qsTr("Moves targets only after the drag operation has started.\n"
                              + "When disabled, moves targets straight to the current mouse position.")
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
