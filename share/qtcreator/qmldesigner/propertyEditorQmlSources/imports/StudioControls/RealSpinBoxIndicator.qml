// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: spinBoxIndicator

    property T.Control myControl

    property bool hover: spinBoxIndicatorMouseArea.containsMouse
    property bool pressed: spinBoxIndicatorMouseArea.containsPress
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
        hoverEnabled: true
        pressAndHoldInterval: 500
        onPressed: function(mouse) {
            if (myControl.activeFocus)
                spinBoxIndicator.forceActiveFocus()

            spinBoxIndicator.realPressed()
            mouse.accepted = true
        }
        onPressAndHold: {
            pressAndHoldTimer.restart()
            pressedAndHeld = true
        }
        onReleased: function(mouse) {
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
                when: myControl.enabled && spinBoxIndicator.enabled && !myControl.edit
                      && !spinBoxIndicator.hover && !myControl.hover && !myControl.drag
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeTextColor
                }
            },
            State {
                name: "globalHover"
                when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                      && !spinBoxIndicator.hover && myControl.hover && !myControl.edit
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeTextColor
                }
            },
            State {
                name: "hover"
                when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                      && spinBoxIndicator.hover && myControl.hover && !spinBoxIndicator.pressed
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeIconColorHover
                }
            },
            State {
                name: "press"
                when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                      && spinBoxIndicator.pressed
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeIconColor
                }
            },
            State {
                name: "edit"
                when: myControl.edit && spinBoxIndicator.enabled
                PropertyChanges {
                    target: spinBoxIndicatorIcon
                    color: StudioTheme.Values.themeTextColor
                }
            },
            State {
                name: "disable"
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
            when: myControl.enabled && !myControl.edit
                  && !spinBoxIndicator.hover && !myControl.hover && !myControl.drag
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: false
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "globalHover"
            when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                  && !spinBoxIndicator.hover && myControl.hover && !myControl.edit
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
            }
        },
        State {
            name: "hover"
            when: myControl.enabled && !myControl.drag && spinBoxIndicator.enabled
                  && spinBoxIndicator.hover && myControl.hover && !spinBoxIndicator.pressed
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        },
        State {
            name: "press"
            when: myControl.enabled && spinBoxIndicator.enabled && !myControl.drag
                  && spinBoxIndicator.pressed
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "edit"
            when: myControl.edit && myControl.enabled && spinBoxIndicator.enabled
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "drag"
            when: myControl.drag && myControl.enabled
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: false
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }
        },
        State {
            name: "disable"
            when: !myControl.enabled
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: false
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackgroundDisabled
            }
        },
        State {
            name: "limit"
            when: !spinBoxIndicator.enabled && !spinBoxIndicator.realEnabled && myControl.hover
            PropertyChanges {
                target: spinBoxIndicatorIcon
                visible: true
            }
            PropertyChanges {
                target: spinBoxIndicator
                color: StudioTheme.Values.themeControlBackground
            }
        }
    ]
}
