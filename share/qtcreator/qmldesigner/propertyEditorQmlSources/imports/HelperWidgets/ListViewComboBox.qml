// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls

StudioControls.ComboBox {
    id: comboBox

    property alias typeFilter: itemFilterModel.typeFilter

    property var initialModelData
    property bool __isCompleted: false

    editable: true
    model: itemFilterModel.itemModel

    HelperWidgets.ItemFilterModel {
        id: itemFilterModel
        modelNodeBackendProperty: modelNodeBackend
    }

    Component.onCompleted: {
        comboBox.__isCompleted = true

        // Workaround for proper initialization. Use the initial modelData value and search for it
        // in the model. If nothing was found, set the editText to the initial modelData.
        comboBox.currentIndex = comboBox.find(comboBox.initialModelData)

        if (comboBox.currentIndex === -1)
            comboBox.editText = comboBox.initialModelData
    }
}
