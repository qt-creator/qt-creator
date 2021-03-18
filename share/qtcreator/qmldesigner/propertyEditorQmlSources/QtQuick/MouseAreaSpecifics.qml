/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Mouse Area")

        SectionLayout {
            Label {
                text: qsTr("Enabled")
                tooltip: qsTr("Accepts mouse events.")
                disabledState: !backendValues.enabled.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.enabled
                    text: backendValues.enabled.valueToString
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Hover enabled")
                tooltip: qsTr("Handles hover events.")
                disabledState: !backendValues.hoverEnabled.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.hoverEnabled
                    text: backendValues.hoverEnabled.valueToString
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Accepted buttons")
                tooltip: qsTr("Mouse buttons that the mouse area reacts to.")
                disabledState: !backendValues.acceptedButtons.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    backendValue: backendValues.acceptedButtons
                    model: ["LeftButton", "RightButton", "MiddleButton", "BackButton", "ForwardButton", "AllButtons"]
                    Layout.fillWidth: true
                    scope: "Qt"
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Press and hold interval")
                tooltip: qsTr("Overrides the elapsed time in milliseconds before pressAndHold signal is emitted.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.pressAndHoldInterval
                    minimumValue: 0
                    maximumValue: 2000
                    decimals: 0
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Scroll gesture enabled")
                tooltip: qsTr("Responds to scroll gestures from non-mouse devices.")
                disabledState: !backendValues.scrollGestureEnabled.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.scrollGestureEnabled
                    text: backendValues.scrollGestureEnabled.valueToString
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Cursor shape")
                tooltip: qsTr("Cursor shape for this mouse area.")
                disabledState: !backendValues.cursorShape.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    backendValue: backendValues.cursorShape
                    model: ["ArrowCursor", "UpArrowCursor", "CrossCursor", "WaitCursor",
                            "IBeamCursor", "SizeVerCursor", "SizeHorCursor", "SizeBDiagCursor",
                            "SizeFDiagCursor", "SizeAllCursor", "BlankCursor", "SplitVCursor",
                            "SplitHCursor", "PointingHandCursor", "ForbiddenCursor", "WhatsThisCursor",
                            "BusyCursor", "OpenHandCursor", "ClosedHandCursor", "DragCopyCursor",
                            "DragMoveCursor", "DragLinkCursor"]
                    Layout.fillWidth: true
                    scope: "Qt"
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Prevent stealing")
                tooltip: qsTr("Stops mouse events from being stolen from this mouse area.")
                disabledState: !backendValues.preventStealing.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.preventStealing
                    text: backendValues.preventStealing.valueToString
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Propagate composed events")
                tooltip: qsTr("Automatically propagates composed mouse events to other mouse areas.")
                disabledState: !backendValues.propagateComposedEvents.isAvailable
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.propagateComposedEvents
                    text: backendValues.propagateComposedEvents.valueToString
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {
                }
            }
        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Drag")
        visible: backendValues.drag_target.isAvailable

        SectionLayout {
            Label {
                text: qsTr("Target")
                tooltip: qsTr("ID of the component to drag.")
            }
            SecondColumnLayout {
                ItemFilterComboBox {
                    typeFilter: "QtQuick.QtObject"
                    validator: RegExpValidator { regExp: /(^$|^[a-z_]\w*)/ }
                    backendValue: backendValues.drag_target
                    Layout.fillWidth: true
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Axis")
                tooltip: qsTr("Whether dragging can be done horizontally, vertically, or both.")
            }
            SecondColumnLayout {
                ComboBox {
                    scope: "Drag"
                    model: ["XAxis", "YAxis", "XAndYAxis"]
                    backendValue: backendValues.drag_axis
                    Layout.fillWidth: true
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Filter children")
                tooltip: qsTr("Whether dragging overrides descendant mouse areas.")
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.drag_filterChildren
                    text: backendValues.drag_filterChildren.valueToString
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Threshold")
                tooltip: qsTr("Threshold in pixels of when the drag operation should start.")
            }

            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.drag_threshold
                    minimumValue: 0
                    maximumValue: 5000
                    decimals: 0
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Smoothed")
                tooltip: qsTr("Moves targets only after the drag operation has started.\n"
                              + "When disabled, moves targets straight to the current mouse position.")
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.drag_smoothed
                    text: backendValues.drag_smoothed.valueToString
                }

                ExpandingSpacer {
                }
            }
        }
    }
}
