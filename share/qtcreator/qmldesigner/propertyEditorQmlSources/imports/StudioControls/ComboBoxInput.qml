// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

TextInput {
    id: textInput

    property T.Control myControl

    property bool edit: textInput.activeFocus
    property bool hover: mouseArea.containsMouse && textInput.enabled

    z: 2
    font: myControl.font
    color: StudioTheme.Values.themeTextColor
    selectionColor: StudioTheme.Values.themeTextSelectionColor
    selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor

    horizontalAlignment: Qt.AlignLeft
    verticalAlignment: Qt.AlignVCenter
    leftPadding: StudioTheme.Values.inputHorizontalPadding
    rightPadding: StudioTheme.Values.inputHorizontalPadding

    readOnly: !myControl.editable
    validator: myControl.validator
    inputMethodHints: myControl.inputMethodHints
    selectByMouse: false
    activeFocusOnPress: false
    clip: true

    Rectangle {
        id: textInputBackground
        x: StudioTheme.Values.border
        y: StudioTheme.Values.border
        z: -1
        width: textInput.width
        height: StudioTheme.Values.height - StudioTheme.Values.border * 2
        color: StudioTheme.Values.themeControlBackground
        border.width: 0
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: true
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.PointingHandCursor
        onPressed: function(mouse) {
            if (!textInput.myControl.editable) {
                if (myControl.popup.opened) {
                    myControl.popup.close()
                    myControl.focus = false
                } else {
                    myControl.popup.open()
                    //myControl.forceActiveFocus()
                }
            } else {
                textInput.forceActiveFocus()
            }

            mouse.accepted = false
        }
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !textInput.edit && !textInput.hover && !myControl.hover
                  && !myControl.open && !myControl.hasActiveDrag
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackground
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton
            }
        },
        State {
            name: "dragHover"
            when: myControl.enabled && myControl.hasActiveHoverDrag
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }
        },
        State {
            name: "globalHover"
            when: myControl.hover && !textInput.hover && !textInput.edit && !myControl.open
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
            }
        },
        State {
            name: "hover"
            when: textInput.hover && myControl.hover && !textInput.edit
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        },
        // This state is intended for ComboBoxes which aren't editable, but have focus e.g. via
        // tab focus. It is therefor possible to use the mouse wheel to scroll through the items.
        State {
            name: "focus"
            when: textInput.edit && !myControl.editable
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }
        },
        State {
            name: "edit"
            when: textInput.edit && myControl.editable
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.IBeamCursor
                acceptedButtons: Qt.NoButton
            }
        },
        State {
            name: "popup"
            when: myControl.open
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        },
        State {
            name: "disable"
            when: !myControl.enabled
            PropertyChanges {
                target: textInputBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
            }
            PropertyChanges {
                target: textInput
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
