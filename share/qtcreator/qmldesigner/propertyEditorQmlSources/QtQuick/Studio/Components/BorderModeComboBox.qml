// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

ComboBox {
    model: ["Inside", "Middle", "Outside"]
    backendValue: backendValues.borderMode
    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                   + StudioTheme.Values.actionIndicatorWidth
    width: implicitWidth
    useInteger: true
    manualMapping: true

    property bool block: false

    onValueFromBackendChanged: fromBackendToFrontend()

    onCurrentTextChanged: {
        if (!__isCompleted)
            return

        if (block)
            return

        if (currentText === "Inside")
            backendValues.borderMode.value = 0

        if (currentText === "Middle")
            backendValues.borderMode.value = 1

        if (currentText === "Outside")
            backendValues.borderMode.value = 2
    }

    Connections {
        target: modelNodeBackend
        function onSelectionChanged() { fromBackendToFrontend() }
    }

    function fromBackendToFrontend() {
        if (!__isCompleted || backendValues.borderMode === undefined)
            return

        block = true

        if (backendValues.borderMode.value === 0)
            currentIndex = 0
        if (backendValues.borderMode.value === 1)
            currentIndex = 1
        if (backendValues.borderMode.value === 2)
            currentIndex = 2

        block = false
    }
}
