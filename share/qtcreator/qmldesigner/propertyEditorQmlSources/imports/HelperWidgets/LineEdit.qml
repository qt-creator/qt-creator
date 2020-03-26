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
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import QtQuickDesignerTheme 1.0

StudioControls.TextField {
    id: lineEdit

    property variant backendValue
    property color borderColor: "#222"
    property color highlightColor: "orange"
    color: lineEdit.edit ? StudioTheme.Values.themeTextColor : colorLogic.textColor

    property bool showTranslateCheckBox: true
    translationIndicatorVisible: showTranslateCheckBox

    property bool writeValueManually: false

    property bool __dirty: false

    property bool showExtendedFunctionButton: true

    actionIndicator.visible: showExtendedFunctionButton

    signal commitData

    property string context

    function setTranslateExpression()
    {
        if (translateFunction() === "qsTranslate") {
            backendValue.expression = translateFunction()
                    + "(\"" + backendValue.getTranslationContext()
                    + "\", " + "\"" + escapeString(text) + "\")"
        } else {
            backendValue.expression = translateFunction() + "(\"" + escapeString(text) + "\")"
        }
    }

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: lineEdit.backendValue
    }

    actionIndicator.icon.color: extFuncLogic.color
    actionIndicator.icon.text: extFuncLogic.glyph
    actionIndicator.onClicked: extFuncLogic.show()
    actionIndicator.forceVisible: extFuncLogic.menuVisible

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

    property bool isTranslated: colorLogic.backendValue.isTranslated

    translationIndicator.onClicked: {
        if (translationIndicator.checked) {
            setTranslateExpression()
        } else {
            var textValue = lineEdit.text
            lineEdit.backendValue.value = textValue
        }
        colorLogic.evaluate();
    }

    property variant backendValueValueInternal: backendValue.value
    onBackendValueValueInternalChanged: {
        lineEdit.translationIndicator.checked = lineEdit.backendValue.isTranslated
    }

    onIsTranslatedChanged: {
        lineEdit.translationIndicator.checked = lineEdit.backendValue.isTranslated
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

}
