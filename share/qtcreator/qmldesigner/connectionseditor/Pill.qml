// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import HelperWidgets as HelperWidgets

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
    property var operatorModel

    property bool hovered: rootMouseArea.containsMouse
    property bool selected: root.focus

    property int maxTextWidth: 600

    width: row.width
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

    HelperWidgets.ToolTipArea {
        id: rootMouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.isEditable() ? Qt.IBeamCursor : Qt.ArrowCursor
        onClicked: root.forceActiveFocus()
        tooltip: {
            if (textItem.visible) {
                if (textItem.truncated)
                    return textItem.text
                else
                    return ""
            }

            if (textMetrics.width > textInput.width)
                return textInput.text

            return ""
        }
    }

    Rectangle {
        id: pill
        anchors.fill: parent
        color: {
            if (root.isIntermediate())
                return "transparent"

            if (root.isShadow())
                return StudioTheme.Values.themePillShadowBackground

            if (root.isInvalid() && root.selected)
                return StudioTheme.Values.themeWarning

            if (root.hovered) {
                if (root.isOperator())
                    return StudioTheme.Values.themePillOperatorBackgroundHover
                if (root.isLiteral())
                    return StudioTheme.Values.themePillLiteralBackgroundHover

                return StudioTheme.Values.themePillDefaultBackgroundHover
            }

            if (root.isLiteral())
                return StudioTheme.Values.themePillLiteralBackgroundIdle

            if (root.isOperator())
                return StudioTheme.Values.themePillOperatorBackgroundIdle

            return StudioTheme.Values.themePillDefaultBackgroundIdle
        }
        border.color: root.isInvalid() ? StudioTheme.Values.themeWarning
                                       : StudioTheme.Values.themePillOutline
        border.width: {
            if (root.isShadow())
                return 0
            if (root.isInvalid())
                 return 1
            if (root.isEditable())
                return 0
            if (root.selected)
                return 1

            return 0
        }
        radius: StudioTheme.Values.flowPillRadius

        Row {
            id: row
            leftPadding: root.margin
            rightPadding: icon.visible ? 0 : root.margin

            property int textWidth: Math.min(textMetrics.advanceWidth + 2,
                                             root.maxTextWidth - row.leftPadding
                                             - (icon.visible ? icon.width : root.margin))

            TextMetrics {
                id: textMetrics
                text: textItem.visible ? textItem.text : textInput.text
                font: textItem.font
            }

            Text {
                id: textItem
                width: row.textWidth
                height: StudioTheme.Values.flowPillHeight
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.PlainText
                elide: Text.ElideMiddle
                font.pixelSize: StudioTheme.Values.baseFontSize
                color: root.isShadow() ? StudioTheme.Values.themePillTextSelected
                                       : StudioTheme.Values.themePillText
                text: root.isOperator() ? root.operatorModel.convertValueToName(root.value)
                                        : root.value
                visible: root.isOperator() || root.isProperty() || root.isShadow()
            }

            TextInput {
                id: textInput
                x: root.isInvalid() ? root.margin : 0
                width: row.textWidth
                height: StudioTheme.Values.flowPillHeight
                topPadding: 1
                clip: true
                font.pixelSize: StudioTheme.Values.baseFontSize
                color: {
                    if (root.isIntermediate())
                        return StudioTheme.Values.themePillTextEdit
                    if (root.isInvalid() && root.selected)
                        return StudioTheme.Values.themePillTextSelected
                    return StudioTheme.Values.themePillText
                }

                selectedTextColor:StudioTheme.Values.themePillTextSelected
                selectionColor: StudioTheme.Values.themeInteraction

                text: root.value
                visible: !textItem.visible
                enabled: root.isEditable()

                onEditingFinished: {
                    root.update(textInput.text) // emit
                    root.submit(textInput.cursorPosition) // emit
                }

                Keys.onReleased: function (event) {
                    if (event.key === Qt.Key_Backspace) {
                        if (textInput.text !== "")
                            return

                        root.remove() // emit
                    }
                }
            }

            Item {
                id: icon
                width: StudioTheme.Values.flowPillHeight
                height: StudioTheme.Values.flowPillHeight
                visible: !root.isShadow() && !root.isIntermediate()

                Text {
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.smallIconFontSize
                    color: root.isInvalid() && root.selected ? StudioTheme.Values.themePillTextSelected
                                                             : StudioTheme.Values.themePillText
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
    }
}
