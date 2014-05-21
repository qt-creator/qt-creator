/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


import QtQuick 2.2
import QtQuick.Controls 1.1 as Controls
import QtQuick.Controls.Styles 1.0

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

    property bool hasToConvertColor: false

    property bool __dirty: false

    ExtendedFunctionButton {
        x: 2
        y: 4
        backendValue: lineEdit.backendValue
        visible: lineEdit.enabled
    }

    ColorLogic {
        id: colorLogic
        backendValue: lineEdit.backendValue
        onValueFromBackendChanged: {
            if (hasToConvertColor) {
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
            if (__dirty) {
                lineEdit.backendValue.value = text
            }
            __dirty = false
        }
    }

    onEditingFinished: {

        if (hasToConvertColor)
            return

        if (!__dirty)
            return

        if (backendValue.isTranslated) {
            backendValue.expression = "qsTr(\"" + trCheckbox.escapeString(text) + "\")"
        } else {
            if (lineEdit.backendValue.value !== text)
                lineEdit.backendValue.value = text;
        }
        __dirty = false
    }

    style: TextFieldStyle {
        selectionColor: lineEdit.textColor
        selectedTextColor: "black"
        textColor: lineEdit.textColor
        padding.top: 3
        padding.bottom: 1
        padding.left: 16
        padding.right: lineEdit.showTranslateCheckBox ? 16 : 1
        placeholderTextColor: "gray"
        background: Rectangle {
            implicitWidth: 100
            implicitHeight: 23
            border.color: borderColor
            radius: 3
            gradient: Gradient {
                GradientStop {color: "#2c2c2c" ; position: 0}
                GradientStop {color: "#343434" ; position: 0.15}
                GradientStop {color: "#373737" ; position: 1.0}
            }
            Rectangle {
                border.color: highlightColor
                anchors.fill: parent
                anchors.margins: -1
                color: "transparent"
                radius: 4
                opacity: 0.3
                visible: control.activeFocus
            }
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
                lineEdit.backendValue.expression = "qsTr(\"" + escapeString(lineEdit.text) + "\")"
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
            indicator:  Item {
                implicitWidth: 16
                implicitHeight: 16
                Image { source: "qrc:qmldesigner/images/checkbox_tr_" +
                                (control.checked ? "checked": "unchecked") +
                                (control.pressed ? "_pressed": "") + ".png" }
            }
        }                                          //control.pressed ? "qrc:qmldesigner/images/checkbox_unchecked_pressed.png" :

    }
}
