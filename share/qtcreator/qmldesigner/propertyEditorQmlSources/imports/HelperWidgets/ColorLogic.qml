// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import StudioTheme 1.0 as StudioTheme

QtObject {
    id: root

    property variant backendValue
    property color textColor: StudioTheme.Values.themeTextColor
    property variant valueFromBackend: root.backendValue?.value ?? 0
    property bool baseStateFlag: isBaseState
    property bool isInModel: root.backendValue?.isInModel ?? false
    property bool isInSubState: root.backendValue?.isInSubState ?? false

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
        if (!root.backendValue)
            return

        if (root.errorState) {
            root.textColor = root.__errorTextColor
            return
        }

        if (root.baseStateFlag) {
            root.textColor = root.backendValue.isInModel ? root.__changedTextColor
                                                         : root.__defaultTextColor
        } else {
            root.textColor = root.backendValue.isInSubState ? StudioTheme.Values.themeChangedStateText
                                                            : root.__defaultTextColor
        }
    }
}
