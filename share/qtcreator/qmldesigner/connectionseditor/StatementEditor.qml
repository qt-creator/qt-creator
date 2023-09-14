// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ConnectionsEditorEditorBackend

Column {
    id: root

    property int actionType

    property int horizontalSpacing
    property int columnWidth

    property var statement

    //implicitWidth: Math.max(16, container.childrenRect.width + container.childrenRect.x)
    //implicitHeight: Math.max(16, container.childrenRect.height + container.childrenRect.y)

    // Call Function
    Row {
        visible: root.actionType === ConnectionModelStatementDelegate.CallFunction
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("Item") ; tooltip: qsTr("The target item of the function.")}
        PopupLabel { text: qsTr("Method") ; tooltip: qsTr("The name of the function.")}
    }

    Row {
        visible: root.actionType === ConnectionModelStatementDelegate.CallFunction
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
        visible: root.actionType === ConnectionModelStatementDelegate.Assign
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("From") ; tooltip: qsTr("The Property to assign from.")}
        PopupLabel { text: qsTr("To"); tooltip: qsTr("The Property to assign to.") }
    }

    Row {
        visible: root.actionType === ConnectionModelStatementDelegate.Assign
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
        visible: root.actionType === ConnectionModelStatementDelegate.Assign
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
        visible: root.actionType === ConnectionModelStatementDelegate.ChangeState
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("State Group"); tooltip: qsTr("The State Group.") }
        PopupLabel { text: qsTr("State"); tooltip: qsTr("The State .") }
    }

    Row {
        visible: root.actionType === ConnectionModelStatementDelegate.ChangeState
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
        visible: root.actionType === ConnectionModelStatementDelegate.SetProperty
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("Item"); tooltip: qsTr("The Item.")}
        PopupLabel { text: qsTr("Property"); tooltip: qsTr("The property of the item.")}
    }

    Row {
        visible: root.actionType === ConnectionModelStatementDelegate.SetProperty
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

    PopupLabel {
        visible: root.actionType === ConnectionModelStatementDelegate.SetProperty
        text: qsTr("Value")
    }

    StudioControls.TextField {
        id: setPropertyArgument
        visible: root.actionType === ConnectionModelStatementDelegate.SetProperty
        width: root.width
        actionIndicatorVisible: false
        translationIndicatorVisible: false

        text: root.statement.stringArgument.text ?? ""
        onEditingFinished: {
            root.statement.stringArgument.activateText(setPropertyArgument.text)
        }
    }

    // Print Message
    PopupLabel {
        visible: root.actionType === ConnectionModelStatementDelegate.PrintMessage
        text: qsTr("Message")
        tooltip: qsTr("The message that is printed.")
    }

    StudioControls.TextField {
        id: messageString
        visible: root.actionType === ConnectionModelStatementDelegate.PrintMessage
        width: root.width
        actionIndicatorVisible: false
        translationIndicatorVisible: false
        text: root.statement.stringArgument.text ?? ""
        onEditingFinished: {
            root.statement.stringArgument.activateText(messageString.text)
        }
    }

    // Custom
    PopupLabel {
        visible: root.actionType === ConnectionModelStatementDelegate.Custom
        text: qsTr("Custom Connections can only be edited with the binding editor")
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 30
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }
}
