// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import StudioControls as StudioControls
import StudioTheme as StudioTheme

StudioControls.TextField {
    id: textField

    translationIndicator.visible: false
    actionIndicator.visible: false

    property bool completeOnlyTypes: false
    property bool completionActive: listView.model !== null
    property bool dotCompletion: false
    property int dotCursorPos: 0
    property string prefix
    property bool fixedSize: false
    property bool replaceCurrentTextByCompletion: false

    property alias completionList: listView

    function commitCompletion() {
        if (replaceCurrentTextByCompletion) {
            textField.text = listView.currentItem.text
        } else {
            var cursorPos = textField.cursorPosition
            var string = textField.text
            var before = string.slice(0, cursorPos - textField.prefix.length)
            var after = string.slice(cursorPos)

            textField.text = before + listView.currentItem.text + after
            textField.cursorPosition = cursorPos + listView.currentItem.text.length - prefix.length
        }
        listView.model = null
    }

    Popup {
        id: textFieldPopup
        x: StudioTheme.Values.border
        y: textField.height
        width: textField.width - (StudioTheme.Values.border * 2)
        // TODO Setting the height on the popup solved the problem with the popup of height 0,
        // but it has the problem that it sometimes extend over the border of the actual window
        // and is then cut off.
        height: Math.min(contentItem.implicitHeight + textFieldPopup.topPadding + textFieldPopup.bottomPadding,
                         textField.Window.height - topMargin - bottomMargin,
                         StudioTheme.Values.maxComboBoxPopupHeight)
        padding: StudioTheme.Values.border
        margins: 0 // If not defined margin will be -1
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnPressOutsideParent
                     | Popup.CloseOnEscape | Popup.CloseOnReleaseOutside
                     | Popup.CloseOnReleaseOutsideParent

        visible: textField.completionActive

        onClosed: listView.model = null

        contentItem: ListView {
            id: listView
            clip: true
            implicitHeight: contentHeight
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: StudioControls.ScrollBar {
                id: popupScrollBar
            }

            model: null

            delegate: ItemDelegate {
                id: myItemDelegate

                width: textFieldPopup.width - textFieldPopup.leftPadding - textFieldPopup.rightPadding
                       - (popupScrollBar.visible ? popupScrollBar.contentItem.implicitWidth
                                                           + 2 : 0) // TODO Magic number
                height: StudioTheme.Values.height - 2 * StudioTheme.Values.border
                padding: 0
                text: itemDelegateText.text

                highlighted: listView.currentIndex === index

                contentItem: Text {
                    id: itemDelegateText
                    leftPadding: 8
                    text: modelData
                    color: highlighted ? StudioTheme.Values.themeTextSelectedTextColor
                                       : StudioTheme.Values.themeTextColor
                    font: textField.font
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: "transparent"
                }

                hoverEnabled: true
                onHoveredChanged: {
                    if (hovered)
                        listView.currentIndex = index
                }
                onClicked: {
                    listView.currentIndex = index
                    if (textField.completionActive)
                        textField.commitCompletion()
                }
            }

            highlight: Rectangle {
                id: listViewHighlight
                width: textFieldPopup.width - textFieldPopup.leftPadding - textFieldPopup.rightPadding
                       - (popupScrollBar.visible ? popupScrollBar.contentItem.implicitWidth
                                                           + 2 : 0)
                height: StudioTheme.Values.height - 2 * StudioTheme.Values.border
                color: StudioTheme.Values.themeInteraction
                y: listView.currentItem?.y ?? 0
            }
            highlightFollowsCurrentItem: false
        }

        background: Rectangle {
            color: StudioTheme.Values.themePopupBackground
            border.width: 0
        }

        enter: Transition {}
        exit: Transition {}
    }

    verticalAlignment: Text.AlignVCenter

    onPressed: listView.model = null

    onRejected: {
        if (textField.completionActive)
            listView.model = null
    }

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: function(event) {
        var text = textField.text
        var pos = textField.cursorPosition
        var explicitComplete = true

        switch (event.key) {

        case Qt.Key_Period:
            textField.dotCursorPos = textField.cursorPosition + 1
            text = textField.text + "."
            pos = textField.dotCursorPos
            explicitComplete = false
            textField.dotCompletion = true
            break

        case Qt.Key_Right:
            if (!textField.completionActive)
                return

            pos = Math.min(textField.cursorPosition + 1, textField.text.length)
            break

        case Qt.Key_Left:
            if (!textField.completionActive)
                return

            pos = Math.max(0, textField.cursorPosition - 1)
            break

        case Qt.Key_Backspace:
            if (!textField.completionActive)
                return

            pos = textField.cursorPosition - 1
            if (pos < 0)
                return

            text = textField.text.substring(0, pos) + textField.text.substring(textField.cursorPosition)
            break

        case Qt.Key_Delete:
            return

        default:
            if (!textField.completionActive)
                return

            var tmp = textField.text
            text = tmp.substring(0, textField.cursorPosition) + event.text + tmp.substring(textField.cursorPosition)
            pos = textField.cursorPosition + event.text.length
        }

        var list = autoComplete(text.trim(), pos, explicitComplete, textField.completeOnlyTypes)
        textField.prefix = text.substring(0, pos)

        if (list.length && list[list.length - 1] === textField.prefix)
            list.pop()

        listView.model = list
    }

    // Currently deactivated as it is causing a crash when calling autoComplete()
    //Keys.onSpacePressed: function(event) {
    //    if (event.modifiers & Qt.ControlModifier) {
    //        var list = autoComplete(textField.text, textField.cursorPosition, true, textField.completeOnlyTypes)
    //        textField.prefix = textField.text.substring(0, textField.cursorPosition)
    //        if (list.length && list[list.length - 1] === textField.prefix)
    //            list.pop()

    //        listView.model = list
    //        textField.dotCompletion = false

    //        event.accepted = true
    //    } else {
    //        event.accepted = false
    //    }
    //}

    Keys.onReturnPressed: function(event) {
        event.accepted = false
        if (textField.completionActive) {
            textField.commitCompletion()
            event.accepted = true
        }
    }

    Keys.onUpPressed: function(event) {
        listView.decrementCurrentIndex()
        event.accepted = false
    }

    Keys.onDownPressed: function(event) {
        listView.incrementCurrentIndex()
        event.accepted = false
    }
}
