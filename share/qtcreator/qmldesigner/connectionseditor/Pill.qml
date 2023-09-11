// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme

FocusScope {
    id: root

    required property int index
    required property string value
    required property int type

    function setCursorBegin() { textInput.cursorPosition = 0 }
    function setCursorEnd() { textInput.cursorPosition = textInput.text.length }

    function isEditable() { return root.type === ConditionListModel.Intermediate
                                   || root.type === ConditionListModel.Literal
                                   || root.type === ConditionListModel.Invalid }

    function isIntermediate() { return root.type === ConditionListModel.Intermediate }
    function isLiteral() { return root.type === ConditionListModel.Literal }
    function isOperator() { return root.type === ConditionListModel.Operator }
    function isProperty() { return root.type === ConditionListModel.Variable }
    function isShadow() { return root.type === ConditionListModel.Shadow }
    function isInvalid() { return root.type === ConditionListModel.Invalid }

    signal remove()
    signal update(var value)
    signal submit(int cursorPosition)

    readonly property int margin: StudioTheme.Values.flowPillMargin

    width: {
        if (root.isEditable()) {
            if (root.isInvalid())
                return textInput.width + 1 + 2 * root.margin
            else
                return textInput.width + 1
        }
        return textItem.contentWidth + icon.width + root.margin
    }
    height: StudioTheme.Values.flowPillHeight

    onActiveFocusChanged: {
        if (root.activeFocus && root.isEditable())
            textInput.forceActiveFocus()
    }

    Keys.onPressed: function (event) {
        if (root.isEditable())
            return

        if (event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete)
            root.remove()
    }

    MouseArea {
        id: rootMouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.isEditable() ? Qt.IBeamCursor : Qt.ArrowCursor
        onClicked: root.forceActiveFocus()
    }

    Rectangle {
        id: pill
        anchors.fill: parent
        color: {
            if (root.isShadow())
                return StudioTheme.Values.themeInteraction
            if (root.isEditable())
                return "transparent"

            return StudioTheme.Values.themePillBackground
        }
        border.color: root.isInvalid() ? StudioTheme.Values.themeWarning : "white" // TODO colors
        border.width: {
            if (root.isShadow())
                return 0
            if (root.isInvalid())
                 return 1
            if (root.isEditable())
                return 0
            if (rootMouseArea.containsMouse || root.focus)
                return 1

            return 0
        }
        radius: 4

        Row {
            id: row
            anchors.left: parent.left
            anchors.leftMargin: root.margin
            anchors.verticalCenter: parent.verticalCenter
            visible: root.isOperator() || root.isProperty() || root.isShadow()

            Text {
                id: textItem
                font.pixelSize: StudioTheme.Values.baseFontSize
                color: root.isShadow() ? StudioTheme.Values.themeTextSelectedTextColor
                                       : StudioTheme.Values.themeTextColor
                text: root.value
                anchors.verticalCenter: parent.verticalCenter
            }

            Item {
                id: icon
                width: root.isShadow() ? root.margin : StudioTheme.Values.flowPillHeight
                height: StudioTheme.Values.flowPillHeight
                visible: !root.isShadow()

                Text {
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.smallIconFontSize
                    color: StudioTheme.Values.themeIconColor
                    text: StudioTheme.Constants.close_small
                    anchors.centerIn: parent
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    onClicked: root.remove()
                }
            }
        }

        TextInput {
            id: textInput

            x: root.isInvalid() ? root.margin : 0
            height: StudioTheme.Values.flowPillHeight
            topPadding: 1
            font.pixelSize: StudioTheme.Values.baseFontSize
            color: (rootMouseArea.containsMouse || textInput.activeFocus) ? StudioTheme.Values.themeIconColor
                                                                          : StudioTheme.Values.themeTextColor
            text: root.value
            visible: root.isEditable()
            enabled: root.isEditable()

            //validator: RegularExpressionValidator { regularExpression: /^\S+/ }

            onEditingFinished: {
                root.update(textInput.text) // emit
                root.submit(textInput.cursorPosition) // emit
            }

            Keys.onPressed: function (event) {
                if (event.key === Qt.Key_Backspace) {
                    if (textInput.text !== "")
                        return

                    root.remove() // emit
                }
            }
        }
    }
}
