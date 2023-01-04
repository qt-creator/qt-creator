// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.SpinBox {
    id: mySpinBox

    property alias labelColor: spinBoxInput.color
    property alias actionIndicator: actionIndicator

    property int decimals: 0
    property int factor: Math.pow(10, decimals)

    property real minStepSize: 1
    property real maxStepSize: 10

    property bool edit: spinBoxInput.activeFocus
    // This property is used to indicate the global hover state
    property bool hover: (spinBoxInput.hover || actionIndicator.hover || spinBoxIndicatorUp.hover
                         || spinBoxIndicatorDown.hover || sliderIndicator.hover)
                         && mySpinBox.enabled
    property bool drag: false
    property bool sliderDrag: sliderPopup.drag

    property bool dirty: false // user modification flag

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
    property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

    property bool spinBoxIndicatorVisible: true
    property real __spinBoxIndicatorWidth: StudioTheme.Values.spinBoxIndicatorWidth
    property real __spinBoxIndicatorHeight: StudioTheme.Values.spinBoxIndicatorHeight

    property alias sliderIndicatorVisible: sliderIndicator.visible
    property real __sliderIndicatorWidth: StudioTheme.Values.sliderIndicatorWidth
    property real __sliderIndicatorHeight: StudioTheme.Values.sliderIndicatorHeight

    property alias __devicePixelRatio: spinBoxInput.devicePixelRatio
    property alias pixelsPerUnit: spinBoxInput.pixelsPerUnit

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

    width: StudioTheme.Values.defaultControlWidth
    height: StudioTheme.Values.defaultControlHeight

    leftPadding: spinBoxIndicatorDown.x + spinBoxIndicatorDown.width
    rightPadding: sliderIndicator.width + StudioTheme.Values.border

    font.pixelSize: StudioTheme.Values.myFontSize
    editable: true
    validator: mySpinBox.decimals ? doubleValidator : intValidator

    DoubleValidator {
        id: doubleValidator
        locale: mySpinBox.locale.name
        notation: DoubleValidator.StandardNotation
        decimals: mySpinBox.decimals
        bottom: Math.min(mySpinBox.from, mySpinBox.to) / mySpinBox.factor
        top: Math.max(mySpinBox.from, mySpinBox.to) / mySpinBox.factor
    }

    IntValidator {
        id: intValidator
        locale: mySpinBox.locale.name
        bottom: Math.min(mySpinBox.from, mySpinBox.to)
        top: Math.max(mySpinBox.from, mySpinBox.to)
    }

    ActionIndicator {
        id: actionIndicator
        myControl: mySpinBox
        x: 0
        y: 0
        width: actionIndicator.visible ? mySpinBox.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? mySpinBox.__actionIndicatorHeight : 0
    }

    up.indicator: SpinBoxIndicator {
        id: spinBoxIndicatorUp
        myControl: mySpinBox
        iconFlip: -1
        visible: mySpinBox.spinBoxIndicatorVisible
        pressed: mySpinBox.up.pressed
        x: actionIndicator.width + StudioTheme.Values.border
        y: StudioTheme.Values.border
        width: mySpinBox.spinBoxIndicatorVisible ? mySpinBox.__spinBoxIndicatorWidth : 0
        height: mySpinBox.spinBoxIndicatorVisible ? mySpinBox.__spinBoxIndicatorHeight : 0

        enabled: (mySpinBox.from < mySpinBox.to) ? mySpinBox.value < mySpinBox.to
                                                 : mySpinBox.value > mySpinBox.to
    }

    down.indicator: SpinBoxIndicator {
        id: spinBoxIndicatorDown
        myControl: mySpinBox
        visible: mySpinBox.spinBoxIndicatorVisible
        pressed: mySpinBox.down.pressed
        x: actionIndicator.width + StudioTheme.Values.border
        y: spinBoxIndicatorUp.y + spinBoxIndicatorUp.height
        width: mySpinBox.spinBoxIndicatorVisible ? mySpinBox.__spinBoxIndicatorWidth : 0
        height: mySpinBox.spinBoxIndicatorVisible ? mySpinBox.__spinBoxIndicatorHeight : 0

        enabled: (mySpinBox.from < mySpinBox.to) ? mySpinBox.value > mySpinBox.from
                                                 : mySpinBox.value < mySpinBox.from
    }

    contentItem: SpinBoxInput {
        id: spinBoxInput
        myControl: mySpinBox

        function handleEditingFinished() {
            mySpinBox.focus = false

            // Keep the dirty state before calling setValueFromInput(),
            // it will be set to false (cleared) internally
            var valueModified = mySpinBox.dirty

            mySpinBox.setValueFromInput()
            myTimer.stop()

            // Only trigger the signal, if the value was modified
            if (valueModified)
                mySpinBox.compressedValueModified()
        }

        onEditingFinished: spinBoxInput.handleEditingFinished()
    }

    background: Rectangle {
        id: spinBoxBackground
        color: StudioTheme.Values.themeControlOutline
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
        x: actionIndicator.width
        width: mySpinBox.width - actionIndicator.width
        height: mySpinBox.height
    }

    CheckIndicator {
        id: sliderIndicator
        myControl: mySpinBox
        myPopup: sliderPopup
        x: spinBoxInput.x + spinBoxInput.width
        y: StudioTheme.Values.border
        width: sliderIndicator.visible ? mySpinBox.__sliderIndicatorWidth - StudioTheme.Values.border : 0
        height: sliderIndicator.visible ? mySpinBox.__sliderIndicatorHeight - (StudioTheme.Values.border * 2) : 0
        visible: false // reasonable default
    }

    SliderPopup {
        id: sliderPopup
        myControl: mySpinBox
        x: actionIndicator.width + StudioTheme.Values.border
        y: StudioTheme.Values.height
        width: mySpinBox.width - actionIndicator.width - (StudioTheme.Values.border * 2)
        height: StudioTheme.Values.sliderHeight

        enter: Transition {}
        exit: Transition {}
    }

    textFromValue: function (value, locale) {
        locale.numberOptions = Locale.OmitGroupSeparator
        return Number(value / mySpinBox.factor).toLocaleString(locale, 'f',
                                                               mySpinBox.decimals)
    }

    valueFromText: function (text, locale) {
        return Number.fromLocaleString(locale, text) * mySpinBox.factor
    }

    states: [
        State {
            name: "default"
            when: mySpinBox.enabled && !mySpinBox.hover && !mySpinBox.hovered
                  && !mySpinBox.edit && !mySpinBox.drag && !mySpinBox.sliderDrag
            PropertyChanges {
                target: mySpinBox
                __wheelEnabled: false
            }
            PropertyChanges {
                target: spinBoxInput
                selectByMouse: false
            }
            PropertyChanges {
                target: spinBoxBackground
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "edit"
            when: mySpinBox.edit
            PropertyChanges {
                target: mySpinBox
                __wheelEnabled: true
            }
            PropertyChanges {
                target: spinBoxInput
                selectByMouse: true
            }
            PropertyChanges {
                target: spinBoxBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "drag"
            when: mySpinBox.drag || mySpinBox.sliderDrag
            PropertyChanges {
                target: spinBoxBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutlineInteraction
            }
        },
        State {
            name: "disable"
            when: !mySpinBox.enabled
            PropertyChanges {
                target: spinBoxBackground
                color: StudioTheme.Values.themeControlOutlineDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
        }
    ]

    Timer {
        id: myTimer
        repeat: false
        running: false
        interval: 400
        onTriggered: mySpinBox.compressedValueModified()
    }

    onValueModified: myTimer.restart()
    onFocusChanged: mySpinBox.setValueFromInput()
    onDisplayTextChanged: spinBoxInput.text = mySpinBox.displayText
    onActiveFocusChanged: {
        if (mySpinBox.activeFocus) { // QTBUG-75862 && mySpinBox.focusReason === Qt.TabFocusReason)
            mySpinBox.preFocusText = spinBoxInput.text
            spinBoxInput.selectAll()
        }

        if (sliderPopup.opened && !mySpinBox.activeFocus)
            sliderPopup.close()
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            event.accepted = true

            // Store current step size
            var currStepSize = mySpinBox.stepSize

            if (event.modifiers & Qt.ControlModifier)
                mySpinBox.stepSize = mySpinBox.minStepSize

            if (event.modifiers & Qt.ShiftModifier)
                mySpinBox.stepSize = mySpinBox.maxStepSize

            // Check if value is in sync with text input, if not sync it!
            var val = mySpinBox.valueFromText(spinBoxInput.text,
                                              mySpinBox.locale)
            if (mySpinBox.value !== val)
                mySpinBox.value = val

            var currValue = mySpinBox.value

            if (event.key === Qt.Key_Up)
                mySpinBox.increase()
            else
                mySpinBox.decrease()

            if (currValue !== mySpinBox.value)
                mySpinBox.valueModified()

            // Reset step size
            mySpinBox.stepSize = currStepSize
        }

        if (event.key === Qt.Key_Escape) {
            spinBoxInput.text = mySpinBox.preFocusText
            mySpinBox.dirty = true
            spinBoxInput.handleEditingFinished()
        }

        // FIX: This is a temporary fix for QTBUG-74239
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            mySpinBox.setValueFromInput()
    }

    function clamp(v, lo, hi) {
        return (v < lo || v > hi) ? Math.min(Math.max(lo, v), hi) : v
    }

    function setValueFromInput() {
        if (!mySpinBox.dirty)
            return

        // FIX: This is a temporary fix for QTBUG-74239
        var currValue = mySpinBox.value

        if (!spinBoxInput.acceptableInput)
            mySpinBox.value = clamp(valueFromText(spinBoxInput.text,
                                                  mySpinBox.locale),
                                    mySpinBox.validator.bottom * mySpinBox.factor,
                                    mySpinBox.validator.top * mySpinBox.factor)
        else
            mySpinBox.value = valueFromText(spinBoxInput.text,
                                            mySpinBox.locale)

        if (spinBoxInput.text !== mySpinBox.displayText)
            spinBoxInput.text = mySpinBox.displayText

        if (mySpinBox.value !== currValue)
            mySpinBox.valueModified()

        mySpinBox.dirty = false
    }
}
