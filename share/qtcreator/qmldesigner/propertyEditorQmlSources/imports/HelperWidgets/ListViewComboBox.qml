// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls

StudioControls.ComboBox {
    id: root

    property var initialModelData
    property bool __isCompleted: false

    editable: true
    textRole: "id"
    valueRole: "id"

    Component.onCompleted: {
        root.__isCompleted = true
        root.resetInitialIndex()
    }

    onInitialModelDataChanged: root.resetInitialIndex()
    onValueRoleChanged: root.resetInitialIndex()
    onModelChanged: root.resetInitialIndex()
    onTextRoleChanged: root.resetInitialIndex()

    function resetInitialIndex() {
        let currentSelectedDataIndex = -1

        // Workaround for proper initialization. Use the initial modelData value and search for it
        // in the model. If nothing was found, set the editText to the initial modelData.
        if (root.textRole === root.valueRole) {
            currentSelectedDataIndex = root.find(root.initialModelData)
        } else {
            for (let i = 0; i < root.count; ++i) {
                if (root.valueAt(i) === root.initialModelData) {
                    currentSelectedDataIndex = i
                    break
                }
            }
        }
        root.currentIndex = currentSelectedDataIndex
        if (root.currentIndex === -1)
            root.editText = root.initialModelData
    }

    function availableValue() {
        if (root.currentIndex !== -1 && root.currentValue !== "")
            return root.currentValue

        return root.editText
    }
}
