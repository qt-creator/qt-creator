// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme as StudioTheme
import StudioWindowManager

T.ComboBox {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property bool hover: (comboBoxInput.hover || window.visible || popupIndicator.hover)
                         && control.enabled
    property bool edit: false
    property bool open: window.visible
    property bool openUpwards: false
    property alias suffix: comboBoxInput.suffix

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

        onAboutToHide: window.close()
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
        flags: Qt.FramelessWindowHint | Qt.Dialog | Qt.NoDropShadowWindowHint | Qt.WindowStaysOnTopHint
        modality: Qt.NonModal
        transientParent: control.Window.window
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
        id: listView
        x: 0
        y: control.style.borderWidth
        width: control.width
        height: Math.min(control.style.maxComboBoxPopupHeight, control.listView.contentHeight)
        model: control.model
        Keys.onEscapePressed: comboBoxPopup.close()
        currentIndex: control.highlightedIndex
        boundsBehavior: Flickable.StopAtBounds

        HoverHandler { id: hoverHandler }

        ScrollBar.vertical: TransientScrollBar {
            id: verticalScrollBar
            style: control.style
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

            onClicked: {
                // Necessary to keep the transient parent open otherwise it will change the focus
                // to the main window "Utils::AppMainWindowClassWindow" and closes the transient
                // parent.
                window.transientParent.requestActivate()
                comboBoxPopup.close()
                control.currentIndex = index
                control.activated(index)
            }

            width: control.width
            height: control.style.controlSize.height
            padding: 0
            enabled: model.enabled === undefined ? true : model.enabled

            contentItem: Text {
                leftPadding: 8
                rightPadding: verticalScrollBar.style.scrollBarThicknessHover
                text: control.textRole ? (Array.isArray(control.model)
                                          ? modelData[control.textRole]
                                          : model[control.textRole])
                                       : modelData
                color: {
                    if (!itemDelegate.enabled)
                        return control.style.text.disabled

                    if (control.currentIndex === index)
                        return control.style.text.selectedText

                    return control.style.text.idle
                }
                font: control.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                id: itemDelegateBackground
                x: control.style.borderWidth
                y: 0
                width: itemDelegate.width - 2 * control.style.borderWidth
                height: itemDelegate.height
                color: {
                    if (!itemDelegate.enabled)
                        return "transparent"

                    if (itemDelegate.hovered && control.currentIndex === index)
                        return control.style.interactionHover

                    if (control.currentIndex === index)
                        return control.style.interaction

                    if (itemDelegate.hovered)
                        return control.style.background.hover

                    return "transparent"
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
