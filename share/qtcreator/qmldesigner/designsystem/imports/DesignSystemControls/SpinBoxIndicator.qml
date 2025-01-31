// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme as StudioTheme

Rectangle {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property bool hover: spinBoxIndicatorMouseArea.containsMouse
    property bool pressed: spinBoxIndicatorMouseArea.containsPress
    property bool released: false
    property bool realEnabled: true

    property bool parentHover: false
    property bool parentEdit: false

    signal realPressed
    signal realPressAndHold
    signal realReleased

    property alias iconFlip: spinBoxIndicatorIconScale.yScale


    color: control.style.background.idle
    border.width: 0

    onEnabledChanged: control.syncEnabled()
    onRealEnabledChanged: {
        control.syncEnabled()
        if (control.realEnabled === false) {
            pressAndHoldTimer.stop()
            spinBoxIndicatorMouseArea.pressedAndHeld = false
        }
    }

    // This function is meant to synchronize enabled with realEnabled to avoid
    // the internal logic messing with the actual state.
    function syncEnabled() {
        control.enabled = control.realEnabled
    }

    Timer {
        id: pressAndHoldTimer
        repeat: true
        running: false
        interval: 100
        onTriggered: control.realPressAndHold()
    }

    // This MouseArea is a workaround to avoid some hover state related bugs
    // when using the actual signal 'up.hovered'. QTBUG-74688
    MouseArea {
        id: spinBoxIndicatorMouseArea

        property bool pressedAndHeld: false

        anchors.fill: parent
        hoverEnabled: true
        pressAndHoldInterval: 500
        onPressed: function(mouse) {
            //if (control.__parentControl.activeFocus)
            //    control.forceActiveFocus()

            control.realPressed()
            mouse.accepted = true
        }
        onPressAndHold: {
            pressAndHoldTimer.restart()
            spinBoxIndicatorMouseArea.pressedAndHeld = true
        }
        onReleased: function(mouse) {
            // Only trigger real released when pressAndHold isn't active
            if (!pressAndHoldTimer.running && containsMouse)
                control.realReleased()
            pressAndHoldTimer.stop()
            mouse.accepted = true
            spinBoxIndicatorMouseArea.pressedAndHeld = false
        }
        onEntered: {
            if (spinBoxIndicatorMouseArea.pressedAndHeld)
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
        color: control.style.icon.idle
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: control.style.smallIconFontSize
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
                when: control.enabled && !control.hover && !control.parentEdit && !control.parentHover
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: control.style.icon.idle
                }
            },
            State {
                name: "globalHover"
                when: control.enabled && !control.hover && !control.parentEdit && control.parentHover
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: control.style.icon.idle
                }
            },
            State {
                name: "hover"
                when: control.enabled && control.hover && !control.pressed && control.parentHover
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: control.style.icon.hover
                }
            },
            State {
                name: "press"
                when: control.enabled && control.pressed
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: control.style.icon.idle
                }
            },
            State {
                name: "parentEdit"
                when: control.parentEdit && control.enabled
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: control.style.icon.idle
                }
            },
            State {
                name: "disable"
                when: !control.enabled
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: control.style.icon.disabled
                }
            }
        ]
    }

    states: [
        State {
            name: "default"
            when: !control.parentEdit && !control.hover && !control.parentHover
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: false
            }
            PropertyChanges {
                target: control
                color: control.style.background.idle
            }
        },
        State {
            name: "globalHover"
            when: control.enabled && !control.hover && !control.parentEdit && control.parentHover
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: control
                color: control.style.background.globalHover
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hover && !control.pressed && control.parentHover
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: control
                color: control.style.background.hover
            }
        },
        State {
            name: "press"
            when: control.enabled && control.pressed
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: control
                color: control.style.interaction
            }
        },
        State {
            name: "parentEdit"
            when: control.parentEdit && control.enabled
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: control
                color: control.style.background.idle
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: false
            }
            PropertyChanges {
                target: control
                color: control.style.background.disabled
            }
        },
        State {
            name: "limit"
            when: !control.enabled && !control.realEnabled && control.parentHover
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: control
                color: control.style.background.idle
            }
        }
    ]
}
