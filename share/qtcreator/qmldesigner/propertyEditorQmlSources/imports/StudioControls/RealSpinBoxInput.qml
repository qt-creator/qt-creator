// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

TextInput {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property T.Control __parentControl

    property bool edit: control.activeFocus
    property bool drag: false
    property bool hover: mouseArea.containsMouse && control.enabled

    property int devicePixelRatio: 1
    property int pixelsPerUnit: 10

    z: 2
    font: control.__parentControl.font
    color: control.style.text.idle
    selectionColor: control.style.text.selection
    selectedTextColor: control.style.text.selectedText

    horizontalAlignment: Qt.AlignRight
    verticalAlignment: Qt.AlignVCenter
    leftPadding: control.style.inputHorizontalPadding
    rightPadding: control.style.inputHorizontalPadding

    readOnly: !control.__parentControl.editable
    validator: control.__parentControl.validator
    inputMethodHints: control.__parentControl.inputMethodHints
    selectByMouse: false
    activeFocusOnPress: false
    clip: true

    // TextInput focus needs to be set to activeFocus whenever it changes,
    // otherwise TextInput will get activeFocus whenever the parent SpinBox gets
    // activeFocus. This will lead to weird side effects.
    onActiveFocusChanged: control.focus = control.activeFocus

    Rectangle {
        id: textInputBackground
        x: 0
        y: control.style.borderWidth
        z: -1
        width: control.width
        height: control.style.controlSize.height - (control.style.borderWidth * 2)
        color: control.style.background.idle
        border.width: 0
    }

    Item {
        id: dragModifierWorkaround
        Keys.onPressed: function(event) {
            event.accepted = true

            if (event.modifiers & Qt.ControlModifier) {
                mouseArea.stepSize = control.__parentControl.minStepSize
                mouseArea.calcValue()
                control.__parentControl.realValueModified()
            }

            if (event.modifiers & Qt.ShiftModifier) {
                mouseArea.stepSize = control.__parentControl.maxStepSize
                mouseArea.calcValue()
                control.__parentControl.realValueModified()
            }
        }
        Keys.onReleased: function(event) {
            event.accepted = true
            mouseArea.stepSize = control.__parentControl.realStepSize
            mouseArea.calcValue()
            control.__parentControl.realValueModified()
        }
    }

    // Ensure that we get Up and Down key press events first
    Keys.onShortcutOverride: function(event) {
        event.accepted = (event.key === Qt.Key_Up || event.key === Qt.Key_Down)
    }

    MouseArea {
        id: mouseArea

        property real stepSize: control.__parentControl.realStepSize

        // Properties to store the state of a drag operation
        property bool dragging: false
        property bool hasDragged: false
        property bool potentialDragStart: false

        property real initialValue: control.__parentControl.realValue // value on drag operation starts

        property real pressStartX: 0.0
        property real dragStartX: 0.0
        property real translationX: 0.0

        property real dragDirection: 0.0
        property real totalUnits: 0.0 // total number of units dragged
        property real units: 0.0

        property real __pixelsPerUnit: control.devicePixelRatio * control.pixelsPerUnit

        anchors.fill: parent
        enabled: true
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.PointingHandCursor
        preventStealing: true

        onPositionChanged: function(mouse) {
            if (!mouseArea.dragging
                    && !control.__parentControl.edit
                    && Math.abs(mouseArea.pressStartX - mouse.x) > StudioTheme.Values.dragThreshold
                    && mouse.buttons === Qt.LeftButton
                    && mouseArea.potentialDragStart) {
                mouseArea.dragging = true
                mouseArea.potentialDragStart = false
                mouseArea.initialValue = control.__parentControl.realValue
                mouseArea.cursorShape = Qt.ClosedHandCursor
                mouseArea.dragStartX = mouse.x

                control.__parentControl.drag = true
                control.__parentControl.dragStarted()
                // Force focus on the non visible component to receive key events
                dragModifierWorkaround.forceActiveFocus()
                control.deselect()
            }

            if (!mouseArea.dragging)
                return

            mouse.accepted = true

            var translationX = mouse.x - mouseArea.dragStartX

            // Early return if mouse didn't move along x-axis
            if (translationX === 0.0)
                return

            var currentDragDirection = Math.sign(translationX)

            // Has drag direction changed
            if (currentDragDirection !== mouseArea.dragDirection) {
                mouseArea.translationX = 0.0
                mouseArea.dragDirection = currentDragDirection
                mouseArea.totalUnits = mouseArea.units
            }

            mouseArea.translationX += translationX
            mouseArea.calcValue()
            control.__parentControl.realValueModified()
        }

        onClicked: function(mouse) {
            if (control.edit)
                mouse.accepted = false

            if (mouseArea.hasDragged) {
                mouseArea.hasDragged = false
                return
            }

            control.forceActiveFocus()
            control.deselect() // QTBUG-75862
        }

        onPressed: function(mouse) {
            if (control.edit)
                mouse.accepted = false

            mouseArea.potentialDragStart = true
            mouseArea.pressStartX = mouse.x
        }

        onReleased: function(mouse) {
            if (control.edit)
                mouse.accepted = false

            mouseArea.endDrag()
        }

        function endDrag() {
            if (!mouseArea.dragging)
                return

            mouseArea.dragging = false
            mouseArea.hasDragged = true

            if (control.__parentControl.compressedValueTimer.running) {
                control.__parentControl.compressedValueTimer.stop()
                mouseArea.calcValue()
                control.__parentControl.compressedRealValueModified()
            }
            mouseArea.cursorShape = Qt.PointingHandCursor
            control.__parentControl.drag = false
            control.__parentControl.dragEnded()
            // Avoid active focus on the component after dragging
            dragModifierWorkaround.focus = false
            control.focus = false
            control.__parentControl.focus = false

            mouseArea.translationX = 0.0
            mouseArea.units = 0.0
            mouseArea.totalUnits = 0.0
        }

        function calcValue() {
            var minUnit = (control.__parentControl.realFrom - mouseArea.initialValue) / mouseArea.stepSize
            var maxUnit = (control.__parentControl.realTo - mouseArea.initialValue) / mouseArea.stepSize

            var units = Math.trunc(mouseArea.translationX / mouseArea.__pixelsPerUnit)
            mouseArea.units = Math.min(Math.max(mouseArea.totalUnits + units, minUnit), maxUnit)
            control.__parentControl.setRealValue(mouseArea.initialValue + (mouseArea.units * mouseArea.stepSize))

            if (mouseArea.dragging)
                control.__parentControl.dragging()
        }

        onWheel: function(wheel) {
            if (!control.__parentControl.__wheelEnabled) {
                wheel.accepted = false
                return
            }

            // Set stepSize according to used modifier key
            if (wheel.modifiers & Qt.ControlModifier)
                mouseArea.stepSize = control.__parentControl.minStepSize

            if (wheel.modifiers & Qt.ShiftModifier)
                mouseArea.stepSize = control.__parentControl.maxStepSize

            control.__parentControl.valueFromText(control.text, control.__parentControl.locale)
            control.__parentControl.setRealValue(__parentControl.realValue + (wheel.angleDelta.y / 120.0 * mouseArea.stepSize))
            control.__parentControl.realValueModified()

            // Reset stepSize
            mouseArea.stepSize = control.__parentControl.realStepSize
        }
    }

    states: [
        State {
            name: "default"
            when: control.__parentControl.enabled && !control.edit && !control.hover
                  && !control.__parentControl.hover && !control.__parentControl.drag
                  && !control.__parentControl.sliderDrag
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.idle
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.PointingHandCursor
            }
        },
        State {
            name: "globalHover"
            when: control.__parentControl.hover && !control.hover
                  && !control.edit && !control.__parentControl.drag
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.globalHover
            }
        },
        State {
            name: "hover"
            when: control.hover && control.__parentControl.hover && !control.edit
                  && !control.__parentControl.drag
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.hover
            }
        },
        State {
            name: "edit"
            when: control.edit && !control.__parentControl.drag
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.interaction
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.IBeamCursor
            }
        },
        State {
            name: "drag"
            when: control.__parentControl.drag
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.interaction
            }
            PropertyChanges {
                target: control
                color: control.style.interaction
            }
        },
        State {
            name: "sliderDrag"
            when: control.__parentControl.sliderDrag
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.idle
            }
            PropertyChanges {
                target: control
                color: control.style.interaction
            }
        },
        State {
            name: "disable"
            when: !control.__parentControl.enabled
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.disabled
            }
            PropertyChanges {
                target: control
                color: control.style.text.disabled
            }
        }
    ]
}
