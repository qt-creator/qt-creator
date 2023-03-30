// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

TextField {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property real relativePopupX: 0 // TODO Maybe call it leftPadding
    property real popupWidth: control.width
    property string txtStorage

    property int temp: 0

    T.Popup {
        id: popup
        x: control.relativePopupX
        y: control.height - control.style.borderWidth
        width: control.popupWidth
        height: scrollView.height
        background: Rectangle {
            color: control.style.popup.background
            border.color: control.style.interaction
            border.width: control.style.borderWidth
        }

        contentItem: ScrollView {
            id: scrollView
            style: control.style
            padding: 0
            height: Math.min(textAreaPopup.contentHeight + scrollView.topPadding
                             + scrollView.bottomPadding,
                             control.style.maxTextAreaPopupHeight)
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

            T.TextArea {
                id: textAreaPopup
                padding: 10
                width: textAreaPopup.contentWidth + textAreaPopup.leftPadding
                       + textAreaPopup.rightPadding
                anchors.fill: parent
                font.pixelSize: control.style.baseFontSize
                color: control.style.text.idle
                selectionColor: control.style.text.selection
                selectedTextColor: control.style.text.selectedText
                selectByMouse: true
                persistentSelection: textAreaPopup.focus

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    enabled: true
                    cursorShape: Qt.IBeamCursor
                    acceptedButtons: Qt.RightButton
                    onPressed: contextMenu.popup(textAreaPopup)
                }
            }
        }

        ContextMenu {
            id: contextMenu
            style: control.style
            __parentControl: textAreaPopup
        }

        AbstractButton {
            id: acceptButton
            style: control.style
            x: popup.width - acceptButton.width
            y: popup.height - control.style.borderWidth
            width: Math.round(control.style.smallControlSize.width)
            height: Math.round(control.style.smallControlSize.height)
            buttonIcon: StudioTheme.Constants.tickIcon
        }

        AbstractButton {
            id: discardButton
            style: control.style
            x: popup.width - acceptButton.width - discardButton.width + control.style.borderWidth
            y: popup.height - control.style.borderWidth
            width: Math.round(control.style.smallControlSize.width)
            height: Math.round(control.style.smallControlSize.height)
            buttonIcon: StudioTheme.Constants.closeCross
        }

        Component.onCompleted: control.storeAndFormatTextInput(control.text)

        onOpened: {
            textAreaPopup.text = control.txtStorage
            control.clear()
        }

        onClosed: {
            control.storeAndFormatTextInput(textAreaPopup.text)
            control.forceActiveFocus()
            textAreaPopup.deselect()
        }
    }

    function storeAndFormatTextInput(inputText) {
        control.txtStorage = inputText
        var pos = control.txtStorage.search(/\n/g)
        var sliceAt = Math.min(pos, 15)
        control.text = control.txtStorage.slice(0, sliceAt).padEnd(sliceAt + 3, '.')
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape) {
            if (popup.opened)
                popup.close()
            else
                control.focus = false
        }

        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !popup.opened) {
            popup.open()
            textAreaPopup.forceActiveFocus()
        }
    }
}
