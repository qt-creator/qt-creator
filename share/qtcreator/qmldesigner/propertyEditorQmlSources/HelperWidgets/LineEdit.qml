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

import QtQuick 2.2
import QtQuick.Controls 1.1 as Controls
import QtQuick.Controls.Styles 1.0
import QtQuickDesignerTheme 1.0

Controls.TextField {

    Controls.Action {
        //Workaround to avoid that "Delete" deletes the item.
        shortcut: "Delete"
    }

    id: lineEdit
    property variant backendValue
    property color borderColor: "#222"
    property color highlightColor: "orange"
    property color textColor: colorLogic.textColor

    property bool showTranslateCheckBox: true

    property bool writeValueManually: false

    property bool __dirty: false

    property bool showExtendedFunctionButton: true

    signal commitData

    property string context

    function setTranslateExpression()
    {
        if (translateFunction() === "qsTranslate") {
            backendValue.expression = translateFunction()
                    + "(\"" + backendValue.getTranslationContext()
                    + "\", " + "\"" + trCheckbox.escapeString(text) + "\")"
        } else {
            backendValue.expression = translateFunction() + "(\"" + trCheckbox.escapeString(text) + "\")"
        }
    }

    ExtendedFunctionButton {
        x: 2
        anchors.verticalCenter: parent.verticalCenter
        backendValue: lineEdit.backendValue
        visible: lineEdit.enabled && showExtendedFunctionButton
    }

    ColorLogic {
        id: colorLogic
        backendValue: lineEdit.backendValue
        onValueFromBackendChanged: {
            if (writeValueManually) {
                lineEdit.text = convertColorToString(valueFromBackend)
            } else {
                lineEdit.text = valueFromBackend
            }
            __dirty = false
        }
    }

    onTextChanged: {
        __dirty = true
    }

    Connections {
        target: modelNodeBackend
        onSelectionToBeChanged: {
            if (__dirty && !writeValueManually) {
                lineEdit.backendValue.value = text
            } else if (__dirty) {
                commitData()
            }

            __dirty = false
        }
    }

    onEditingFinished: {

        if (writeValueManually)
            return

        if (!__dirty)
            return

        if (backendValue.isTranslated) {
           setTranslateExpression()
        } else {
            if (lineEdit.backendValue.value !== text)
                lineEdit.backendValue.value = text;
        }
        __dirty = false
    }

    style: TextFieldStyle {

        selectionColor: Theme.color(Theme.PanelTextColorLight)
        selectedTextColor: Theme.color(Theme.PanelTextColorMid)
        textColor: lineEdit.textColor
        placeholderTextColor: Theme.color(Theme.PanelTextColorMid)

        padding.top: 2
        padding.bottom: 2
        padding.left: 16
        padding.right: lineEdit.showTranslateCheckBox ? 16 : 1
        background: Rectangle {
            implicitWidth: 100
            implicitHeight: 24
            color: Theme.qmlDesignerBackgroundColorDarker()
            border.color: Theme.qmlDesignerBorderColor()
        }
    }

    Controls.CheckBox {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        id: trCheckbox


        property bool isTranslated: colorLogic.backendValue.isTranslated
        property bool backendValueValue: colorLogic.backendValue.value

        onIsTranslatedChanged: {
            checked = lineEdit.backendValue.isTranslated
        }

        onBackendValueValueChanged: {
            checked = lineEdit.backendValue.isTranslated
        }

        onClicked: {
            if (trCheckbox.checked) {
                setTranslateExpression()
            } else {
                var textValue = lineEdit.text
                lineEdit.backendValue.value = textValue
            }
            colorLogic.evaluate();
        }

        function escapeString(string) {
            var str  = string;
            str = str.replace(/\\/g, "\\\\");
            str.replace(/\"/g, "\\\"");
            str = str.replace(/\t/g, "\\t");
            str = str.replace(/\r/g, "\\r");
            str = str.replace(/\n/g, '\\n');
            return str;
        }

        visible: showTranslateCheckBox


        style: CheckBoxStyle {
            spacing: 8
            indicator: Item {
                implicitWidth: 15
                implicitHeight: 15
                x: 7
                y: 1
                Rectangle {
                    anchors.fill: parent
                    border.color: Theme.qmlDesignerBorderColor()
                    color: Theme.qmlDesignerBackgroundColorDarker()
                    opacity: control.hovered || control.pressed ? 1 : 0.75
                }
                Image {
                    x: 1
                    y: 1
                    width: 13
                    height: 13
                    source: "image://icons/tr"
                    opacity: control.checked ? 1 : 0.3;
                }
            }
        }
    }
}
