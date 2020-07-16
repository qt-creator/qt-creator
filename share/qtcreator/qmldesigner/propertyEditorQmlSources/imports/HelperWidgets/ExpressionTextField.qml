/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuickDesignerTheme 1.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

StudioControls.TextField {
    id: textField

    signal rejected

    translationIndicator.visible: false
    actionIndicator.visible: false

    property bool completeOnlyTypes: false
    property bool completionActive: listView.model !== null
    property bool dotCompletion: false
    property int dotCursorPos: 0
    property string prefix
    property alias showButtons: buttonrow.visible
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
        x: textField.x
        y: textField.height - StudioTheme.Values.border
        width: textField.width
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

                contentItem: Text {
                    id: itemDelegateText
                    leftPadding: 8
                    text: modelData
                    color: StudioTheme.Values.themeTextColor
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
                y: listView.currentItem.y
            }
            highlightFollowsCurrentItem: false
        }

        background: Rectangle {
            color: StudioTheme.Values.themeControlBackground
            border.color: StudioTheme.Values.themeInteraction
            border.width: StudioTheme.Values.border
        }

        enter: Transition {
        }
        exit: Transition {
        }
    }

    verticalAlignment: Text.AlignTop

    onPressed: listView.model = null

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: {
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

    Keys.onSpacePressed: {
        if (event.modifiers & Qt.ControlModifier) {
            var list = autoComplete(textField.text, textField.cursorPosition, true, textField.completeOnlyTypes)
            textField.prefix = textField.text.substring(0, textField.cursorPosition)
            if (list.length && list[list.length - 1] === textField.prefix)
                list.pop()

            listView.model = list
            textField.dotCompletion = false

            event.accepted = true;
        } else {
            event.accepted = false
        }
    }

    Keys.onReturnPressed: {
        event.accepted = false
        if (textField.completionActive) {
            textField.commitCompletion()
            event.accepted = true
        }
    }

    Keys.onEscapePressed: {
        event.accepted = true
        if (textField.completionActive) {
            listView.model = null
        } else {
            textField.rejected()
        }
    }

    Keys.onUpPressed: {
        listView.decrementCurrentIndex()
        event.accepted = false
    }

    Keys.onDownPressed: {
        listView.incrementCurrentIndex()
        event.accepted = false
    }

    Row {
        id: buttonrow
        spacing: 2
        StudioControls.AbstractButton {
            width: 16
            height: 16

            background: Item{
                Image {
                    width: 16
                    height: 16
                    source: "image://icons/ok"
                    opacity: {
                        if (control.pressed)
                            return 0.8;
                        return 1.0;
                    }
                    Rectangle {
                        z: -1
                        anchors.fill: parent
                        color: control.pressed || control.hovered ? Theme.qmlDesignerBackgroundColorDarker() : Theme.qmlDesignerButtonColor()
                        border.color: Theme.qmlDesignerBorderColor()
                        radius: 2
                    }
                }

            }
            onClicked: accepted()
        }
        StudioControls.AbstractButton {
            width: 16
            height: 16

            background: Item {
                Image {
                    width: 16
                    height: 16
                    source: "image://icons/error"
                    opacity: {
                        if (control.pressed)
                            return 0.8;
                        return 1.0;
                    }
                    Rectangle {
                        z: -1
                        anchors.fill: parent
                        color: control.pressed || control.hovered ? Theme.qmlDesignerBackgroundColorDarker() : Theme.qmlDesignerButtonColor()
                        border.color: Theme.qmlDesignerBorderColor()
                        radius: 2
                    }

                }
            }
            onClicked: {
                rejected()
            }
        }
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 4
    }
}
