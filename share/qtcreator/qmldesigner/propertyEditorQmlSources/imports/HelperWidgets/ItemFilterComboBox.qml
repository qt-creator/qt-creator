// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import HelperWidgets 2.0 as HelperWidgets

HelperWidgets.ComboBox {
    id: comboBox

    property alias typeFilter: itemFilterModel.typeFilter

    manualMapping: true
    editable: true
    model: optionsList.model
    valueRole: "id"
    textRole: (comboBox.typeFilter === "QtQuick3D.Texture") ? "idAndName" : "id"

    validator: RegularExpressionValidator { regularExpression: /(^$|^[a-z_]\w*)/ }

    HelperWidgets.ItemFilterModel {
        id: itemFilterModel
        modelNodeBackendProperty: modelNodeBackend
        onModelReset: optionsList.updateModel()
        onRowsInserted: optionsList.updateModel()
        onRowsRemoved: optionsList.updateModel()
    }

    property string defaultItem: qsTr("[None]")
    property string expressionValue: comboBox.backendValue?.expression ?? ""
    property bool block: false
    property bool dirty: true

    onExpressionValueChanged: {
        if (comboBox.block)
            return

        comboBox.updateText()
    }
    onModelChanged: comboBox.updateText()
    onEditTextChanged: comboBox.dirty = true
    onFocusChanged: {
        if (comboBox.dirty)
           comboBox.handleActivate(comboBox.currentIndex)
    }

    onCompressedActivated: function(index, reason) { comboBox.handleActivate(index) }
    onAccepted: comboBox.setCurrentText(comboBox.editText)

    Component.onCompleted: comboBox.updateText()

    function updateText() {
        let idx = itemFilterModel.indexFromId(comboBox.expressionValue)
        if (idx < 0) {
            comboBox.setCurrentText(comboBox.defaultItem)
            return
        }

        let textValue = itemFilterModel.modelItemData(idx)[comboBox.textRole]
        comboBox.setCurrentText(textValue)
    }

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

        if (comboBox.currentIndex < 1) {
            comboBox.backendValue.resetValue()
        } else {
            let valueData = (comboBox.valueRole === "")
                                ? comboBox.editText
                                : itemFilterModel.modelItemData(comboBox.currentIndex - 1)[comboBox.valueRole]

            if (comboBox.backendValue.expression !== valueData)
                comboBox.backendValue.expression = valueData
        }
        comboBox.dirty = false
    }

    Repeater {
        id: optionsList

        property var localModel: []

        function updateModel() {
            optionsList.localModel = []

            if (comboBox.textRole !== "" && comboBox.valueRole !== "") {
                let defaultItem = {}
                defaultItem[comboBox.textRole] = comboBox.defaultItem
                defaultItem[comboBox.valueRole] = ""
                optionsList.localModel.push(defaultItem)
            } else {
                optionsList.localModel.push(comboBox.defaultItem)
            }

            let rows = itemFilterModel.rowCount()
            for (let i = 0; i < rows; ++i)
                optionsList.localModel.push(itemFilterModel.modelItemData(i))

            optionsList.model = optionsList.localModel // trigger on change handler
        }
    }
}
