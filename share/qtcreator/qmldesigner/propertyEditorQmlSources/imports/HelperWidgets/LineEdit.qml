// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme

StudioControls.TextField {
    id: lineEdit

    property variant backendValue

    property bool writeValueManually: false
    property bool writeAsExpression: false

    property bool showTranslateCheckBox: true
    property bool showExtendedFunctionButton: true
    property string context

    property bool __dirty: false

    signal commitData

    color: lineEdit.edit ? StudioTheme.Values.themeTextColor : colorLogic.textColor
    actionIndicator.visible: lineEdit.showExtendedFunctionButton
    translationIndicatorVisible: lineEdit.showTranslateCheckBox

    function setTranslateExpression() {
        if (translateFunction() === "qsTranslate") {
            lineEdit.backendValue.expression = translateFunction()
                    + "(\"" + lineEdit.backendValue.getTranslationContext()
                    + "\", " + "\"" + lineEdit.escapeString(lineEdit.text) + "\")"
        } else {
            lineEdit.backendValue.expression = translateFunction()
                    + "(\"" + lineEdit.escapeString(lineEdit.text) + "\")"
        }
    }

    function escapeString(string) {
        var str = string
        str = str.replace(/\\/g, "\\\\")
        str = str.replace(/\"/g, "\\\"")
        str = str.replace(/\t/g, "\\t")
        str = str.replace(/\r/g, "\\r")
        str = str.replace(/\n/g, '\\n')
        return str
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
            if (colorLogic.valueFromBackend === undefined) {
                lineEdit.text = ""
            } else {
                if (lineEdit.writeValueManually)
                    lineEdit.text = convertColorToString?.(colorLogic.valueFromBackend) ?? ""
                else
                    lineEdit.text = colorLogic.valueFromBackend
            }
            lineEdit.__dirty = false
        }
    }

    onTextChanged: lineEdit.__dirty = true

    Connections {
        target: modelNodeBackend
        function onSelectionToBeChanged() {
            if (lineEdit.__dirty && !lineEdit.writeValueManually) {
                if (lineEdit.writeAsExpression)
                    lineEdit.backendValue.expression = lineEdit.text
                else
                    lineEdit.backendValue.value = lineEdit.text
            } else if (lineEdit.__dirty) {
                commitData()
            }

            lineEdit.__dirty = false
        }
    }

    onEditingFinished: {
        if (lineEdit.writeValueManually)
            return

        if (!lineEdit.__dirty)
            return

        if (lineEdit.backendValue.isTranslated) {
           lineEdit.setTranslateExpression()
        } else {
            if (lineEdit.writeAsExpression) {
                if (lineEdit.backendValue.expression !== lineEdit.text)
                    lineEdit.backendValue.expression = lineEdit.text
            } else if (lineEdit.backendValue.value !== lineEdit.text) {
                lineEdit.backendValue.value = lineEdit.text
            }
        }
        lineEdit.__dirty = false
    }

    property bool isTranslated: colorLogic.backendValue?.isTranslated ?? false

    translationIndicator.onClicked: {
        if (lineEdit.translationIndicator.checked) {
            setTranslateExpression()
        } else {
            var textValue = lineEdit.text
            lineEdit.backendValue.value = textValue
        }
        colorLogic.evaluate()
    }

    property variant backendValueValueInternal: lineEdit.backendValue?.value ?? 0
    onBackendValueValueInternalChanged: {
        lineEdit.translationIndicator.checked = lineEdit.backendValue?.isTranslated ?? false
    }

    onIsTranslatedChanged: {
        lineEdit.translationIndicator.checked = lineEdit.backendValue?.isTranslated ?? false
    }
}
