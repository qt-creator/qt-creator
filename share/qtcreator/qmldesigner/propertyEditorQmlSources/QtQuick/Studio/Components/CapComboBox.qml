// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

ComboBox {
    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                   + StudioTheme.Values.actionIndicatorWidth
    width: implicitWidth
    model: ["FlatCap", "SquareCap", "RoundCap"]
    backendValue: backendValues.capStyle
    useInteger: true
    manualMapping: true

    property bool block: false

    onValueFromBackendChanged: fromBackendToFrontend()

    onCurrentTextChanged: {
        if (!__isCompleted)
            return

        if (block)
            return

        if (currentText === "FlatCap")
            backendValues.capStyle.value = 0

        if (currentText === "SquareCap")
            backendValues.capStyle.value = 16

        if (currentText === "RoundCap")
            backendValues.capStyle.value = 32
    }

    Connections {
        target: modelNodeBackend
        function onSelectionChanged() { fromBackendToFrontend() }
    }

    function fromBackendToFrontend()
    {
        if (!__isCompleted || backendValues.capStyle === undefined)
            return

        block = true

        if (backendValues.capStyle.value === 0)
            currentIndex = 0
        if (backendValues.capStyle.value === 16)
            currentIndex = 1
        if (backendValues.capStyle.value === 32)
            currentIndex = 2

        block = false
    }
}
