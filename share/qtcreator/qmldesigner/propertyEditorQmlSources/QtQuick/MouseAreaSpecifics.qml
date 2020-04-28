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
                tooltip: qsTr("This property holds whether the item accepts mouse events.")
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.enabled
                    text: backendValues.enabled.valueToString
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Hover enabled")
                tooltip: qsTr("This property holds whether hover events are handled.")
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.hoverEnabled
                    text: backendValues.hoverEnabled.valueToString
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Accepted buttons")
                tooltip: qsTr("This property holds the mouse buttons that the mouse area reacts to.")
            }

            SecondColumnLayout {
                ComboBox {
                    backendValue: backendValues.acceptedButtons
                    model: ["LeftButton", "RightButton", "MiddleButton", "BackButton", "ForwardButton", "AllButtons"]
                    Layout.fillWidth: true
                    scope: "Qt"
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Press and hold interval")
                tooltip: qsTr("This property overrides the elapsed time in milliseconds before pressAndHold is emitted.")
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
                tooltip: qsTr("This property controls whether this MouseArea responds to scroll gestures from non-mouse devices.")
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.scrollGestureEnabled
                    text: backendValues.scrollGestureEnabled.valueToString
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Cursor shape")
                tooltip: qsTr("This property holds the cursor shape for this mouse area.")
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
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Prevent stealing")
                tooltip: qsTr("This property controls whether the mouse events may be stolen from this MouseArea.")
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.preventStealing
                    text: backendValues.preventStealing.valueToString
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Propagate composed events")
                tooltip: qsTr("This property controls whether composed mouse events will automatically propagate to other MouseAreas.")
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.propagateComposedEvents
                    text: backendValues.propagateComposedEvents.valueToString
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

        SectionLayout {
            Label {
                text: qsTr("Target")
                tooltip: qsTr("Sets the id of the item to drag.")
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
                tooltip: qsTr("Specifies whether dragging can be done horizontally, vertically, or both.")
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
                tooltip: qsTr("Specifies whether a drag overrides descendant MouseAreas.")
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
                tooltip: qsTr("Determines the threshold in pixels of when the drag operation should start.")
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
                tooltip: qsTr("If set to true, the target will be moved only after the drag operation has started.\n"
                              + "If set to false, the target will be moved straight to the current mouse position.")
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
