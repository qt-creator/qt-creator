// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

TextField {
    id: myTextField

    property real relativePopupX: 0 // TODO Maybe call it leftPadding
    property real popupWidth: myTextField.width
    property string txtStorage

    property int temp: 0

    T.Popup {
        id: popup
        x: myTextField.relativePopupX
        y: myTextField.height - StudioTheme.Values.border
        width: myTextField.popupWidth
        height: scrollView.height
        background: Rectangle {
            color: StudioTheme.Values.themePopupBackground
            border.color: StudioTheme.Values.themeInteraction
            border.width: StudioTheme.Values.border
        }

        contentItem: ScrollView {
            id: scrollView
            padding: 0
            height: Math.min(textAreaPopup.contentHeight + scrollView.topPadding
                             + scrollView.bottomPadding,
                             StudioTheme.Values.maxTextAreaPopupHeight)
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

            T.TextArea {
                id: textAreaPopup
                padding: 10
                width: textAreaPopup.contentWidth + textAreaPopup.leftPadding
                       + textAreaPopup.rightPadding
                anchors.fill: parent
                font.pixelSize: StudioTheme.Values.myFontSize
                color: StudioTheme.Values.themeTextColor
                selectionColor: StudioTheme.Values.themeTextSelectionColor
                selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor
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
            myTextEdit: textAreaPopup
        }

        AbstractButton {
            id: acceptButton
            x: popup.width - acceptButton.width
            y: popup.height - StudioTheme.Values.border
            width: Math.round(StudioTheme.Values.smallRectWidth)
            height: Math.round(StudioTheme.Values.smallRectWidth)
            buttonIcon: StudioTheme.Constants.tickIcon
        }

        AbstractButton {
            id: discardButton
            x: popup.width - acceptButton.width - discardButton.width + StudioTheme.Values.border
            y: popup.height - StudioTheme.Values.border
            width: Math.round(StudioTheme.Values.smallRectWidth)
            height: Math.round(StudioTheme.Values.smallRectWidth)
            buttonIcon: StudioTheme.Constants.closeCross
        }

        Component.onCompleted: {
            storeAndFormatTextInput(myTextField.text)
        }

        onOpened: {
            textAreaPopup.text = txtStorage
            myTextField.clear()
        }

        onClosed: {
            storeAndFormatTextInput(textAreaPopup.text)
            myTextField.forceActiveFocus()
            textAreaPopup.deselect()
        }
    }

    function storeAndFormatTextInput(inputText) {
        txtStorage = inputText
        var pos = txtStorage.search(/\n/g)
        var sliceAt = Math.min(pos, 15)
        myTextField.text = txtStorage.slice(0, sliceAt).padEnd(sliceAt + 3, '.')
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape)
            popup.opened ? popup.close() : myTextField.focus = false

        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                && !popup.opened) {
            popup.open()
            textAreaPopup.forceActiveFocus()
        }
    }
}
