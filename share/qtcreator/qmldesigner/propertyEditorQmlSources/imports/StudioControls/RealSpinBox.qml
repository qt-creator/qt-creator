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

T.SpinBox {
    id: mySpinBox

    property real realFrom: 0.0
    property real realTo: 99.0
    property real realValue: 1.0
    property real realStepSize: 1.0

    property alias labelColor: spinBoxInput.color
    property alias actionIndicator: actionIndicator

    property int decimals: 0

    property real minStepSize: {
        var tmpMinStepSize = Number((mySpinBox.realStepSize * 0.1).toFixed(mySpinBox.decimals))
        return (tmpMinStepSize) ? tmpMinStepSize : mySpinBox.realStepSize
    }
    property real maxStepSize: {
        var tmpMaxStepSize = Number((mySpinBox.realStepSize * 10.0).toFixed(mySpinBox.decimals))
        return (tmpMaxStepSize < mySpinBox.realTo) ? tmpMaxStepSize : mySpinBox.realStepSize
    }

    property bool edit: spinBoxInput.activeFocus
    property bool hover: false // This property is used to indicate the global hover state
    property bool drag: false

    property bool dirty: false // user modification flag

    property real realDragRange: mySpinBox.realTo - mySpinBox.realFrom

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.squareComponentWidth
    property real __actionIndicatorHeight: StudioTheme.Values.height

    property bool spinBoxIndicatorVisible: true
    property real __spinBoxIndicatorWidth: StudioTheme.Values.smallRectWidth - 2
                                           * StudioTheme.Values.border
    property real __spinBoxIndicatorHeight: StudioTheme.Values.height / 2
                                            - StudioTheme.Values.border

    property alias sliderIndicatorVisible: sliderIndicator.visible
    property real __sliderIndicatorWidth: StudioTheme.Values.squareComponentWidth
    property real __sliderIndicatorHeight: StudioTheme.Values.height

    property alias compressedValueTimer: myTimer

    signal realValueModified
    signal compressedRealValueModified
    signal dragStarted
    signal dragEnded

    // Use custom wheel handling due to bugs
    property bool __wheelEnabled: false
    wheelEnabled: false

    width: StudioTheme.Values.squareComponentWidth * 5
    height: StudioTheme.Values.height

    leftPadding: spinBoxIndicatorDown.x + spinBoxIndicatorDown.width
                 - (spinBoxIndicatorVisible ? 0 : StudioTheme.Values.border)
    rightPadding: sliderIndicator.width - (sliderIndicatorVisible ? StudioTheme.Values.border : 0)

    font.pixelSize: StudioTheme.Values.myFontSize
    editable: true

    // Leave this in for now
    from: -99
    value: 0
    to: 99

    validator: DoubleValidator {
        id: doubleValidator
        locale: mySpinBox.locale.name
        notation: DoubleValidator.StandardNotation
        decimals: mySpinBox.decimals
        bottom: Math.min(mySpinBox.realFrom, mySpinBox.realTo)
        top: Math.max(mySpinBox.realFrom, mySpinBox.realTo)
    }

    ActionIndicator {
        id: actionIndicator
        myControl: mySpinBox
        x: 0
        y: 0
        width: actionIndicator.visible ? __actionIndicatorWidth : 0
        height: actionIndicator.visible ? __actionIndicatorHeight : 0
    }

    up.indicator: RealSpinBoxIndicator {
        id: spinBoxIndicatorUp
        myControl: mySpinBox
        iconFlip: -1
        visible: mySpinBox.spinBoxIndicatorVisible
        onRealReleased: mySpinBox.realIncrease()
        onRealPressAndHold: mySpinBox.realIncrease()
        x: actionIndicator.width + (mySpinBox.actionIndicatorVisible ? 0 : StudioTheme.Values.border)
        y: StudioTheme.Values.border
        width: mySpinBox.spinBoxIndicatorVisible ? mySpinBox.__spinBoxIndicatorWidth : 0
        height: mySpinBox.spinBoxIndicatorVisible ? mySpinBox.__spinBoxIndicatorHeight : 0

        realEnabled: (mySpinBox.realFrom < mySpinBox.realTo) ? (mySpinBox.realValue < mySpinBox.realTo) : (mySpinBox.realValue > mySpinBox.realTo)
    }

    down.indicator: RealSpinBoxIndicator {
        id: spinBoxIndicatorDown
        myControl: mySpinBox
        visible: mySpinBox.spinBoxIndicatorVisible
        onRealReleased: mySpinBox.realDecrease()
        onRealPressAndHold: mySpinBox.realDecrease()
        x: actionIndicator.width + (mySpinBox.actionIndicatorVisible ? 0 : StudioTheme.Values.border)
        y: spinBoxIndicatorUp.y + spinBoxIndicatorUp.height
        width: mySpinBox.spinBoxIndicatorVisible ? mySpinBox.__spinBoxIndicatorWidth : 0
        height: mySpinBox.spinBoxIndicatorVisible ? mySpinBox.__spinBoxIndicatorHeight : 0

        realEnabled: (mySpinBox.realFrom < mySpinBox.realTo) ? (mySpinBox.realValue > mySpinBox.realFrom) : (mySpinBox.realValue < mySpinBox.realFrom)
    }

    contentItem: RealSpinBoxInput {
        id: spinBoxInput
        myControl: mySpinBox
        validator: doubleValidator

        function handleEditingFinished() {
            // Keep the dirty state before calling setValueFromInput(),
            // it will be set to false (cleared) internally
            var valueModified = mySpinBox.dirty

            mySpinBox.setValueFromInput()
            myTimer.stop()

            // Only trigger the signal, if the value was modified
            if (valueModified)
                mySpinBox.compressedRealValueModified()
        }

        onEditingFinished: handleEditingFinished()
        onTextEdited: mySpinBox.dirty = true
    }

    background: Rectangle {
        id: spinBoxBackground
        color: StudioTheme.Values.themeControlOutline
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
        x: actionIndicator.width - (mySpinBox.actionIndicatorVisible ? StudioTheme.Values.border : 0)
        width: mySpinBox.width - actionIndicator.width
        height: mySpinBox.height
    }

    CheckIndicator {
        id: sliderIndicator
        myControl: mySpinBox
        myPopup: sliderPopup
        x: spinBoxInput.x + spinBoxInput.width - StudioTheme.Values.border
        width: sliderIndicator.visible ? mySpinBox.__sliderIndicatorWidth : 0
        height: sliderIndicator.visible ? mySpinBox.__sliderIndicatorHeight : 0
        visible: false // reasonable default
    }

    RealSliderPopup {
        id: sliderPopup
        myControl: mySpinBox
        x: spinBoxInput.x
        y: StudioTheme.Values.height - StudioTheme.Values.border
        width: spinBoxInput.width + sliderIndicator.width - StudioTheme.Values.border
        height: StudioTheme.Values.sliderHeight

        enter: Transition {
        }
        exit: Transition {
        }
    }

    textFromValue: function (value, locale) {
        return Number(mySpinBox.realValue).toLocaleString(locale, 'f', mySpinBox.decimals)
    }

    valueFromText: function (text, locale) {
        mySpinBox.setRealValue(Number.fromLocaleString(locale, spinBoxInput.text))
        return 0
    }

    states: [
        State {
            name: "default"
            when: mySpinBox.enabled && !mySpinBox.hover
                  && !mySpinBox.edit && !mySpinBox.drag
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
                color: StudioTheme.Values.themeControlOutline
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
                color: StudioTheme.Values.themeInteraction
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "drag"
            when: mySpinBox.drag
            PropertyChanges {
                target: spinBoxBackground
                color: StudioTheme.Values.themeInteraction
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "disabled"
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
        interval: 200
        onTriggered: mySpinBox.compressedRealValueModified()
    }

    onRealValueChanged: {
        spinBoxInput.text = mySpinBox.textFromValue(mySpinBox.realValue, mySpinBox.locale)
        mySpinBox.value = 0 // Without setting value back to 0, it can happen that one of
                            // the indicator will be disabled due to range logic.
    }
    onRealValueModified: myTimer.restart()
    onFocusChanged: {
        if (mySpinBox.focus) {
            mySpinBox.dirty = false
        } else {
            // Make sure displayed value is correct after focus loss, as onEditingFinished
            // doesn't trigger when value is something validator doesn't accept.
            spinBoxInput.text = mySpinBox.textFromValue(mySpinBox.realValue, mySpinBox.locale)

            if (mySpinBox.dirty)
                spinBoxInput.handleEditingFinished()
        }
    }
    onDisplayTextChanged: spinBoxInput.text = mySpinBox.displayText
    onActiveFocusChanged: {
        if (mySpinBox.activeFocus)
            // QTBUG-75862 && mySpinBox.focusReason === Qt.TabFocusReason)
            spinBoxInput.selectAll()

        if (sliderPopup.opened && !mySpinBox.activeFocus)
            sliderPopup.close()
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            event.accepted = true

            // Store current step size
            var currStepSize = mySpinBox.realStepSize

            // Set realStepSize according to used modifier key
            if (event.modifiers & Qt.ControlModifier)
                mySpinBox.realStepSize = mySpinBox.minStepSize

            if (event.modifiers & Qt.ShiftModifier)
                mySpinBox.realStepSize = mySpinBox.maxStepSize

            if (event.key === Qt.Key_Up)
                mySpinBox.realIncrease()
            else
                mySpinBox.realDecrease()

            // Reset realStepSize
            mySpinBox.realStepSize = currStepSize
        }

        if (event.key === Qt.Key_Escape)
            mySpinBox.focus = false
    }

    function clamp(v, lo, hi) {
        return (v < lo || v > hi) ? Math.min(Math.max(lo, v), hi) : v
    }

    function setValueFromInput() {

        if (!mySpinBox.dirty)
            return

        // FIX: This is a temporary fix for QTBUG-74239
        var currValue = mySpinBox.realValue

        // Call the function but don't use return value. The realValue property
        // will be implicitly set inside the function/procedure.
        mySpinBox.valueFromText(spinBoxInput.text, mySpinBox.locale)

        if (mySpinBox.realValue !== currValue) {
            mySpinBox.realValueModified()
        } else {
            // Check if input text differs in format from the current value
            var tmpInputValue = mySpinBox.textFromValue(mySpinBox.realValue, mySpinBox.locale)

            if (tmpInputValue !== spinBoxInput.text)
                spinBoxInput.text = tmpInputValue
        }

        mySpinBox.dirty = false
    }

    function setRealValue(value) {
        if (Number.isNaN(value))
            value = 0

        if (mySpinBox.decimals === 0)
            value = Math.round(value)

        mySpinBox.realValue = mySpinBox.clamp(value,
                                              mySpinBox.validator.bottom,
                                              mySpinBox.validator.top)
    }

    function realDecrease() {
        // Store the current value for comparison
        var currValue = mySpinBox.realValue
        mySpinBox.valueFromText(spinBoxInput.text, mySpinBox.locale)

        mySpinBox.setRealValue(mySpinBox.realValue - mySpinBox.realStepSize)

        if (mySpinBox.realValue !== currValue)
            mySpinBox.realValueModified()
    }

    function realIncrease() {
        // Store the current value for comparison
        var currValue = mySpinBox.realValue
        mySpinBox.valueFromText(spinBoxInput.text, mySpinBox.locale)

        mySpinBox.setRealValue(mySpinBox.realValue + mySpinBox.realStepSize)

        if (mySpinBox.realValue !== currValue)
            mySpinBox.realValueModified()
    }
}
