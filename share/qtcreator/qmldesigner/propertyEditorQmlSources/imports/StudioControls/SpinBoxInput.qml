// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

TextInput {
    id: textInput

    property T.Control myControl

    property bool edit: textInput.activeFocus
    property bool drag: false
    property bool hover: mouseArea.containsMouse && textInput.enabled

    property int devicePixelRatio: 1
    property int pixelsPerUnit: 10

    z: 2
    font: myControl.font
    color: StudioTheme.Values.themeTextColor
    selectionColor: StudioTheme.Values.themeTextSelectionColor
    selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor

    horizontalAlignment: Qt.AlignRight
    verticalAlignment: Qt.AlignVCenter
    leftPadding: StudioTheme.Values.inputHorizontalPadding
    rightPadding: StudioTheme.Values.inputHorizontalPadding

    readOnly: !myControl.editable
    validator: myControl.validator
    inputMethodHints: myControl.inputMethodHints
    selectByMouse: false
    activeFocusOnPress: false
    clip: true

    // TextInput focus needs to be set to activeFocus whenever it changes,
    // otherwise TextInput will get activeFocus whenever the parent SpinBox gets
    // activeFocus. This will lead to weird side effects.
    onActiveFocusChanged: textInput.focus = textInput.activeFocus

    Rectangle {
        id: textInputBackground
        x: 0
        y: StudioTheme.Values.border
        z: -1
        width: textInput.width
        height: StudioTheme.Values.height - (StudioTheme.Values.border * 2)
        color: StudioTheme.Values.themeControlBackground
        border.width: 0
    }

    Item {
        id: dragModifierWorkaround
        Keys.onPressed: function(event) {
            event.accepted = true

            if (event.modifiers & Qt.ControlModifier) {
                mouseArea.stepSize = myControl.minStepSize
                mouseArea.calcValue()
                myControl.valueModified()
            }

            if (event.modifiers & Qt.ShiftModifier) {
                mouseArea.stepSize = myControl.maxStepSize
                mouseArea.calcValue()
                myControl.valueModified()
            }
        }
        Keys.onReleased: function(event) {
            event.accepted = true
            mouseArea.stepSize = myControl.stepSize
            mouseArea.calcValue()
            myControl.valueModified()
        }
    }

    // Ensure that we get Up and Down key press events first
    Keys.onShortcutOverride: function(event) {
        event.accepted = (event.key === Qt.Key_Up || event.key === Qt.Key_Down)
    }

    MouseArea {
        id: mouseArea

        property real stepSize: myControl.stepSize

        // Properties to store the state of a drag operation
        property bool dragging: false
        property bool hasDragged: false
        property bool potentialDragStart: false

        property int initialValue: myControl.value // value on drag operation starts

        property real pressStartX: 0.0
        property real dragStartX: 0.0
        property real translationX: 0.0

        property real dragDirection: 0.0
        property real totalUnits: 0.0 // total number of units dragged
        property real units: 0.0

        property real __pixelsPerUnit: textInput.devicePixelRatio * textInput.pixelsPerUnit

        anchors.fill: parent
        enabled: true
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.PointingHandCursor
        preventStealing: true

        onPositionChanged: function(mouse) {
            if (!mouseArea.dragging
                    && !myControl.edit
                    && Math.abs(mouseArea.pressStartX - mouse.x) > StudioTheme.Values.dragThreshold
                    && mouse.buttons === Qt.LeftButton
                    && mouseArea.potentialDragStart) {
                mouseArea.dragging = true
                mouseArea.potentialDragStart = false
                mouseArea.initialValue = myControl.value
                mouseArea.cursorShape = Qt.ClosedHandCursor
                mouseArea.dragStartX = mouse.x

                myControl.drag = true
                myControl.dragStarted()
                // Force focus on the non visible component to receive key events
                dragModifierWorkaround.forceActiveFocus()
                textInput.deselect()
            }

            if (!mouseArea.dragging)
                return

            mouse.accepted = true

            var translationX = mouse.x - mouseArea.dragStartX

            // Early return if mouse didn't move along x-axis
            if (translationX === 0)
                return

            var currentDragDirection = Math.sign(translationX)

            // Has drag direction changed
            if (currentDragDirection !== mouseArea.dragDirection) {
                mouseArea.translationX = 0
                mouseArea.dragDirection = currentDragDirection
                mouseArea.totalUnits = mouseArea.units
            }

            mouseArea.translationX += translationX
            mouseArea.calcValue()
            myControl.valueModified()
        }

        onClicked: function(mouse) {
            if (textInput.edit)
                mouse.accepted = false

            if (mouseArea.hasDragged) {
                mouseArea.hasDragged = false
                return
            }

            textInput.forceActiveFocus()
            textInput.deselect() // QTBUG-75862
        }

        onPressed: function(mouse) {
            if (textInput.edit)
                mouse.accepted = false

            mouseArea.potentialDragStart = true
            mouseArea.pressStartX = mouse.x
        }

        onReleased: function(mouse) {
            if (textInput.edit)
                mouse.accepted = false

            mouseArea.endDrag()
        }

        function endDrag() {
            if (!mouseArea.dragging)
                return

            mouseArea.dragging = false
            mouseArea.hasDragged = true

            if (myControl.compressedValueTimer.running) {
                myControl.compressedValueTimer.stop()
                mouseArea.calcValue()
                myControl.compressedValueModified()
            }
            mouseArea.cursorShape = Qt.PointingHandCursor
            myControl.drag = false
            myControl.dragEnded()
            // Avoid active focus on the component after dragging
            dragModifierWorkaround.focus = false
            textInput.focus = false
            myControl.focus = false

            mouseArea.translationX = 0
            mouseArea.units = 0
            mouseArea.totalUnits = 0
        }

        function calcValue() {
            var minUnit = (myControl.from - mouseArea.initialValue) / mouseArea.stepSize
            var maxUnit = (myControl.to - mouseArea.initialValue) / mouseArea.stepSize

            var units = Math.trunc(mouseArea.translationX / mouseArea.__pixelsPerUnit)
            mouseArea.units = Math.min(Math.max(mouseArea.totalUnits + units, minUnit), maxUnit)
            myControl.value = mouseArea.initialValue + (mouseArea.units * mouseArea.stepSize)

            if (mouseArea.dragging)
                myControl.dragging()
        }

        onWheel: function(wheel) {
            if (!myControl.__wheelEnabled) {
                wheel.accepted = false
                return
            }

            // Set stepSize according to used modifier key
            if (wheel.modifiers & Qt.ControlModifier)
                mouseArea.stepSize = myControl.minStepSize

            if (wheel.modifiers & Qt.ShiftModifier)
                mouseArea.stepSize = myControl.maxStepSize

            var val = myControl.valueFromText(textInput.text, myControl.locale)
            if (myControl.value !== val)
                myControl.value = val

            var currValue = myControl.value
            myControl.value += (wheel.angleDelta.y / 120 * mouseArea.stepSize)

            if (currValue !== myControl.value)
                myControl.valueModified()

            // Reset stepSize
            mouseArea.stepSize = myControl.stepSize
        }
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !textInput.edit && !textInput.hover && !myControl.hover
                  && !myControl.drag && !myControl.sliderDrag
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackground
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.PointingHandCursor
            }
        },
        State {
            name: "globalHover"
            when: myControl.hover && !textInput.hover && !textInput.edit && !myControl.drag
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
            }
        },
        State {
            name: "hover"
            when: textInput.hover && myControl.hover
                  && !textInput.edit && !myControl.drag
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        },
        State {
            name: "edit"
            when: textInput.edit && !myControl.drag
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.IBeamCursor
            }
        },
        State {
            name: "drag"
            when: myControl.drag
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }
            PropertyChanges {
                target: textInput
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "sliderDrag"
            when: myControl.sliderDrag
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackground
            }
            PropertyChanges {
                target: textInput
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "disable"
            when: !myControl.enabled
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
            }
            PropertyChanges {
                target: textInput
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
