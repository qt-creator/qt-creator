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

import QtQuick 2.1
import QtQuick.Controls 1.0 as Controls
import QtQuick.Controls.Styles 1.1
import "Constants.js" as Constants

Controls.TextField {

    id: textField

    signal rejected

    property bool completeOnlyTypes: false

    property bool completionActive: listView.count > 0
    property bool dotCompletion: false
    property int dotCursorPos: 0
    property string prefix

    property alias showButtons: buttonrow.visible

    property bool fixedSize: false

    function commitCompletion() {
        var cursorPos = textField.cursorPosition

        var string = textField.text
        var before = string.slice(0, cursorPos - textField.prefix.length)
        var after = string.slice(cursorPos)

        textField.text = before + listView.currentItem.text + after

        textField.cursorPosition = cursorPos + listView.currentItem.text.length - prefix.length

        listView.model = null
    }

    ListView {
        id: listView

        clip: true
        cacheBuffer: 0
        snapMode: ListView.SnapToItem
        boundsBehavior: Flickable.StopAtBounds
        visible: textField.completionActive
        delegate: Text {
            text: modelData
            color: creatorTheme.PanelTextColorLight
            Rectangle {
                visible: index === listView.currentIndex
                z: -1
                anchors.fill: parent
                color: creatorTheme.QmlDesignerBackgroundColorDarkAlternate
            }
        }

        anchors.top: parent.top
        anchors.topMargin: 26
        anchors.bottomMargin: textField.fixedSize ? -180 : 12
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 200
        spacing: 2
        children: [
            Rectangle {
                visible: textField.fixedSize
                anchors.fill: parent
                color: creatorTheme.QmlDesignerBackgroundColorDarker
                border.color: creatorTheme.QmlDesignerBorderColor
                anchors.rightMargin: 12
                z: -1
            }

        ]
    }

    verticalAlignment: Text.AlignTop

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: {

        if (event.key === Qt.Key_Period) {
            textField.dotCursorPos = textField.cursorPosition + 1
            var list = autoComplete(textField.text+".", textField.dotCursorPos, false, textField.completeOnlyTypes)
            textField.prefix = list.pop()
            listView.model = list;
            textField.dotCompletion = true
        } else {
            if (textField.completionActive) {
                var list2 = autoComplete(textField.text + event.text,
                                         textField.cursorPosition + event.text.length,
                                         true, textField.completeOnlyTypes)
                textField.prefix = list2.pop()
                listView.model = list2;
            }
        }
    }

    Keys.onSpacePressed: {
        if (event.modifiers & Qt.ControlModifier) {
            var list = autoComplete(textField.text, textField.cursorPosition, true, textField.completeOnlyTypes)
            textField.prefix = list.pop()
            listView.model = list;
            textField.dotCompletion = false

            event.accepted = true;

            if (list.length == 1)
                textField.commitCompletion()

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

    style: TextFieldStyle {
        textColor: creatorTheme.PanelTextColorLight
        padding.top: 6
        padding.bottom: 2
        padding.left: 6
        placeholderTextColor: creatorTheme.PanelTextColorMid
        background: Rectangle {
            implicitWidth: 100
            implicitHeight: 23
            radius: 2
            color: creatorTheme.QmlDesignerBackgroundColorDarker
            border.color: creatorTheme.QmlDesignerBorderColor
        }
    }

    Row {
        id: buttonrow
        spacing: 2
        Button {
            width: 16
            height: 16
            style: ButtonStyle {
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
                            color: control.pressed || control.hovered ? creatorTheme.QmlDesignerBackgroundColorDarker : creatorTheme.QmlDesignerButtonColor
                            border.color: creatorTheme.QmlDesignerBorderColor
                            radius: 2
                        }
                    }
                }
            }
            onClicked: accepted()
        }
        Button {
            width: 16
            height: 16
            style: ButtonStyle {
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
                            color: control.pressed || control.hovered ? creatorTheme.QmlDesignerBackgroundColorDarker : creatorTheme.QmlDesignerButtonColor
                            border.color: creatorTheme.QmlDesignerBorderColor
                            radius: 2
                        }
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
