// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.SpinBox {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property real realFrom: 0.0
    property real realTo: 99.0
    property real realValue: 1.0
    property real realStepSize: 1.0

    property alias labelColor: spinBoxInput.color
    property alias actionIndicator: actionIndicator

    property int decimals: 0

    property real minStepSize: {
        var tmpMinStepSize = Number((control.realStepSize * 0.1).toFixed(control.decimals))
        return (tmpMinStepSize) ? tmpMinStepSize : control.realStepSize
    }
    property real maxStepSize: {
        var tmpMaxStepSize = Number((control.realStepSize * 10.0).toFixed(control.decimals))
        return (tmpMaxStepSize < control.realTo) ? tmpMaxStepSize : control.realStepSize
    }

    property bool edit: spinBoxInput.activeFocus
    // This property is used to indicate the global hover state
    property bool hover: (spinBoxInput.hover || actionIndicator.hover || spinBoxIndicatorUp.hover
                         || spinBoxIndicatorDown.hover || sliderIndicator.hover)
                         && control.enabled
    property bool drag: false
    property bool sliderDrag: sliderPopup.drag

    property bool trailingZeroes: true

    property bool dirty: false // user modification flag

    // TODO Not used anymore. Will be removed when all dependencies were removed.
    property real realDragRange: control.realTo - control.realFrom

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

    signal realValueModified
    signal compressedRealValueModified
    signal dragStarted
    signal dragEnded
    signal dragging
    signal indicatorPressed

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

    // Leave this in for now
    from: -99
    value: 0
    to: 99

    validator: DoubleValidator {
        id: doubleValidator
        locale: control.locale.name
        notation: DoubleValidator.StandardNotation
        decimals: control.decimals
        bottom: Math.min(control.realFrom, control.realTo)
        top: Math.max(control.realFrom, control.realTo)
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

    up.indicator: RealSpinBoxIndicator {
        id: spinBoxIndicatorUp
        style: control.style
        __parentControl: control
        iconFlip: -1
        visible: control.spinBoxIndicatorVisible
        onRealPressed: control.indicatorPressed()
        onRealReleased: control.realIncrease()
        onRealPressAndHold: control.realIncrease()
        x: actionIndicator.width + control.style.borderWidth
        y: control.style.borderWidth
        width: control.spinBoxIndicatorVisible ? control.__spinBoxIndicatorWidth : 0
        height: control.spinBoxIndicatorVisible ? control.__spinBoxIndicatorHeight : 0

        realEnabled: (control.realFrom < control.realTo) ? (control.realValue < control.realTo)
                                                         : (control.realValue > control.realTo)
    }

    down.indicator: RealSpinBoxIndicator {
        id: spinBoxIndicatorDown
        style: control.style
        __parentControl: control
        visible: control.spinBoxIndicatorVisible
        onRealPressed: control.indicatorPressed()
        onRealReleased: control.realDecrease()
        onRealPressAndHold: control.realDecrease()
        x: actionIndicator.width + control.style.borderWidth
        y: spinBoxIndicatorUp.y + spinBoxIndicatorUp.height
        width: control.spinBoxIndicatorVisible ? control.__spinBoxIndicatorWidth : 0
        height: control.spinBoxIndicatorVisible ? control.__spinBoxIndicatorHeight : 0

        realEnabled: (control.realFrom < control.realTo) ? (control.realValue > control.realFrom)
                                                         : (control.realValue < control.realFrom)
    }

    contentItem: RealSpinBoxInput {
        id: spinBoxInput
        style: control.style
        __parentControl: control
        validator: doubleValidator

        function handleEditingFinished() {
            control.focus = false

            // Keep the dirty state before calling setValueFromInput(),
            // it will be set to false (cleared) internally
            var valueModified = control.dirty

            control.setValueFromInput()
            myTimer.stop()

            // Only trigger the signal, if the value was modified
            if (valueModified)
                control.compressedRealValueModified()
        }

        onEditingFinished: spinBoxInput.handleEditingFinished()
        onTextEdited: control.dirty = true
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

    RealSliderPopup {
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
        var decimals = trailingZeroes ? control.decimals : decimalCounter(value)

        return Number(control.realValue).toLocaleString(locale, 'f', decimals)
    }

    valueFromText: function (text, locale) {
        control.setRealValue(Number.fromLocaleString(locale, spinBoxInput.text))

        return 0
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
        onTriggered: control.compressedRealValueModified()
    }

    onRealValueChanged: {
        control.setRealValue(control.realValue) // sanitize and clamp realValue
        spinBoxInput.text = control.textFromValue(control.realValue, control.locale)
        control.value = 0 // Without setting value back to 0, it can happen that one of
                          // the indicator will be disabled due to range logic.
    }
    onRealValueModified: myTimer.restart()
    onFocusChanged: {
        if (control.focus)
            control.dirty = false
    }
    onDisplayTextChanged: spinBoxInput.text = control.displayText
    onActiveFocusChanged: {
        if (control.activeFocus) { // QTBUG-75862 && mySpinBox.focusReason === Qt.TabFocusReason)
            control.preFocusText = spinBoxInput.text
            spinBoxInput.selectAll()
        } else {
            // Make sure displayed value is correct after focus loss, as onEditingFinished
            // doesn't trigger when value is something validator doesn't accept.
            if (spinBoxInput.text === "")
                spinBoxInput.text = control.textFromValue(control.realValue, control.locale)

            if (control.dirty)
                spinBoxInput.handleEditingFinished()
        }
    }
    onDecimalsChanged: spinBoxInput.text = control.textFromValue(control.realValue, control.locale)

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            event.accepted = true

            // Store current step size
            var currStepSize = control.realStepSize

            // Set realStepSize according to used modifier key
            if (event.modifiers & Qt.ControlModifier)
                control.realStepSize = control.minStepSize

            if (event.modifiers & Qt.ShiftModifier)
                control.realStepSize = control.maxStepSize

            if (event.key === Qt.Key_Up)
                control.realIncrease()
            else
                control.realDecrease()

            // Reset realStepSize
            control.realStepSize = currStepSize
        }

        if (event.key === Qt.Key_Escape) {
            spinBoxInput.text = control.preFocusText
            control.dirty = true
            spinBoxInput.handleEditingFinished()
        }
    }

    function clamp(v, lo, hi) {
        return (v < lo || v > hi) ? Math.min(Math.max(lo, v), hi) : v
    }

    function setValueFromInput() {
        if (!control.dirty)
            return

        // FIX: This is a temporary fix for QTBUG-74239
        var currValue = control.realValue

        // Call the function but don't use return value. The realValue property
        // will be implicitly set inside the function/procedure.
        control.valueFromText(spinBoxInput.text, control.locale)

        if (control.realValue !== currValue) {
            control.realValueModified()
        } else {
            // Check if input text differs in format from the current value
            var tmpInputValue = control.textFromValue(control.realValue, control.locale)

            if (tmpInputValue !== spinBoxInput.text)
                spinBoxInput.text = tmpInputValue
        }

        control.dirty = false
    }

    function setRealValue(value) {
        if (Number.isNaN(value))
            value = 0

        if (control.decimals === 0)
            value = Math.round(value)

        control.realValue = control.clamp(value,
                                          control.validator.bottom,
                                          control.validator.top)
    }

    function realDecrease() {
        // Store the current value for comparison
        var currValue = control.realValue
        control.valueFromText(spinBoxInput.text, control.locale)

        control.setRealValue(control.realValue - control.realStepSize)

        if (control.realValue !== currValue)
            control.realValueModified()
    }

    function realIncrease() {
        // Store the current value for comparison
        var currValue = control.realValue
        control.valueFromText(spinBoxInput.text, control.locale)

        control.setRealValue(control.realValue + control.realStepSize)

        if (control.realValue !== currValue)
            control.realValueModified()
    }

    function decimalCounter(number) {
        var strNumber = Math.abs(number).toString()
        var decimalIndex = strNumber.indexOf('.')

        // Set 'index' to a minimum of 1 if there are no fractions
        var index = decimalIndex == -1 ? 1 : strNumber.length - decimalIndex - 1

        return Math.min(index, control.decimals);
    }
}
