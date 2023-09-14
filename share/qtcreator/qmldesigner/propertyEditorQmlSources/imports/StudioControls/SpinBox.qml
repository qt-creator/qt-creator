// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.SpinBox {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias labelColor: spinBoxInput.color
    property alias actionIndicator: actionIndicator

    property int decimals: 0
    property int factor: Math.pow(10, control.decimals)

    property real minStepSize: 1
    property real maxStepSize: 10

    property bool edit: spinBoxInput.activeFocus
    // This property is used to indicate the global hover state
    property bool hover: (spinBoxInput.hover || actionIndicator.hover || spinBoxIndicatorUp.hover
                         || spinBoxIndicatorDown.hover || sliderIndicator.hover)
                         && control.enabled
    property bool drag: false
    property bool sliderDrag: sliderPopup.drag

    property bool dirty: false // user modification flag

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: control.style.actionIndicatorSize.width
    property real __actionIndicatorHeight: control.style.actionIndicatorSize.height

    property bool spinBoxIndicatorVisible: true
    property real __spinBoxIndicatorWidth: control.style.spinBoxIndicatorSize.width
    property real __spinBoxIndicatorHeight: control.style.spinBoxIndicatorSize.height

    property alias sliderIndicatorVisible: sliderIndicator.visible
    property real __sliderIndicatorWidth: control.style.squareControlSize.width
    property real __sliderIndicatorHeight: control.style.squareControlSize.height

    property alias __devicePixelRatio: spinBoxInput.devicePixelRatio
    property alias pixelsPerUnit: spinBoxInput.pixelsPerUnit
    property alias inputHAlignment: spinBoxInput.horizontalAlignment

    property alias compressedValueTimer: myTimer

    property string preFocusText: ""

    signal compressedValueModified
    signal dragStarted
    signal dragEnded
    signal dragging

    // Use custom wheel handling due to bugs
    property bool __wheelEnabled: false
    wheelEnabled: false
    hoverEnabled: true

    width: control.style.controlSize.width
    height: control.style.controlSize.height

    leftPadding: spinBoxIndicatorDown.x + spinBoxIndicatorDown.width
    rightPadding: sliderIndicator.width + control.style.borderWidth

    font.pixelSize: control.style.baseFontSize
    editable: true
    validator: control.decimals ? doubleValidator : intValidator

    DoubleValidator {
        id: doubleValidator
        locale: control.locale.name
        notation: DoubleValidator.StandardNotation
        decimals: control.decimals
        bottom: Math.min(control.from, control.to) / control.factor
        top: Math.max(control.from, control.to) / control.factor
    }

    IntValidator {
        id: intValidator
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    ActionIndicator {
        id: actionIndicator
        style: control.style
        __parentControl: control
        x: 0
        y: 0
        width: actionIndicator.visible ? control.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? control.__actionIndicatorHeight : 0
    }

    up.indicator: SpinBoxIndicator {
        id: spinBoxIndicatorUp
        style: control.style
        __parentControl: control
        iconFlip: -1
        visible: control.spinBoxIndicatorVisible
        pressed: control.up.pressed
        x: actionIndicator.width + control.style.borderWidth
        y: control.style.borderWidth
        width: control.spinBoxIndicatorVisible ? control.__spinBoxIndicatorWidth : 0
        height: control.spinBoxIndicatorVisible ? control.__spinBoxIndicatorHeight : 0

        enabled: (control.from < control.to) ? control.value < control.to
                                             : control.value > control.to
    }

    down.indicator: SpinBoxIndicator {
        id: spinBoxIndicatorDown
        style: control.style
        __parentControl: control
        visible: control.spinBoxIndicatorVisible
        pressed: control.down.pressed
        x: actionIndicator.width + control.style.borderWidth
        y: spinBoxIndicatorUp.y + spinBoxIndicatorUp.height
        width: control.spinBoxIndicatorVisible ? control.__spinBoxIndicatorWidth : 0
        height: control.spinBoxIndicatorVisible ? control.__spinBoxIndicatorHeight : 0

        enabled: (control.from < control.to) ? control.value > control.from
                                             : control.value < control.from
    }

    contentItem: SpinBoxInput {
        id: spinBoxInput
        style: control.style
        __parentControl: control

        function handleEditingFinished() {
            control.focus = false

            // Keep the dirty state before calling setValueFromInput(),
            // it will be set to false (cleared) internally
            var valueModified = control.dirty

            control.setValueFromInput()
            myTimer.stop()

            // Only trigger the signal, if the value was modified
            if (valueModified)
                control.compressedValueModified()
        }

        onEditingFinished: spinBoxInput.handleEditingFinished()
    }

    background: Rectangle {
        id: spinBoxBackground
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth
        x: actionIndicator.width
        width: control.width - actionIndicator.width
        height: control.height
    }

    CheckIndicator {
        id: sliderIndicator
        style: control.style
        __parentControl: control
        __parentPopup: sliderPopup
        x: spinBoxInput.x + spinBoxInput.width
        y: control.style.borderWidth
        width: sliderIndicator.visible ? control.__sliderIndicatorWidth - control.style.borderWidth : 0
        height: sliderIndicator.visible ? control.__sliderIndicatorHeight - (control.style.borderWidth * 2) : 0
        visible: false // reasonable default
    }

    SliderPopup {
        id: sliderPopup
        style: control.style
        __parentControl: control
        x: actionIndicator.width + control.style.borderWidth
        y: control.style.controlSize.height
        width: control.width - actionIndicator.width - (control.style.borderWidth * 2)
        height: control.style.smallControlSize.height

        enter: Transition {}
        exit: Transition {}
    }

    textFromValue: function (value, locale) {
        locale.numberOptions = Locale.OmitGroupSeparator
        return Number(value / control.factor).toLocaleString(locale, 'f',
                                                             control.decimals)
    }

    valueFromText: function (text, locale) {
        return Number.fromLocaleString(locale, text) * control.factor
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hover && !control.hovered
                  && !control.edit && !control.drag && !control.sliderDrag
            PropertyChanges {
                target: control
                __wheelEnabled: false
            }
            PropertyChanges {
                target: spinBoxInput
                selectByMouse: false
            }
            PropertyChanges {
                target: spinBoxBackground
                border.color: control.style.border.idle
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hover && control.hovered
                  && !control.edit && !control.drag && !control.sliderDrag
            PropertyChanges {
                target: spinBoxBackground
                border.color: control.style.border.hover
            }
        },
        State {
            name: "edit"
            when: control.edit
            PropertyChanges {
                target: control
                __wheelEnabled: true
            }
            PropertyChanges {
                target: spinBoxInput
                selectByMouse: true
            }
            PropertyChanges {
                target: spinBoxBackground
                border.color: control.style.border.idle
            }
        },
        State {
            name: "drag"
            when: control.drag || control.sliderDrag
            PropertyChanges {
                target: spinBoxBackground
                border.color: control.style.border.interaction
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: spinBoxBackground
                border.color: control.style.border.disabled
            }
        }
    ]

    Timer {
        id: myTimer
        repeat: false
        running: false
        interval: 400
        onTriggered: control.compressedValueModified()
    }

    onValueModified: myTimer.restart()
    onFocusChanged: control.setValueFromInput()
    onDisplayTextChanged: spinBoxInput.text = control.displayText
    onActiveFocusChanged: {
        if (control.activeFocus) { // QTBUG-75862 && mySpinBox.focusReason === Qt.TabFocusReason)
            control.preFocusText = spinBoxInput.text
            spinBoxInput.selectAll()
        }

        if (sliderPopup.opened && !control.activeFocus)
            sliderPopup.close()
    }
    onDecimalsChanged: spinBoxInput.text = control.textFromValue(control.value, control.locale)

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            event.accepted = true

            // Store current step size
            var currStepSize = control.stepSize

            if (event.modifiers & Qt.ControlModifier)
                control.stepSize = control.minStepSize

            if (event.modifiers & Qt.ShiftModifier)
                control.stepSize = control.maxStepSize

            // Check if value is in sync with text input, if not sync it!
            var val = control.valueFromText(spinBoxInput.text,
                                              control.locale)
            if (control.value !== val)
                control.value = val

            var currValue = control.value

            if (event.key === Qt.Key_Up)
                control.increase()
            else
                control.decrease()

            if (currValue !== control.value)
                control.valueModified()

            // Reset step size
            control.stepSize = currStepSize
        }

        if (event.key === Qt.Key_Escape) {
            spinBoxInput.text = control.preFocusText
            control.dirty = true
            spinBoxInput.handleEditingFinished()
        }

        // FIX: This is a temporary fix for QTBUG-74239
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            control.setValueFromInput()
    }

    function clamp(v, lo, hi) {
        return (v < lo || v > hi) ? Math.min(Math.max(lo, v), hi) : v
    }

    function setValueFromInput() {
        if (!control.dirty)
            return

        // FIX: This is a temporary fix for QTBUG-74239
        var currValue = control.value

        if (!spinBoxInput.acceptableInput)
            control.value = clamp(valueFromText(spinBoxInput.text,
                                                  control.locale),
                                    control.validator.bottom * control.factor,
                                    control.validator.top * control.factor)
        else
            control.value = valueFromText(spinBoxInput.text,
                                            control.locale)

        if (spinBoxInput.text !== control.displayText)
            spinBoxInput.text = control.displayText

        if (control.value !== currValue)
            control.valueModified()

        control.dirty = false
    }
}
