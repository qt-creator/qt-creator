// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

TextInput {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property T.ComboBox __parentControl

    property bool edit: control.activeFocus
    property bool hover: mouseArea.containsMouse && control.enabled
    property bool elidable: false
    property string suffix: ""

    z: 2
    font: control.__parentControl.font
    color: control.style.text.idle
    selectionColor: control.style.text.selection
    selectedTextColor: control.style.text.selectedText

    horizontalAlignment: Qt.AlignLeft
    verticalAlignment: Qt.AlignVCenter
    leftPadding: control.style.inputHorizontalPadding
    rightPadding: control.style.inputHorizontalPadding

    readOnly: !control.__parentControl.editable
    validator: control.__parentControl.validator
    inputMethodHints: control.__parentControl.inputMethodHints
    selectByMouse: false
    activeFocusOnPress: false
    clip: true
    echoMode: control.elidable ? TextInput.NoEcho : TextInput.Normal

    Text {
        id: elidableText
        anchors.fill: control
        leftPadding: control.leftPadding
        rightPadding: control.rightPadding
        horizontalAlignment: control.horizontalAlignment
        verticalAlignment: control.verticalAlignment
        font: control.font
        color: control.color
        text: control.text + control.suffix
        visible: control.elidable
        elide: Text.ElideRight
    }

    Text {
        id: nonElidableSuffix
        anchors.fill: control
        leftPadding: control.implicitWidth - control.rightPadding
        rightPadding: control.rightPadding
        horizontalAlignment: control.horizontalAlignment
        verticalAlignment: control.verticalAlignment
        font: control.font
        color: control.color
        text: control.suffix
        visible: !control.elidable
    }

    Rectangle {
        id: background
        x: control.style.borderWidth
        y: control.style.borderWidth
        z: -1
        width: control.width
        height: control.style.controlSize.height - control.style.borderWidth * 2
        color: control.style.background.idle
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
            if (!control.__parentControl.editable) {
                if (control.__parentControl.popup.opened) {
                    control.__parentControl.popup.close()
                    control.__parentControl.focus = false
                } else {
                    control.__parentControl.popup.open()
                    //textInput.__control.forceActiveFocus()
                }
            } else {
                control.forceActiveFocus()
            }

            mouse.accepted = false
        }
    }

    states: [
        State {
            name: "default"
            when: control.__parentControl.enabled && !control.edit && !control.hover
                  && !control.__parentControl.hover && !control.__parentControl.open
                  && !control.__parentControl.hasActiveDrag
            PropertyChanges {
                target: background
                color: control.style.background.idle
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton
            }
        },
        State {
            name: "dragHover"
            when: control.__parentControl.enabled
                  && (control.__parentControl.hasActiveHoverDrag ?? false)
            PropertyChanges {
                target: background
                color: control.style.background.interaction
            }
        },
        State {
            name: "globalHover"
            when: control.__parentControl.hover && !control.hover && !control.edit
                  && !control.__parentControl.open
            PropertyChanges {
                target: background
                color: control.style.background.globalHover
            }
        },
        State {
            name: "hover"
            when: control.hover && control.__parentControl.hover && !control.edit
            PropertyChanges {
                target: background
                color: control.style.background.hover
            }
        },
        // This state is intended for ComboBoxes which aren't editable, but have focus e.g. via
        // tab focus. It is therefor possible to use the mouse wheel to scroll through the items.
        State {
            name: "focus"
            when: control.edit && !control.__parentControl.editable
            PropertyChanges {
                target: background
                color: control.style.background.interaction
            }
        },
        State {
            name: "edit"
            when: control.edit && control.__parentControl.editable
            PropertyChanges {
                target: background
                color: control.style.background.interaction
            }
            PropertyChanges {
                target: control
                echoMode: TextInput.Normal
            }
            PropertyChanges {
                target: elidableText
                visible: false
            }
            PropertyChanges {
                target: nonElidableSuffix
                visible: false
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.IBeamCursor
                acceptedButtons: Qt.NoButton
            }
        },
        State {
            name: "popup"
            when: control.__parentControl.open
            PropertyChanges {
                target: background
                color: control.style.background.hover
            }
        },
        State {
            name: "disable"
            when: !control.__parentControl.enabled
            PropertyChanges {
                target: background
                color: control.style.background.disabled
            }
            PropertyChanges {
                target: control
                color: control.style.text.disabled
            }
        }
    ]
}
