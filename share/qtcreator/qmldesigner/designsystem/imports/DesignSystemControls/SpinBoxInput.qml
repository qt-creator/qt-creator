// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme as StudioTheme

TextInput {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property bool edit: control.activeFocus
    property bool drag: false
    property bool hover: hoverHandler.hovered && control.enabled

    z: 2
    color: control.style.text.idle
    selectionColor: control.style.text.selection
    selectedTextColor: control.style.text.selectedText

    horizontalAlignment: Qt.AlignLeft
    verticalAlignment: Qt.AlignVCenter
    leftPadding: control.style.inputHorizontalPadding
    rightPadding: control.style.inputHorizontalPadding

    selectByMouse: false
    activeFocusOnPress: false
    clip: true

    // TextInput focus needs to be set to activeFocus whenever it changes,
    // otherwise TextInput will get activeFocus whenever the parent SpinBox gets
    // activeFocus. This will lead to weird side effects.
    onActiveFocusChanged: control.focus = control.activeFocus

    HoverHandler { id: hoverHandler }

    Rectangle {
        id: textInputBackground
        x: 0
        y: control.style.borderWidth
        z: -1
        width: control.width
        height: control.height - (control.style.borderWidth * 2)
        color: control.style.background.idle
        border.width: 0
    }

    // Ensure that we get Up and Down key press events first
    Keys.onShortcutOverride: function(event) {
        event.accepted = (event.key === Qt.Key_Up || event.key === Qt.Key_Down)
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.edit && !control.hover
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.idle
            }
        },
        State {
            name: "globalHover"
            when: !control.hover && !control.edit
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.globalHover
            }
        },
        State {
            name: "hover"
            when: control.hover && !control.edit
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.hover
            }
        },
        State {
            name: "edit"
            when: control.edit
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.interaction
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: textInputBackground
                color: control.style.background.disabled
            }
            PropertyChanges {
                target: control
                color: control.style.text.disabled
            }
        }
    ]
}
