// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme
import QtQuickDesignerTheme 1.0

T.ComboBox {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property bool hover: (comboBoxInput.hover || window.visible || popupIndicator.hover)
                         && control.enabled
    property bool edit: false
    property bool open: window.visible
    property bool openUpwards: false

    editable: false
    width: control.style.controlSize.width
    height: control.style.controlSize.height

    leftPadding: 0
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
        closePolicy: T.Popup.CloseOnEscape
        onAboutToShow: {
            control.listView.parent = window.contentItem
            control.listView.visible = true

            var originMapped = control.mapToGlobal(0,0)

            if (control.openUpwards) {
                window.x = originMapped.x + 1 // This is a workaround for the status bar
                window.y = originMapped.y - window.height
            } else {
                window.x = originMapped.x
                window.y = originMapped.y + control.height
            }

            window.show()
            window.requestActivate()

            control.listView.focus = true
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
        width: control.listView.width
        height: control.listView.height + 2 * control.style.borderWidth
        visible: false
        flags: Qt.FramelessWindowHint | Qt.Dialog | Qt.NoDropShadowWindowHint
        modality: Qt.NonModal
        transientParent: null
        color: "transparent"

        onActiveFocusItemChanged: {
            if (window.activeFocusItem === null && !comboBoxInput.hover
                    && !popupIndicator.hover && comboBoxPopup.opened)
                comboBoxPopup.close()
        }

        Rectangle {
            anchors.fill: parent
            color: control.style.popup.background
        }
    }

    property ListView listView: ListView {
        x: 0
        y: control.style.borderWidth
        width: control.width
        height: control.listView.contentHeight
        interactive: false
        model: control.model
        Keys.onEscapePressed: comboBoxPopup.close()
        currentIndex: control.highlightedIndex

        delegate: ItemDelegate {
            id: itemDelegate

            onClicked: {
                control.activated(index)
                comboBoxPopup.close()
            }

            width: control.width
            height: control.style.controlSize.height
            padding: 0

            contentItem: Text {
                leftPadding: itemDelegateIconArea.width
                text: modelData
                color: {
                    if (!itemDelegate.enabled)
                        return control.style.text.disabled

                    return itemDelegate.hovered ? control.style.text.selectedText
                                                : control.style.text.idle
                }
                font: control.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Item {
                id: itemDelegateIconArea
                width: itemDelegate.height
                height: itemDelegate.height

                T.Label {
                    id: itemDelegateIcon
                    text: StudioTheme.Constants.tickIcon
                    color: itemDelegate.hovered ? control.style.text.selectedText
                                                : control.style.text.idle
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: control.style.smallIconFontSize
                    visible: control.currentIndex === index
                    anchors.fill: parent
                    renderType: Text.NativeRendering
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            background: Rectangle {
                id: itemDelegateBackground
                x: control.style.borderWidth
                y: 0
                width: itemDelegate.width - 2 * control.style.borderWidth
                height: itemDelegate.height
                color: itemDelegate.hovered ? control.style.interaction : "transparent"
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
                border.color: control.style.border.idle
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hover && !control.edit && !control.open
                  && !control.activeFocus && !control.hasActiveDrag
            PropertyChanges {
                target: comboBoxBackground
                border.color: control.style.border.hover
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
                border.color: control.style.border.interaction
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: comboBoxBackground
                border.color: control.style.border.disabled
            }
        }
    ]
}
