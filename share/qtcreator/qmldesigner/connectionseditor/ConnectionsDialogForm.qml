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

    y: StudioTheme.Values.popupMargin
    width: parent.width
    spacing: root.verticalSpacing

    TapHandler {
        onTapped: root.forceActiveFocus()
    }

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

    StatementEditor {
        actionType: action.currentValue ?? ConnectionModelStatementDelegate.Custom
        horizontalSpacing: root.horizontalSpacing
        columnWidth: root.columnWidth
        statement: backend.okStatement
        spacing: root.verticalSpacing
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

    ExpressionBuilder {
        style: StudioTheme.Values.connectionPopupControlStyle
        width: root.width

        visible: backend.hasCondition
        model: backend.conditionListModel

        onRemove: function(index) {
            //console.log("remove", index)
            backend.conditionListModel.removeToken(index)
        }

        onUpdate: function(index, value) {
            //console.log("update", index, value)
            backend.conditionListModel.updateToken(index, value)
        }

        onAdd: function(value) {
            //console.log("add", value)
            backend.conditionListModel.appendToken(value)
        }

        onInsert: function(index, value, type) {
            //console.log("insert", index, value, type)
            if (type === ConditionListModel.Intermediate)
                backend.conditionListModel.insertIntermediateToken(index, value)
            else if (type === ConditionListModel.Shadow)
                backend.conditionListModel.insertShadowToken(index, value)
            else
                backend.conditionListModel.insertToken(index, value)
        }

        onSetValue: function(index, value) {
            //console.log("setValue", index, value)
            backend.conditionListModel.setShadowToken(index, value)
        }
    }

    HelperWidgets.AbstractButton {
        style: StudioTheme.Values.connectionPopupButtonStyle
        width: 160
        buttonIcon: qsTr("Add Else Statement")
        iconSize: StudioTheme.Values.baseFontSize
        iconFont: StudioTheme.Constants.font
        anchors.horizontalCenter: parent.horizontalCenter
        visible: action.currentValue !== ConnectionModelStatementDelegate.Custom
                 && backend.hasCondition && !backend.hasElse

        onClicked: backend.addElse()
    }

    HelperWidgets.AbstractButton {
        style: StudioTheme.Values.connectionPopupButtonStyle
        width: 160
        buttonIcon: qsTr("Remove Else Statement")
        iconSize: StudioTheme.Values.baseFontSize
        iconFont: StudioTheme.Constants.font
        anchors.horizontalCenter: parent.horizontalCenter
        visible: action.currentValue !== ConnectionModelStatementDelegate.Custom
                 && backend.hasCondition && backend.hasElse

        onClicked: backend.removeElse()
    }

    //Else Statement
    StatementEditor {
        actionType: action.currentValue ?? ConnectionModelStatementDelegate.Custom
        horizontalSpacing: root.horizontalSpacing
        columnWidth: root.columnWidth
        statement: backend.koStatement
        spacing: root.verticalSpacing
        visible: action.currentValue !== ConnectionModelStatementDelegate.Custom
                 && backend.hasCondition && backend.hasElse
    }

    // Editor
    Rectangle {
        id: editor
        width: parent.width
        height: 150
        color: StudioTheme.Values.themeToolbarBackground

        Text {
            id: code
            anchors.fill: parent
            anchors.margins: 4
            text: backend.source
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.myFontSize
            wrapMode: Text.WordWrap
            horizontalAlignment: code.lineCount === 1 ? Text.AlignHCenter : Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
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
