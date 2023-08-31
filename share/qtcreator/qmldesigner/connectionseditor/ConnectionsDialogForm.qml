// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Column {
    id: root

    readonly property real horizontalSpacing: 10
    readonly property real verticalSpacing: 16
    readonly property real columnWidth: (root.width - root.horizontalSpacing) / 2

    property var backend


    /* replaced by ConnectionModelStatementDelegate defined in C++
    enum ActionType {
        CallFunction,
        Assign,
        ChangeState,
        SetProperty,
        PrintMessage,
        Custom
    } */

    y: StudioTheme.Values.popupMargin
    width: parent.width
    spacing: root.verticalSpacing

    Row {
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("Signal"); tooltip: qsTr("The name of the signal.") }
        PopupLabel { text: qsTr("Action"); tooltip: qsTr("The type of the action.") }
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

        StudioControls.TopLevelComboBox {
            id: action
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            textRole: "text"
            valueRole: "value"
            ///model.getData(currentIndex, "role")
            property int indexFromBackend: indexOfValue(backend.actionType)
            onIndexFromBackendChanged: action.currentIndex = action.indexFromBackend
            onActivated: backend.changeActionType(action.currentValue)

            model: [
                { value: ConnectionModelStatementDelegate.CallFunction, text: qsTr("Call Function") },
                { value: ConnectionModelStatementDelegate.Assign, text: qsTr("Assign") },
                { value: ConnectionModelStatementDelegate.ChangeState, text: qsTr("Change State") },
                { value: ConnectionModelStatementDelegate.SetProperty, text: qsTr("Set Property") },
                { value: ConnectionModelStatementDelegate.PrintMessage, text: qsTr("Print Message") },
                { value: ConnectionModelStatementDelegate.Custom, text: qsTr("Unknown") }
            ]
        }
    }

    // Call Function
    Row {
        visible: action.currentValue === ConnectionModelStatementDelegate.CallFunction
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("Item") ; tooltip: qsTr("The target item of the function.")}
        PopupLabel { text: qsTr("Method") ; tooltip: qsTr("The name of the function.")}
    }

    Row {
        visible: action.currentValue === ConnectionModelStatementDelegate.CallFunction
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: functionId
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth

            model: backend.okStatement.function.id.model ?? 0

            onActivated: backend.okStatement.function.id.activateIndex(functionId.currentIndex)
            property int currentTypeIndex: backend.okStatement.function.id.currentIndex ?? 0
            onCurrentTypeIndexChanged: functionId.currentIndex = functionId.currentTypeIndex
        }

        StudioControls.TopLevelComboBox {
            id: functionName
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: backend.okStatement.function.name.model ?? 0

            onActivated: backend.okStatement.function.name.activateIndex(functionName.currentIndex)
            property int currentTypeIndex: backend.okStatement.function.name.currentIndex ?? 0
            onCurrentTypeIndexChanged: functionName.currentIndex = functionName.currentTypeIndex
        }
    }

    // Assign
    Row {
        visible: action.currentValue === ConnectionModelStatementDelegate.Assign
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("From") ; tooltip: qsTr("The Property to assign from.")}
        PopupLabel { text: qsTr("To"); tooltip: qsTr("The Property to assign to.") }
    }

    Row {
        visible: action.currentValue === ConnectionModelStatementDelegate.Assign
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: rhsAssignmentId
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            //from - rhs - id

            model: backend.okStatement.rhsAssignment.id.model ?? 0

            onActivated: backend.okStatement.rhsAssignment.id.activateIndex(rhsAssignmentId.currentIndex)
            property int currentTypeIndex: backend.okStatement.rhsAssignment.id.currentIndex ?? 0
            onCurrentTypeIndexChanged: rhsAssignmentId.currentIndex = rhsAssignmentId.currentTypeIndex
        }

        StudioControls.TopLevelComboBox {
            id: lhsAssignmentId
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            //to lhs - id
            model: backend.okStatement.lhs.id.model ?? 0

            onActivated: backend.okStatement.lhs.id.activateIndex(lhsAssignmentId.currentIndex)
            property int currentTypeIndex: backend.okStatement.lhs.id.currentIndex ?? 0
            onCurrentTypeIndexChanged: lhsAssignmentId.currentIndex = lhsAssignmentId.currentTypeIndex
        }
    }

    Row {
        visible: action.currentValue === ConnectionModelStatementDelegate.Assign
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: rhsAssignmentName
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            //from - rhs - name

            model: backend.okStatement.rhsAssignment.name.model ?? 0

            onActivated: backend.okStatement.rhsAssignment.name.activateIndex(rhsAssignmentName.currentIndex)
            property int currentTypeIndex: backend.okStatement.rhsAssignment.name.currentIndex ?? 0
            onCurrentTypeIndexChanged: rhsAssignmentName.currentIndex = rhsAssignmentName.currentTypeIndex
        }

        StudioControls.TopLevelComboBox {
            id: lhsAssignmentName
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            //to lhs - name
            model: backend.okStatement.lhs.name.model ?? 0

            onActivated: backend.okStatement.lhs.name.activateIndex(lhsAssignmentName.currentIndex)
            property int currentTypeIndex: backend.okStatement.lhs.name.currentIndex ?? 0
            onCurrentTypeIndexChanged: lhsAssignmentName.currentIndex = lhsAssignmentName.currentTypeIndex
        }
    }

    // Change State
    Row {
        visible: action.currentValue === ConnectionModelStatementDelegate.ChangeState
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("State Group"); tooltip: qsTr("The State Group.") }
        PopupLabel { text: qsTr("State"); tooltip: qsTr("The State .") }
    }

    Row {
        visible: action.currentValue === ConnectionModelStatementDelegate.ChangeState
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: stateGroups
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: backend.okStatement.stateTargets.model ?? 0

            onActivated: backend.okStatement.stateTargets.activateIndex(stateGroups.currentIndex)
            property int currentTypeIndex: backend.okStatement.stateTargets.currentIndex ?? 0
            onCurrentTypeIndexChanged: stateGroups.currentIndex = stateGroups.currentTypeIndex
        }

        StudioControls.TopLevelComboBox {
            id: states
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth

            model: backend.okStatement.states.model ?? 0

            onActivated: backend.okStatement.states.activateIndex(states.currentIndex)
            property int currentTypeIndex: backend.okStatement.states.currentIndex ?? 0
            onCurrentTypeIndexChanged: states.currentIndex = states.currentTypeIndex
        }
    }

    // Set Property
    Row {
        visible: action.currentValue === ConnectionModelStatementDelegate.SetProperty
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("Item"); tooltip: qsTr("The Item.")}
        PopupLabel { text: qsTr("Property"); tooltip: qsTr("The property of the item.")}
    }

    Row {
        visible: action.currentValue === ConnectionModelStatementDelegate.SetProperty
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: lhsPropertyId
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth

            model: backend.okStatement.lhs.id.model ?? 0

            onActivated: backend.okStatement.lhs.id.activateIndex(lhsPropertyId.currentIndex)
            property int currentTypeIndex: backend.okStatement.lhs.id.currentIndex ?? 0
            onCurrentTypeIndexChanged: lhsPropertyId.currentIndex = lhsPropertyId.currentTypeIndex

        }

        StudioControls.TopLevelComboBox {
            id: lhsPropertyName
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: backend.okStatement.lhs.name.model ?? 0

            onActivated: backend.okStatement.lhs.name.activateIndex(lhsPropertyName.currentIndex)
            property int currentTypeIndex: backend.okStatement.lhs.name.currentIndex ?? 0
            onCurrentTypeIndexChanged: lhsPropertyName.currentIndex = lhsPropertyName.currentTypeIndex
        }
    }

    PopupLabel {
        visible: action.currentValue === ConnectionModelStatementDelegate.SetProperty
        text: qsTr("Value")
    }

    StudioControls.TextField {
        id: setPropertyArgument
        visible: action.currentValue === ConnectionModelStatementDelegate.SetProperty
        width: root.width
        actionIndicatorVisible: false
        translationIndicatorVisible: false

        text: backend.okStatement.stringArgument.text ?? ""
        onEditingFinished: {
            backend.okStatement.stringArgument.activateText(setPropertyArgument.text)
        }
    }

    // Print Message
    PopupLabel {
        visible: action.currentValue === ConnectionModelStatementDelegate.PrintMessage
        text: qsTr("Message")
        tooltip: qsTr("The message that is printed.")
    }

    StudioControls.TextField {
        id: messageString
        visible: action.currentValue === ConnectionModelStatementDelegate.PrintMessage
        width: root.width
        actionIndicatorVisible: false
        translationIndicatorVisible: false
        text: backend.okStatement.stringArgument.text ?? ""
        onEditingFinished: {
            backend.okStatement.stringArgument.activateText(messageString.text)
        }
    }

    // Custom
    PopupLabel {
        visible: action.currentValue === ConnectionModelStatementDelegate.Custom
        text: qsTr("Custom Connections can only be edited with the binding editor")
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 30
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }

    HelperWidgets.AbstractButton {
        style: StudioTheme.Values.connectionPopupButtonStyle
        width: 160
        buttonIcon: qsTr("Add Condition")
        iconSize: StudioTheme.Values.baseFontSize
        iconFont: StudioTheme.Constants.font
        anchors.horizontalCenter: parent.horizontalCenter
        visible: action.currentValue !== ConnectionModelStatementDelegate.Custom && !backend.hasCondition

        onClicked: backend.addCondition()
    }

        HelperWidgets.AbstractButton {
        style: StudioTheme.Values.connectionPopupButtonStyle
        width: 160
        buttonIcon: qsTr("Remove Condition")
        iconSize: StudioTheme.Values.baseFontSize
        iconFont: StudioTheme.Constants.font
        anchors.horizontalCenter: parent.horizontalCenter
        visible: action.currentValue !== ConnectionModelStatementDelegate.Custom && backend.hasCondition

        onClicked: backend.removeCondition()
    }

    Flow {
        spacing: root.horizontalSpacing
        width: root.width
        Repeater {

            model: backend.conditionListModel
            Text {
                text: value
                color: "white"
                Rectangle {
                    z: -1
                    opacity: 0.2
                    color: {
                        if (type === ConditionListModel.Invalid)
                           return "red"
                        if (type === ConditionListModel.Operator)
                           return "blue"
                        if (type === ConditionListModel.Literal)
                            return "green"
                        if (type === ConditionListModel.Variable)
                            return "yellow"
                    }
                    anchors.fill: parent
                }
            }
        }
    }

    TextInput {
        id: commandInput
        width: root.width
        onAccepted:  backend.conditionListModel.command(commandInput.text)
    }

    Text {
        text: "invalid " + backend.conditionListModel.error
        visible: !backend.conditionListModel.valid
    }

    // Editor
    Rectangle {
        id: editor
        width: parent.width
        height: 150
        color: StudioTheme.Values.themeToolbarBackground

        Text {
            anchors.centerIn: parent
            text: backend.source
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.myFontSize
        }

        HelperWidgets.AbstractButton {
            id: editorButton

            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 4

            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.edit_medium
            tooltip: qsTr("Add something.")
            onClicked: console.log("OPEN EDITOR")
        }
    }
}
