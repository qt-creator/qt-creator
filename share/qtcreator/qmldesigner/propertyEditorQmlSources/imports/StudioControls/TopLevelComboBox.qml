// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.ComboBox {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property bool hover: (comboBoxInput.hover || window.visible) && control.enabled
    property bool edit: false
    property bool open: window.visible
    property bool openUpwards: false

    editable: false
    width: control.style.controlSize.width
    height: control.style.controlSize.height

    leftPadding: control.style.borderWidth
    rightPadding: popupIndicator.width + control.style.borderWidth
    font.pixelSize: control.style.baseFontSize

    delegate: ItemDelegate {
        required property int index
        required property var modelData

        width: control.width
        highlighted: control.highlightedIndex === index
        contentItem: Text {
            text: modelData
            color: "#21be2b"
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    indicator: CheckIndicator {
        id: popupIndicator
        style: control.style
        __parentControl: control
        __parentPopup: comboBoxPopup
        x: comboBoxInput.x + comboBoxInput.width
        y: control.style.borderWidth
        width: control.style.squareControlSize.width - control.style.borderWidth
        height: control.style.squareControlSize.height - control.style.borderWidth * 2
    }

    contentItem: ComboBoxInput {
        id: comboBoxInput
        style: control.style
        __parentControl: control
        text: control.editText
    }

    background: Rectangle {
        id: comboBoxBackground
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth
        width: control.width
        height: control.height
    }

    popup: T.Popup {
        id: comboBoxPopup
        width: 0
        height: 0
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent
        onAboutToShow: {
            control.menuDelegate.parent = window.contentItem
            control.menuDelegate.visible = true

            window.show()
            window.requestActivate()
            if (!control.openUpwards) {
                window.x = control.mapToGlobal(0,0).x
                window.y = control.mapToGlobal(0,0).y + control.height
            } else {
                window.x = control.mapToGlobal(0,0).x
                window.y = control.mapToGlobal(0,0).y - window.height
            }

            control.menuDelegate.focus = true
        }

        onAboutToHide: window.hide()
    }

    // Close popup when application goes to background
    Connections {
        target: Qt.application
        function onStateChanged() {
            if (Qt.application.state === Qt.ApplicationInactive)
                comboBoxPopup.close()
        }
    }

    Window {
        id: window
        width: control.menuDelegate.implicitWidth
        height: control.menuDelegate.implicitHeight
        visible: false
        flags: Qt.Popup | Qt.NoDropShadowWindowHint
        modality: Qt.NonModal
        transientParent: control.Window.window
        color: "transparent"

        onActiveChanged: {
            if (!window.active)
                comboBoxPopup.close()
        }
    }

    property Menu menuDelegate: Menu {
        id: textEditMenu
        y: 0
        width: control.width
        overlap: 0

        Repeater {
            model: control.model

            MenuItem {
                id: menuItem
                x: 0
                text: modelData
                onTriggered: {
                    control.currentIndex = index
                    control.activated(index)
                    comboBoxPopup.close()
                }

                background: Rectangle {
                    implicitWidth: textLabel.implicitWidth + menuItem.labelSpacing
                                   + menuItem.leftPadding + menuItem.rightPadding
                    implicitHeight: control.style.controlSize.height
                    x: control.style.borderWidth
                    y: control.style.borderWidth
                    width: menuItem.menu.width - (control.style.borderWidth * 2)
                    height: menuItem.height - (control.style.borderWidth * 2)
                    color: menuItem.highlighted ? control.style.interaction
                                                : "transparent"
                }

                contentItem: Item {
                    Text {
                        id: textLabel
                        leftPadding: itemDelegateIconArea.width
                        text: menuItem.text
                        font: control.font
                        color: menuItem.highlighted ? control.style.text.selectedText
                                                    : control.style.text.idle
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Item {
                        id: itemDelegateIconArea
                        width: menuItem.height
                        height: menuItem.height

                        T.Label {
                            id: itemDelegateIcon
                            text: StudioTheme.Constants.tickIcon
                            color: textLabel.color
                            font.family: StudioTheme.Constants.iconFont.family
                            font.pixelSize: control.style.smallIconFontSize
                            visible: control.currentIndex === index
                            anchors.fill: parent
                            renderType: Text.NativeRendering
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hover && !control.edit && !control.open
                  && !control.activeFocus && !control.hasActiveDrag
            PropertyChanges {
                target: control
                wheelEnabled: false
            }
            PropertyChanges {
                target: comboBoxInput
                selectByMouse: false
            }
            PropertyChanges {
                target: comboBoxBackground
                color: control.style.background.idle
            }
        },
        // This state is intended for ComboBoxes which aren't editable, but have focus e.g. via
        // tab focus. It is therefor possible to use the mouse wheel to scroll through the items.
        State {
            name: "focus"
            when: control.enabled && control.activeFocus && !control.editable && !control.open
            PropertyChanges {
                target: control
                wheelEnabled: true
            }
            PropertyChanges {
                target: comboBoxInput
                focus: true
            }
        },
        State {
            name: "popup"
            when: control.enabled && control.open
            PropertyChanges {
                target: control
                wheelEnabled: true
            }
            PropertyChanges {
                target: comboBoxInput
                selectByMouse: false
                readOnly: true
            }
            PropertyChanges {
                target: comboBoxBackground
                color: control.style.background.interaction
                border.color: control.style.border.interaction
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: comboBoxBackground
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
        }
    ]
}
