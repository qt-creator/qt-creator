// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ScriptEditorBackend

Column {
    id: root

    property int actionType

    property int horizontalSpacing
    property int columnWidth

    property var statement

    property var backend

    property string itemTooltip
    property alias methodTooltip: methodPopupLabel.tooltip
    property alias fromTooltip: fromPopupLabel.tooltip
    property alias toTooltip: toPopupLabel.tooltip
    property alias stateGroupTooltip: stateGroupPopupLabel.tooltip
    property alias stateTooltip: statePopupLabel.tooltip
    property alias propertyTooltip: propertyPopupLabel.tooltip
    property alias valueTooltip: valuePopupLabel.tooltip
    property alias messageTooltip: messagePopupLabel.tooltip

    // Call Function
    Row {
        visible: root.actionType === StatementDelegate.CallFunction
        spacing: root.horizontalSpacing

        HelperWidgets.PopupLabel {
            id: itemPopupLabel
            width: root.columnWidth
            text: qsTr("Item")
            tooltip: root.itemTooltip
        }

        HelperWidgets.PopupLabel {
            id: methodPopupLabel
            width: root.columnWidth
            text: qsTr("Method")
        }
    }

    Row {
        visible: root.actionType === StatementDelegate.CallFunction
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: functionId
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth

            model: root.statement.function.id.model ?? 0

            onActivated: backend.okStatement.function.id.activateIndex(functionId.currentIndex)
            property int currentTypeIndex: backend.okStatement.function.id.currentIndex ?? 0
            onCurrentTypeIndexChanged: functionId.currentIndex = functionId.currentTypeIndex
        }

        StudioControls.TopLevelComboBox {
            id: functionName
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: root.statement.function.name.model ?? 0

            onActivated: root.statement.function.name.activateIndex(functionName.currentIndex)
            property int currentTypeIndex: root.statement.function.name.currentIndex ?? 0
            onCurrentTypeIndexChanged: functionName.currentIndex = functionName.currentTypeIndex
        }
    }

    // Assign
    Row {
        visible: root.actionType === StatementDelegate.Assign
        spacing: root.horizontalSpacing

        HelperWidgets.PopupLabel {
            id: fromPopupLabel
            width: root.columnWidth
            text: qsTr("From")
        }
        HelperWidgets.PopupLabel {
            id: toPopupLabel
            width: root.columnWidth
            text: qsTr("To")
        }
    }

    Row {
        visible: root.actionType === StatementDelegate.Assign
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: rhsAssignmentId
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            //from - rhs - id

            model: root.statement.rhsAssignment.id.model ?? 0

            onActivated: root.statement.rhsAssignment.id.activateIndex(rhsAssignmentId.currentIndex)
            property int currentTypeIndex: root.statement.rhsAssignment.id.currentIndex ?? 0
            onCurrentTypeIndexChanged: rhsAssignmentId.currentIndex = rhsAssignmentId.currentTypeIndex
        }

        StudioControls.TopLevelComboBox {
            id: lhsAssignmentId
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            //to lhs - id
            model: root.statement.lhs.id.model ?? 0

            onActivated: root.statement.lhs.id.activateIndex(lhsAssignmentId.currentIndex)
            property int currentTypeIndex: root.statement.lhs.id.currentIndex ?? 0
            onCurrentTypeIndexChanged: lhsAssignmentId.currentIndex = lhsAssignmentId.currentTypeIndex
        }
    }

    Row {
        visible: root.actionType === StatementDelegate.Assign
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: rhsAssignmentName
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            //from - rhs - name

            model: root.statement.rhsAssignment.name.model ?? 0

            onActivated: root.statement.rhsAssignment.name.activateIndex(rhsAssignmentName.currentIndex)
            property int currentTypeIndex: root.statement.rhsAssignment.name.currentIndex ?? 0
            onCurrentTypeIndexChanged: rhsAssignmentName.currentIndex = rhsAssignmentName.currentTypeIndex
        }

        StudioControls.TopLevelComboBox {
            id: lhsAssignmentName
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            //to lhs - name
            model: root.statement.lhs.name.model ?? 0

            onActivated: root.statement.lhs.name.activateIndex(lhsAssignmentName.currentIndex)
            property int currentTypeIndex: root.statement.lhs.name.currentIndex ?? 0
            onCurrentTypeIndexChanged: lhsAssignmentName.currentIndex = lhsAssignmentName.currentTypeIndex
        }
    }

    // Change State
    Row {
        visible: root.actionType === StatementDelegate.ChangeState
        spacing: root.horizontalSpacing

        HelperWidgets.PopupLabel {
            id: stateGroupPopupLabel
            width: root.columnWidth
            text: qsTr("State Group")
        }

        HelperWidgets.PopupLabel {
            id: statePopupLabel
            width: root.columnWidth
            text: qsTr("State")
        }
    }

    Row {
        visible: root.actionType === StatementDelegate.ChangeState
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: stateGroups
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: root.statement.stateTargets.model ?? 0

            onActivated: root.statement.stateTargets.activateIndex(stateGroups.currentIndex)
            property int currentTypeIndex: root.statement.stateTargets.currentIndex ?? 0
            onCurrentTypeIndexChanged: stateGroups.currentIndex = stateGroups.currentTypeIndex
        }

        StudioControls.TopLevelComboBox {
            id: states
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth

            model: root.statement.states.model ?? 0

            onActivated: root.statement.states.activateIndex(states.currentIndex)
            property int currentTypeIndex: root.statement.states.currentIndex ?? 0
            onCurrentTypeIndexChanged: states.currentIndex = states.currentTypeIndex
        }
    }

    // Set Property
    Row {
        visible: root.actionType === StatementDelegate.SetProperty
        spacing: root.horizontalSpacing

        HelperWidgets.PopupLabel {
            width: root.columnWidth
            text: qsTr("Item")
            tooltip: root.itemTooltip
        }

        HelperWidgets.PopupLabel {
            id: propertyPopupLabel
            width: root.columnWidth
            text: qsTr("Property")
        }
    }

    Row {
        visible: root.actionType === StatementDelegate.SetProperty
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: lhsPropertyId
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth

            model: root.statement.lhs.id.model ?? 0

            onActivated: root.statement.lhs.id.activateIndex(lhsPropertyId.currentIndex)
            property int currentTypeIndex: root.statement.lhs.id.currentIndex ?? 0
            onCurrentTypeIndexChanged: lhsPropertyId.currentIndex = lhsPropertyId.currentTypeIndex

        }

        StudioControls.TopLevelComboBox {
            id: lhsPropertyName
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: root.statement.lhs.name.model ?? 0

            onActivated: root.statement.lhs.name.activateIndex(lhsPropertyName.currentIndex)
            property int currentTypeIndex: root.statement.lhs.name.currentIndex ?? 0
            onCurrentTypeIndexChanged: lhsPropertyName.currentIndex = lhsPropertyName.currentTypeIndex
        }
    }

    HelperWidgets.PopupLabel {
        id: valuePopupLabel
        width: root.columnWidth
        visible: root.actionType === StatementDelegate.SetProperty
        text: qsTr("Value")
    }

    StudioControls.TextField {
        id: setPropertyArgument
        visible: root.actionType === StatementDelegate.SetProperty
        width: root.width
        actionIndicatorVisible: false
        translationIndicatorVisible: false

        text: root.statement.stringArgument.text ?? ""
        onEditingFinished: {
            root.statement.stringArgument.activateText(setPropertyArgument.text)
        }
    }

    // Print Message
    HelperWidgets.PopupLabel {
        id: messagePopupLabel
        width: root.columnWidth
        visible: root.actionType === StatementDelegate.PrintMessage
        text: qsTr("Message")
    }

    StudioControls.TextField {
        id: messageString
        visible: root.actionType === StatementDelegate.PrintMessage
        width: root.width
        actionIndicatorVisible: false
        translationIndicatorVisible: false
        text: root.statement.stringArgument.text ?? ""
        onEditingFinished: {
            root.statement.stringArgument.activateText(messageString.text)
        }
    }

    // Custom
    HelperWidgets.PopupLabel {
        visible: root.actionType === StatementDelegate.Custom
        text: qsTr("Custom Connections can only be edited with the binding editor")
        width: root.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }
}
