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

Rectangle {
    id: spinBoxIndicator

    property T.Control myControl

    property bool hover: false
    property bool pressed: false
    property bool released: false
    property bool realEnabled: true

    signal realPressed
    signal realPressAndHold
    signal realReleased

    property alias iconFlip: spinBoxIndicatorIconScale.yScale

    color: StudioTheme.Values.themeControlBackground
    border.width: 0

    onEnabledChanged: syncEnabled()
    onRealEnabledChanged: {
        syncEnabled()
        if (spinBoxIndicator.realEnabled === false) {
            pressAndHoldTimer.stop()
            spinBoxIndicatorMouseArea.pressedAndHeld = false
        }
    }

    // This function is meant to synchronize enabled with realEnabled to avoid
    // the internal logic messing with the actual state.
    function syncEnabled() {
        spinBoxIndicator.enabled = spinBoxIndicator.realEnabled
    }

    Timer {
        id: pressAndHoldTimer
        repeat: true
        running: false
        interval: 100
        onTriggered: spinBoxIndicator.realPressAndHold()
    }

    // This MouseArea is a workaround to avoid some hover state related bugs
    // when using the actual signal 'up.hovered'. QTBUG-74688
    MouseArea {
        id: spinBoxIndicatorMouseArea

        property bool pressedAndHeld: false

        anchors.fill: parent
        // Shift the MouseArea down by 1 pixel due to potentially overlapping areas
        anchors.topMargin: iconFlip < 0 ? 0 : 1
        anchors.bottomMargin: iconFlip < 0 ? 1 : 0
        hoverEnabled: true
        pressAndHoldInterval: 500
        onContainsMouseChanged: spinBoxIndicator.hover = containsMouse
        onContainsPressChanged: spinBoxIndicator.pressed = containsPress
        onPressed: {
            myControl.forceActiveFocus()
            spinBoxIndicator.realPressed()
            mouse.accepted = true
        }
        onPressAndHold: {
            pressAndHoldTimer.restart()
            pressedAndHeld = true
        }
        onReleased: {
            // Only trigger real released when pressAndHold isn't active
            if (!pressAndHoldTimer.running && containsMouse)
                spinBoxIndicator.realReleased()
            pressAndHoldTimer.stop()
            mouse.accepted = true
            pressedAndHeld = false
        }
        onEntered: {
            if (pressedAndHeld)
                pressAndHoldTimer.restart()
        }
        onExited: {
            if (pressAndHoldTimer.running)
                pressAndHoldTimer.stop()
        }
    }

    T.Label {
        id: spinBoxIndicatorIcon
        text: StudioTheme.Constants.upDownSquare2
        color: StudioTheme.Values.themeTextColor
        renderType: Text.NativeRendering
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: StudioTheme.Values.spinControlIconSizeMulti
        font.family: StudioTheme.Constants.iconFont.family
        anchors.fill: parent
        transform: Scale {
            id: spinBoxIndicatorIconScale
            origin.x: 0
            origin.y: spinBoxIndicatorIcon.height / 2
            yScale: 1
        }

        states: [
            State {
                name: "default"
                when: myControl.enabled && spinBoxIndicator.enabled
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeTextColor
                }
            },
            State {
                name: "disabled"
                when: !myControl.enabled || !spinBoxIndicator.enabled
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeTextColorDisabled
                }
            }
        ]
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !(spinBoxIndicator.hover
                                         || myControl.hover)
                  && !spinBoxIndicator.pressed && !myControl.edit
                  && !myControl.drag
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "hovered"
            when: (spinBoxIndicator.hover || myControl.hover)
                  && !spinBoxIndicator.pressed && !myControl.edit
                  && !myControl.drag
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeHoverHighlight
            }
        },
        State {
            name: "pressed"
            when: spinBoxIndicator.pressed
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "edit"
            when: myControl.edit
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeFocusEdit
            }
        },
        State {
            name: "drag"
            when: myControl.drag
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeFocusDrag
            }
        },
        State {
            name: "disabled"
            when: !myControl.enabled
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackgroundDisabled
            }
        }
    ]
}
