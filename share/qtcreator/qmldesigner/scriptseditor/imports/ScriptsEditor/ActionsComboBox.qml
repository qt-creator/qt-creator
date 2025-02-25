// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ScriptEditorBackend

StudioControls.TopLevelComboBox {
    id: action

    style: StudioTheme.Values.connectionPopupControlStyle
    textRole: "text"
    valueRole: "value"

    property var backend
    property int indexFromBackend: indexOfValue(backend.actionType)

    onIndexFromBackendChanged: action.currentIndex = action.indexFromBackend
    onActivated: backend.changeActionType(action.currentValue)

    model: ListModel {
        ListElement {
            value: StatementDelegate.CallFunction
            text: qsTr("Call Function")
            enabled: true
        }
        ListElement {
            value: StatementDelegate.Assign
            text: qsTr("Assign")
            enabled: true
        }
        ListElement {
            value: StatementDelegate.ChangeState
            text: qsTr("Change State")
            enabled: true
        }
        ListElement {
            value: StatementDelegate.SetProperty
            text: qsTr("Set Property")
            enabled: true
        }
        ListElement {
            value: StatementDelegate.PrintMessage
            text: qsTr("Print Message")
            enabled: true
        }
        ListElement {
            value: StatementDelegate.Custom
            text: qsTr("Custom")
            enabled: false
        }
    }
}
