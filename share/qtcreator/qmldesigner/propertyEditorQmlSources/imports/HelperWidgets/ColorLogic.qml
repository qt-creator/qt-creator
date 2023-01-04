// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import StudioTheme 1.0 as StudioTheme

QtObject {
    id: root

    property variant backendValue
    property color textColor: StudioTheme.Values.themeTextColor
    property variant valueFromBackend: root.backendValue === undefined ? 0 : root.backendValue.value
    property bool baseStateFlag: isBaseState
    property bool isInModel: {
        if (root.backendValue !== undefined && root.backendValue.isInModel !== undefined)
            return root.backendValue.isInModel

        return false
    }
    property bool isInSubState: {
        if (root.backendValue !== undefined && root.backendValue.isInSubState !== undefined)
            return root.backendValue.isInSubState

        return false
    }
    property bool highlight: root.textColor === root.__changedTextColor
    property bool errorState: false

    readonly property color __defaultTextColor: StudioTheme.Values.themeTextColor
    readonly property color __changedTextColor: StudioTheme.Values.themeInteraction
    readonly property color __errorTextColor: StudioTheme.Values.themeError

    onBackendValueChanged: root.evaluate()
    onValueFromBackendChanged: root.evaluate()
    onBaseStateFlagChanged: root.evaluate()
    onIsInModelChanged: root.evaluate()
    onIsInSubStateChanged: root.evaluate()
    onErrorStateChanged: root.evaluate()

    function evaluate() {
        if (root.backendValue === undefined)
            return

        if (root.errorState) {
            root.textColor = root.__errorTextColor
            return
        }

        if (root.baseStateFlag) {
            if (root.backendValue.isInModel)
                root.textColor = root.__changedTextColor
            else
                root.textColor = root.__defaultTextColor
        } else {
            if (root.backendValue.isInSubState)
                root.textColor = StudioTheme.Values.themeChangedStateText
            else
                root.textColor = root.__defaultTextColor
        }
    }
}
