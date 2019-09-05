/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import StudioTheme 1.0 as StudioTheme

TextInput {
    id: textInput

    property T.Control myControl

    property bool edit: textInput.activeFocus
    property bool drag: false

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
    onActiveFocusChanged: textInput.focus = activeFocus

    Rectangle {
        id: textInputArea
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
        x: 0
        y: 0
        z: -1
        width: textInput.width
        height: StudioTheme.Values.height
    }

    DragHandler {
        id: dragHandler
        target: null
        acceptedDevices: PointerDevice.Mouse
        enabled: true

        property real initialValue: myControl.realValue
        property real multiplier: 1.0

        onActiveChanged: {
            if (dragHandler.active) {
                dragHandler.initialValue = myControl.realValue
                mouseArea.cursorShape = Qt.ClosedHandCursor // TODO
                myControl.drag = true
                myControl.dragStarted()
                // Force focus on the non visible component to receive key events
                dragModifierWorkaround.forceActiveFocus()
            } else {
                if (myControl.compressedValueTimer.running) {
                    myControl.compressedValueTimer.stop()
                    calcValue(myControl.compressedRealValueModified)
                }
                mouseArea.cursorShape = Qt.PointingHandCursor // TODO
                myControl.drag = false
                myControl.dragEnded()
                // Avoid active focus on the component after dragging
                dragModifierWorkaround.focus = false
                textInput.focus = false
                myControl.focus = false
            }
        }
        onTranslationChanged: calcValue(myControl.realValueModified)
        onMultiplierChanged: calcValue(myControl.realValueModified)

        function calcValue(callback) {
            var tmp = myControl.realDragRange / StudioTheme.Values.dragLength
            myControl.setRealValue(dragHandler.initialValue + (tmp * dragHandler.translation.x * dragHandler.multiplier))
            callback()
        }
    }

    Item {
        id: dragModifierWorkaround
        Keys.onPressed: {
            event.accepted = true

            if (event.modifiers & Qt.ControlModifier)
                dragHandler.multiplier = 0.1

            if (event.modifiers & Qt.ShiftModifier)
                dragHandler.multiplier = 10.0
        }
        Keys.onReleased: {
            event.accepted = true
            dragHandler.multiplier = 1.0
        }
    }

    TapHandler {
        id: tapHandler
        acceptedDevices: PointerDevice.Mouse
        enabled: true
        onTapped: {
            textInput.forceActiveFocus()
            textInput.deselect() // QTBUG-75862
        }
    }

    MouseArea {
        id: mouseArea

        property real stepSize: myControl.realStepSize

        anchors.fill: parent
        enabled: true
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.PointingHandCursor
        // Sets the global hover
        onContainsMouseChanged: myControl.hover = containsMouse
        onPressed: mouse.accepted = false
        onWheel: {
            if (!myControl.__wheelEnabled)
                return

            // Set stepSize according to used modifier key
            if (wheel.modifiers & Qt.ControlModifier)
                mouseArea.stepSize = myControl.minStepSize

            if (wheel.modifiers & Qt.ShiftModifier)
                mouseArea.stepSize = myControl.maxStepSize

            myControl.valueFromText(textInput.text, myControl.locale)
            myControl.setRealValue(myControl.realValue + (wheel.angleDelta.y / 120.0 * mouseArea.stepSize))
            myControl.realValueModified()

            // Reset stepSize
            mouseArea.stepSize = myControl.realStepSize
        }
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !textInput.edit
                  && !mouseArea.containsMouse && !myControl.drag
            PropertyChanges {
                target: textInputArea
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: dragHandler
                enabled: true
            }
            PropertyChanges {
                target: tapHandler
                enabled: true
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.PointingHandCursor
            }
        },
        State {
            name: "hovered"
            when: myControl.hover && !textInput.edit && !myControl.drag
            PropertyChanges {
                target: textInputArea
                color: StudioTheme.Values.themeHoverHighlight
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "edit"
            when: textInput.edit && !myControl.drag
            PropertyChanges {
                target: textInputArea
                color: StudioTheme.Values.themeFocusEdit
                border.color: StudioTheme.Values.themeInteraction
            }
            PropertyChanges {
                target: dragHandler
                enabled: false
            }
            PropertyChanges {
                target: tapHandler
                enabled: false
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
                target: textInputArea
                color: StudioTheme.Values.themeFocusDrag
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "disabled"
            when: !myControl.enabled
            PropertyChanges {
                target: textInputArea
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: textInput
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
