// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ScriptsEditor as ScriptsEditor
import ScriptEditorBackend

Column {
    id: root

    readonly property real horizontalSpacing: 10
    readonly property real verticalSpacing: 12
    readonly property real columnWidth: (root.width - root.horizontalSpacing) / 2

    property var backend

    property bool keepOpen: scriptEditor.keepOpen

    width: parent.width
    spacing: root.verticalSpacing

    TapHandler {
        onTapped: root.forceActiveFocus()
    }

    Row {
        spacing: root.horizontalSpacing

        HelperWidgets.PopupLabel {
            width: root.columnWidth
            text: qsTr("Signal")
            tooltip: qsTr("Sets an interaction method that connects to the <b>Target</b> component.")
        }

        HelperWidgets.PopupLabel {
            width: root.columnWidth
            text: qsTr("Action")
            tooltip: qsTr("Sets an action that is associated with the selected <b>Target</b> component's <b>Signal</b>.")
        }
    }

    Row {
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: signal

            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth

            model: backend.signal.name.model ?? 0

            onActivated: backend.signal.name.activateIndex(signal.currentIndex)
            property int currentTypeIndex: backend.signal.name.currentIndex ?? 0
            onCurrentTypeIndexChanged: signal.currentIndex = signal.currentTypeIndex
        }

        ScriptsEditor.ActionsComboBox {
            id: action

            width: root.columnWidth
            backend: root.backend
        }
    }

    ScriptsEditor.ScriptEditorForm {
        id: scriptEditor

        anchors.left: parent.left
        anchors.right: parent.right

        horizontalSpacing: root.horizontalSpacing
        verticalSpacing: root.verticalSpacing
        columnWidth: root.columnWidth
        spacing: root.spacing

        backend: root.backend

        currentAction: action.currentValue ?? StatementDelegate.Custom

        itemTooltip: qsTr("Sets the component that is affected by the action of the <b>Target</b> component's <b>Signal</b>.")
        methodTooltip: qsTr("Sets the item component's method that is affected by the <b>Target</b> component's <b>Signal</b>.")
        fromTooltip: qsTr("Sets the component and its property from which the value is copied when the <b>Target</b> component initiates the <b>Signal</b>.")
        toTooltip: qsTr("Sets the component and its property to which the copied value is assigned when the <b>Target</b> component initiates the <b>Signal</b>.")
        addConditionTooltip: qsTr("Sets a logical condition for the selected <b>Signal</b>. It works with the properties of the <b>Target</b> component.")
        removeConditionTooltip: qsTr("Removes the logical condition for the <b>Target</b> component.")
        stateGroupTooltip: qsTr("Sets a <b>State Group</b> that is accessed when the <b>Target</b> component initiates the <b>Signal</b>.")
        stateTooltip: qsTr("Sets a <b>State</b> within the assigned <b>State Group</b> that is accessed when the <b>Target</b> component initiates the <b>Signal</b>.")
        propertyTooltip: qsTr("Sets the property of the component that is affected by the action of the <b>Target</b> component's <b>Signal</b>.")
        valueTooltip: qsTr("Sets the value of the property of the component that is affected by the action of the <b>Target</b> component's <b>Signal</b>.")
        messageTooltip: qsTr("Sets a text that is printed when the <b>Signal</b> of the <b>Target</b> component initiates.")
    }
}

