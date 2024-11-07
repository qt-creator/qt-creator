// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import QtQuick.Templates as T

import StudioControls as StudioControls
import StudioTheme as StudioTheme

T.AbstractButton {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias model: listView.model

    width: control.style.squareControlSize.width + buttonLabel.implicitWidth
    height: control.style.squareControlSize.height

    checked: dropdownPopup.visible

    onPressed: control.checked ? dropdownPopup.close() : dropdownPopup.open()

    contentItem: Row {
        spacing: 0

        Text {
            id: buttonLabel
            height: control.height
            leftPadding: 8
            font.pixelSize: control.style.baseFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: control.text
            visible: control.text !== ""
        }

        Text {
            id: buttonIcon
            width: control.style.squareControlSize.width
            height: control.height
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: control.style.baseIconFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: StudioTheme.Constants.upDownSquare2
        }
    }

    background: Rectangle {
        id: controlBackground
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth
        radius: control.style.radius
    }

    T.Popup {
        id: dropdownPopup
        parent: control
        x: control.width - dropdownPopup.width
        y: control.height
        width: 120
        height: 200
        padding: control.style.borderWidth
        margins: 0 // If not defined margin will be -1
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent | T.Popup.CloseOnReleaseOutsideParent

        contentItem: ListView {
            id: listView
            clip: true
            implicitHeight: listView.contentHeight
            boundsBehavior: Flickable.StopAtBounds

            HoverHandler { id: hoverHandler }

            ScrollBar.vertical: StudioControls.TransientScrollBar {
                id: verticalScrollBar
                parent: listView
                x: listView.width - verticalScrollBar.width
                y: 0
                height: listView.availableHeight
                orientation: Qt.Vertical

                show: (hoverHandler.hovered || verticalScrollBar.inUse)
                      && verticalScrollBar.isNeeded
            }

            delegate: ItemDelegate {
                id: itemDelegate

                required property string name
                required property bool hidden

                required property int index


                width: dropdownPopup.width - dropdownPopup.leftPadding - dropdownPopup.rightPadding
                height: control.style.controlSize.height - 2 * control.style.borderWidth
                padding: 0
                checkable: true
                checked: !itemDelegate.hidden

                onToggled: {
                    listView.model.setProperty(itemDelegate.index, "hidden", !itemDelegate.checked)
                }

                contentItem: Text {
                    leftPadding: itemDelegateIconArea.width

                    text: itemDelegate.name
                    font: control.font
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    color: {
                        if (!itemDelegate.enabled)
                            return control.style.text.disabled

                        return itemDelegate.highlighted ? control.style.text.selectedText
                                                        : control.style.text.idle
                    }
                }

                Item {
                    id: itemDelegateIconArea
                    width: itemDelegate.height
                    height: itemDelegate.height

                    T.Label {
                        id: itemDelegateIcon
                        text: StudioTheme.Constants.tickIcon
                        color: itemDelegate.highlighted ? control.style.text.selectedText
                                                        : control.style.text.idle
                        font.family: StudioTheme.Constants.iconFont.family
                        font.pixelSize: control.style.smallIconFontSize
                        visible: itemDelegate.checked
                        anchors.fill: parent
                        renderType: Text.NativeRendering
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                highlighted: itemDelegate.hovered

                background: Rectangle {
                    id: itemDelegateBackground
                    x: 0
                    y: 0
                    width: itemDelegate.width
                    height: itemDelegate.height
                    color: itemDelegate.highlighted ? control.style.interaction : "transparent"
                }
            }
        }

        background: Rectangle {
            color: control.style.popup.background
            border.width: 0
        }

        enter: Transition {}
        exit: Transition {}
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hovered && !control.pressed && !control.checked
            PropertyChanges {
                target: controlBackground
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.icon.idle
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.icon.idle
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hovered && !control.pressed && !control.checked
            PropertyChanges {
                target: controlBackground
                color: control.style.background.hover
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.icon.hover
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.icon.hover
            }
        },
        State {
            name: "hoverCheck"
            when: control.enabled && control.hovered && !control.pressed && control.checked
            PropertyChanges {
                target: controlBackground
                color: control.style.interactionHover
                border.color: control.style.interactionHover
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.text.selectedText
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.text.selectedText
            }
        },
        State {
            name: "press"
            when: control.enabled && control.hovered && control.pressed
            PropertyChanges {
                target: controlBackground
                color: control.style.interaction
                border.color: control.style.interaction
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.icon.interaction
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.icon.interaction
            }
        },
        State {
            name: "check"
            when: control.enabled && !control.pressed && control.checked
            extend: "hoverCheck"
            PropertyChanges {
                target: controlBackground
                color: control.style.interaction
                border.color: control.style.interaction
            }
        },
        State {
            name: "pressNotHover"
            when: control.enabled && !control.hovered && control.pressed
            extend: "hover"
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: controlBackground
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.icon.disabled
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.icon.disabled
            }
        }
    ]
}
