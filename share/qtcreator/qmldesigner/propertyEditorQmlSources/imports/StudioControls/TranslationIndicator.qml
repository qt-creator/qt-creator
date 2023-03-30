// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Item {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property Item __parentControl

    property bool hover: mouseArea.containsMouse && control.enabled
    property bool pressed: mouseArea.pressed
    property bool checked: false

    signal clicked

    Rectangle {
        id: background
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth

        anchors.centerIn: parent

        width: background.matchParity(control.height, control.style.smallControlSize.width)
        height: background.matchParity(control.height, control.style.smallControlSize.height)

        function matchParity(root, value) {
            var v = Math.round(value)

            if (root % 2 == 0)
                return (v % 2 == 0) ? v : v - 1
            else
                return (v % 2 == 0) ? v - 1 : v
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onPressed: function(mouse) { mouse.accepted = true }
            onClicked: {
                control.checked = !control.checked
                control.clicked()
            }
        }
    }

    T.Label {
        id: icon
        text: "tr"
        color: control.style.icon.idle
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: control.style.baseIconFontSize
        font.italic: true
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.fill: parent

        states: [
            State {
                name: "default"
                when: control.enabled && !control.pressed && !control.checked
                PropertyChanges {
                    target: icon
                    color: control.style.icon.idle
                }
            },
            State {
                name: "press"
                when: control.enabled && control.pressed
                PropertyChanges {
                    target: icon
                    color: control.style.icon.interaction
                }
            },
            State {
                name: "check"
                when: control.enabled && !control.pressed
                      && control.checked
                PropertyChanges {
                    target: icon
                    color: control.style.icon.selected
                }
            },
            State {
                name: "disable"
                when: !control.__parentControl.enabled
                PropertyChanges {
                    target: icon
                    color: control.style.icon.disabled
                }
            }
        ]
    }

    states: [
        State {
            name: "default"
            when: control.__parentControl.enabled && !control.hover && !control.pressed
                  && !control.__parentControl.hover && !control.__parentControl.edit
                  && !control.checked
            PropertyChanges {
                target: background
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
        },
        State {
            name: "globalHover"
            when: control.__parentControl.hover && !control.hover
            PropertyChanges {
                target: background
                color: control.style.background.globalHover
                border.color: control.style.border.idle
            }
        },
        State {
            name: "hover"
            when: control.hover && !control.pressed
            PropertyChanges {
                target: background
                color: control.style.background.hover
                border.color: control.style.border.idle
            }
        },
        State {
            name: "press"
            when: control.hover && control.pressed
            PropertyChanges {
                target: background
                color: control.style.background.interaction
                border.color: control.style.border.interaction
            }
        },
        State {
            name: "disable"
            when: !control.__parentControl.enabled
            PropertyChanges {
                target: background
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
        }
    ]
}
