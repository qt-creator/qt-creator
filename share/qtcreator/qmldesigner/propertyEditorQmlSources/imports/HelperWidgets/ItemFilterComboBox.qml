// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import HelperWidgets 2.0 as HelperWidgets

HelperWidgets.ComboBox {
    id: comboBox

    property alias typeFilter: itemFilterModel.typeFilter

    manualMapping: true
    editable: true
    model: comboBox.addDefaultItem(itemFilterModel.itemModel)

    validator: RegularExpressionValidator { regularExpression: /(^$|^[a-z_]\w*)/ }

    HelperWidgets.ItemFilterModel {
        id: itemFilterModel
        modelNodeBackendProperty: modelNodeBackend
    }

    property string defaultItem: qsTr("[None]")
    property string textValue: comboBox.backendValue?.expression ?? ""
    property bool block: false
    property bool dirty: true

    onTextValueChanged: {
        if (comboBox.block)
            return

        comboBox.setCurrentText(comboBox.textValue)
    }
    onModelChanged: comboBox.setCurrentText(comboBox.textValue)
    onEditTextChanged: comboBox.dirty = true
    onFocusChanged: {
        if (comboBox.dirty)
           comboBox.handleActivate(comboBox.currentIndex)
    }

    onCompressedActivated: function(index, reason) { comboBox.handleActivate(index) }
    onAccepted: comboBox.setCurrentText(comboBox.editText)

    Component.onCompleted: comboBox.setCurrentText(comboBox.textValue)

    function handleActivate(index) {
        if (!comboBox.__isCompleted || comboBox.backendValue === undefined)
            return

        var cText = (index === -1) ? comboBox.editText : comboBox.textAt(index)
        comboBox.block = true
        comboBox.setCurrentText(cText)
        comboBox.block = false
    }

    function setCurrentText(text) {
        if (!comboBox.__isCompleted || comboBox.backendValue === undefined)
            return

        comboBox.currentIndex = comboBox.find(text)

        if (text === "" || text === "null") {
            comboBox.currentIndex = 0
            comboBox.editText = comboBox.defaultItem
        } else {
            if (comboBox.currentIndex === -1)
                comboBox.editText = text
            else if (comboBox.currentIndex === 0)
                comboBox.editText = comboBox.defaultItem
        }

        if (comboBox.currentIndex === 0) {
            comboBox.backendValue.resetValue()
        } else {
            if (comboBox.backendValue.expression !== comboBox.editText)
                comboBox.backendValue.expression = comboBox.editText
        }
        comboBox.dirty = false
    }

    function addDefaultItem(arr) {
        var copy = arr.slice()
        copy.unshift(comboBox.defaultItem)
        return copy
    }
}
