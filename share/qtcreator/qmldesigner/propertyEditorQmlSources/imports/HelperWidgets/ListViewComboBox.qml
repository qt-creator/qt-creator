// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0

import QtQuick 2.15
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls

StudioControls.ComboBox {
    id: comboBox

    property alias typeFilter: itemFilterModel.typeFilter

    property var initialModelData
    property bool __isCompleted: false

    editable: true
    model: itemFilterModel
    textRole: "IdRole"
    valueRole: "IdRole"

    HelperWidgets.ItemFilterModel {
        id: itemFilterModel
        modelNodeBackendProperty: modelNodeBackend
    }

    Component.onCompleted: {
        comboBox.__isCompleted = true
        resetInitialIndex()
    }

    onInitialModelDataChanged: resetInitialIndex()
    onValueRoleChanged: resetInitialIndex()
    onModelChanged: resetInitialIndex()
    onTextRoleChanged: resetInitialIndex()

    function resetInitialIndex() {
        let currentSelectedDataIndex = -1

        // Workaround for proper initialization. Use the initial modelData value and search for it
        // in the model. If nothing was found, set the editText to the initial modelData.
        if (textRole === valueRole) {
            currentSelectedDataIndex = comboBox.find(comboBox.initialModelData)
        } else {
            for (let i = 0; i < comboBox.count; ++i) {
                let movingModelIndex = model.index(i)
                let movingModelValueData = model.data(movingModelIndex, valueRole)
                if (movingModelValueData === initialModelData) {
                    currentSelectedDataIndex = i
                    break
                }
            }
        }
        comboBox.currentIndex = currentSelectedDataIndex
        if (comboBox.currentIndex === -1)
            comboBox.editText = comboBox.initialModelData
    }

    function currentData(role = valueRole) {
        if (comboBox.currentIndex !== -1) {
            let currentModelIndex = model.index(currentIndex)
            return model.data(currentModelIndex, role)
        }
        return comboBox.editText
    }

    function availableValue() {
        if (comboBox.currentIndex !== -1 && currentValue !== "")
            return currentValue

        return comboBox.editText
    }
}
