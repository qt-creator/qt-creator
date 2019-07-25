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

    property alias labelColor: spinBoxInput.color
    property alias actionIndicator: actionIndicator

    property int decimals: 0
    property int factor: Math.pow(10, decimals)

    property real minStepSize: 1
    property real maxStepSize: 10

    property bool edit: spinBoxInput.activeFocus
    property bool hover: false // This property is used to indicate the global hover state
    property bool drag: false

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.squareComponentWidth
    property real __actionIndicatorHeight: StudioTheme.Values.height

    property bool spinBoxIndicatorVisible: true
    property real __spinBoxIndicatorWidth: StudioTheme.Values.smallRectWidth - (2
                                           * StudioTheme.Values.border)
    property real __spinBoxIndicatorHeight: (StudioTheme.Values.height / 2)
                                            - StudioTheme.Values.border

    property alias sliderIndicatorVisible: sliderIndicator.visible
    property real __sliderIndicatorWidth: StudioTheme.Values.squareComponentWidth
    property real __sliderIndicatorHeight: StudioTheme.Values.height

    signal compressedValueModified

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
        width: actionIndicator.visible ? __actionIndicatorWidth : 0
        height: actionIndicator.visible ? __actionIndicatorHeight : 0
    }

    up.indicator: SpinBoxIndicator {
        id: spinBoxIndicatorUp
        myControl: mySpinBox

        visible: spinBoxIndicatorVisible
        //hover: mySpinBox.up.hovered // TODO QTBUG-74688
        pressed: mySpinBox.up.pressed
        iconFlip: -1

        x: actionIndicator.width + (actionIndicatorVisible ? 0 : StudioTheme.Values.border)
        y: StudioTheme.Values.border
        width: spinBoxIndicatorVisible ? __spinBoxIndicatorWidth : 0
        height: spinBoxIndicatorVisible ? __spinBoxIndicatorHeight : 0

        enabled: (mySpinBox.from < mySpinBox.to) ? mySpinBox.value < mySpinBox.to : mySpinBox.value > mySpinBox.to
    }

    down.indicator: SpinBoxIndicator {
        id: spinBoxIndicatorDown
        myControl: mySpinBox

        visible: spinBoxIndicatorVisible
        //hover: mySpinBox.down.hovered // TODO QTBUG-74688
        pressed: mySpinBox.down.pressed

        x: actionIndicator.width + (actionIndicatorVisible ? 0 : StudioTheme.Values.border)
        y: spinBoxIndicatorUp.y + spinBoxIndicatorUp.height
        width: spinBoxIndicatorVisible ? __spinBoxIndicatorWidth : 0
        height: spinBoxIndicatorVisible ? __spinBoxIndicatorHeight : 0

        enabled: (mySpinBox.from < mySpinBox.to) ? mySpinBox.value > mySpinBox.from : mySpinBox.value < mySpinBox.from
    }

    contentItem: SpinBoxInput {
        id: spinBoxInput
        myControl: mySpinBox
    }

    background: Rectangle {
        id: spinBoxBackground
        color: StudioTheme.Values.themeControlOutline
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
        x: actionIndicator.width - (actionIndicatorVisible ? StudioTheme.Values.border : 0)
        width: mySpinBox.width - actionIndicator.width
        height: mySpinBox.height
    }

    CheckIndicator {
        id: sliderIndicator
        myControl: mySpinBox
        myPopup: sliderPopup

        x: spinBoxInput.x + spinBoxInput.width - StudioTheme.Values.border
        width: sliderIndicator.visible ? __sliderIndicatorWidth : 0
        height: sliderIndicator.visible ? __sliderIndicatorHeight : 0
        visible: false // reasonable default
    }

    SliderPopup {
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
        return Number(value / mySpinBox.factor).toLocaleString(locale, 'f',
                                                               mySpinBox.decimals)
    }

    valueFromText: function (text, locale) {
        return Number.fromLocaleString(locale, text) * mySpinBox.factor
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
        interval: 100
        onTriggered: mySpinBox.compressedValueModified()
    }

    onValueModified: myTimer.restart()
    onFocusChanged: mySpinBox.setValueFromInput()
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

        if (event.key === Qt.Key_Escape)
            mySpinBox.focus = false

        // FIX: This is a temporary fix for QTBUG-74239
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            mySpinBox.setValueFromInput()
    }

    function clamp(v, lo, hi) {
        if (v < lo || v > hi)
            return Math.min(Math.max(lo, v), hi)

        return v
    }

    function setValueFromInput() {
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
    }
}
