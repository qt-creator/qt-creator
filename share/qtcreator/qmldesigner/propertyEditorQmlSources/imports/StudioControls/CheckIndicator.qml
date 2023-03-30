// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property T.Control __parentControl
    property T.Popup __parentPopup

    property bool hover: mouseArea.containsMouse && control.enabled
    property bool pressed: mouseArea.containsPress
    property bool checked: false

    property bool hasActiveDrag: control.__parentControl.hasActiveDrag ?? false
    property bool hasActiveHoverDrag: control.__parentControl.hasActiveHoverDrag ?? false

    color: control.style.background.idle
    border.width: 0

    Connections {
        target: control.__parentPopup
        function onClosed() { control.checked = false }
        function onOpened() { control.checked = true }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if (control.__parentControl.activeFocus)
                control.__parentControl.focus = false

            if (control.__parentPopup.opened) {
                control.__parentPopup.close()
            } else {
                control.__parentPopup.open()
                control.__parentPopup.forceActiveFocus()
            }
        }
    }

    T.Label {
        id: icon
        anchors.fill: parent
        color: control.style.icon.idle
        text: StudioTheme.Constants.upDownSquare2
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: control.style.baseIconFontSize
        font.family: StudioTheme.Constants.iconFont.family
    }

    states: [
        State {
            name: "default"
            when: control.__parentControl.enabled && control.enabled
                  && !control.__parentControl.edit && !control.hover
                  && !control.__parentControl.hover && !control.__parentControl.drag
                  && !control.checked && !control.hasActiveDrag
            PropertyChanges {
                target: icon
                color: control.style.icon.idle
            }
            PropertyChanges {
                target: control
                color: control.style.background.idle
            }
        },
        State {
            name: "dragHover"
            when: control.__parentControl.enabled && control.hasActiveHoverDrag
            PropertyChanges {
                target: control
                color: control.style.background.interaction
            }
        },
        State {
            name: "globalHover"
            when: control.__parentControl.enabled && control.enabled
                  && !control.__parentControl.drag && !control.hover
                  && control.__parentControl.hover && !control.__parentControl.edit
                  && !control.checked
            PropertyChanges {
                target: control
                color: control.style.background.globalHover
            }
        },
        State {
            name: "hover"
            when: control.__parentControl.enabled && control.enabled
                  && !control.__parentControl.drag && control.hover && control.__parentControl.hover
                  && !control.pressed && !control.checked
            PropertyChanges {
                target: icon
                color: control.style.icon.hover
            }
            PropertyChanges {
                target: control
                color: control.style.background.hover
            }
        },
        State {
            name: "check"
            when: control.checked
            PropertyChanges {
                target: icon
                color: control.style.icon.interaction
            }
            PropertyChanges {
                target: control
                color: control.style.interaction
            }
        },
        State {
            name: "edit"
            when: control.__parentControl.edit && !control.checked
                  && !(control.hover && control.__parentControl.hover)
            PropertyChanges {
                target: icon
                color: control.style.icon.idle
            }
            PropertyChanges {
                target: control
                color: control.style.background.idle
            }
        },
        State {
            name: "press"
            when: control.__parentControl.enabled && control.enabled
                  && !control.__parentControl.drag && control.pressed
            PropertyChanges {
                target: icon
                color: control.style.icon.interaction
            }
            PropertyChanges {
                target: control
                color: control.style.interaction
            }
        },
        State {
            name: "drag"
            when: (control.__parentControl.drag !== undefined && control.__parentControl.drag)
                  && !control.checked && !(control.hover && control.__parentControl.hover)
            PropertyChanges {
                target: control
                color: control.style.background.idle
            }
        },
        State {
            name: "disable"
            when: !control.__parentControl.enabled
            PropertyChanges {
                target: control
                color: control.style.background.disabled
            }
            PropertyChanges {
                target: icon
                color: control.style.icon.disabled
            }
        }
    ]
}
