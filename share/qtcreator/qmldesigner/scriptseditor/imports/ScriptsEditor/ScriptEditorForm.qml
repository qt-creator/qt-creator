// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import ScriptEditorBackend

Column {
    id: root

    property real horizontalSpacing
    property real verticalSpacing
    property real columnWidth

    property var backend

    property alias keepOpen: expressionDialogLoader.visible

    property int currentAction: StatementDelegate.Custom

    property string itemTooltip: qsTr("Sets the component that is affected by the action.")
    property string methodTooltip: qsTr("Sets the item component's method.")
    property string fromTooltip: qsTr("Sets the component and its property from which the value is copied.")
    property string toTooltip: qsTr("Sets the component and its property to which the copied value is assigned.")
    property string addConditionTooltip: qsTr("Sets a logical condition for the selected action.")
    property string removeConditionTooltip: qsTr("Removes the logical condition for the action.")
    property string stateGroupTooltip: qsTr("Sets a <b>State Group</b> that is accessed when the action is initiated.")
    property string stateTooltip: qsTr("Sets a <b>State</b> within the assigned <b>State Group</b> that is accessed when the action is initiated.")
    property string propertyTooltip: qsTr("Sets the property of the component that is affected by the action.")
    property string valueTooltip: qsTr("Sets the value of the property of the component that is affected by the action.")
    property string messageTooltip: qsTr("Sets a text that is printed when the action is initiated.")


    StatementEditor {
        width: root.width
        actionType: root.currentAction
        horizontalSpacing: root.horizontalSpacing
        columnWidth: root.columnWidth
        statement: root.backend.okStatement
        backend: root.backend
        spacing: root.verticalSpacing
        itemTooltip: root.itemTooltip
        methodTooltip: root.methodTooltip
        fromTooltip: root.fromTooltip
        toTooltip: root.toTooltip
        stateGroupTooltip: root.stateGroupTooltip
        stateTooltip: root.stateTooltip
        propertyTooltip: root.propertyTooltip
        valueTooltip: root.valueTooltip
        messageTooltip: root.messageTooltip
    }


    HelperWidgets.AbstractButton {
        id: addConditionButton
        style: StudioTheme.Values.connectionPopupButtonStyle
        width: 160
        buttonIcon: qsTr("Add Condition")
        tooltip: root.addConditionTooltip
        iconSize: StudioTheme.Values.baseFontSize
        iconFontFamily: StudioTheme.Constants.font.family
        anchors.horizontalCenter: parent.horizontalCenter
        visible: root.currentAction !== StatementDelegate.Custom && !backend.hasCondition

        onClicked: backend.addCondition()
    }

    HelperWidgets.AbstractButton {
        style: StudioTheme.Values.connectionPopupButtonStyle
        width: 160
        buttonIcon: qsTr("Remove Condition")
        tooltip: root.removeConditionTooltip
        iconSize: StudioTheme.Values.baseFontSize
        iconFontFamily: StudioTheme.Constants.font.family
        anchors.horizontalCenter: parent.horizontalCenter
        visible: root.currentAction !== StatementDelegate.Custom && backend.hasCondition

        onClicked: backend.removeCondition()
    }

    ExpressionBuilder {
        style: StudioTheme.Values.connectionPopupControlStyle
        width: root.width

        visible: backend.hasCondition
        model: backend.conditionListModel
        conditionListModel: backend.conditionListModel
        popupListModel: backend.propertyListProxyModel
        popupTreeModel: backend.propertyTreeModel

        onRemove: function(index) {
            backend.conditionListModel.removeToken(index)
        }

        onUpdate: function(index, value) {
            backend.conditionListModel.updateToken(index, value)
        }

        onAdd: function(value) {
            backend.conditionListModel.appendToken(value)
        }

        onInsert: function(index, value, type) {
            if (type === ConditionListModel.Intermediate)
                backend.conditionListModel.insertIntermediateToken(index, value)
            else if (type === ConditionListModel.Shadow)
                backend.conditionListModel.insertShadowToken(index, value)
            else
                backend.conditionListModel.insertToken(index, value)
        }

        onSetValue: function(index, value) {
            backend.conditionListModel.setShadowToken(index, value)
        }
    }

    HelperWidgets.AbstractButton {
        style: StudioTheme.Values.connectionPopupButtonStyle
        width: 160
        buttonIcon: qsTr("Add Else Statement")
        tooltip: qsTr("Sets an alternate condition for the previously defined logical condition.")
        iconSize: StudioTheme.Values.baseFontSize
        iconFontFamily: StudioTheme.Constants.font.family
        anchors.horizontalCenter: parent.horizontalCenter
        visible: root.currentAcion !== StatementDelegate.Custom
                 && backend.hasCondition && !backend.hasElse

        onClicked: backend.addElse()
    }

    HelperWidgets.AbstractButton {
        style: StudioTheme.Values.connectionPopupButtonStyle
        width: 160
        buttonIcon: qsTr("Remove Else Statement")
        tooltip: qsTr("Removes the alternate logical condition for the previously defined logical condition.")
        iconSize: StudioTheme.Values.baseFontSize
        iconFontFamily: StudioTheme.Constants.font.family
        anchors.horizontalCenter: parent.horizontalCenter
        visible: root.currentAction !== StatementDelegate.Custom
                 && backend.hasCondition && backend.hasElse

        onClicked: backend.removeElse()
    }

    //Else Statement
    StatementEditor {
        width: root.width
        actionType: root.currentAction
        horizontalSpacing: root.horizontalSpacing
        columnWidth: root.columnWidth
        statement: backend.koStatement
        backend: root.backend
        spacing: root.verticalSpacing
        visible: action.currentValue !== StatementDelegate.Custom
                 && backend.hasCondition && backend.hasElse
        itemTooltip: root.itemTooltip
        methodTooltip: root.methodTooltip
        fromTooltip: root.fromTooltip
        toTooltip: root.toTooltip
        stateGroupTooltip: root.stateGroupTooltip
        stateTooltip: root.stateTooltip
        propertyTooltip: root.propertyTooltip
        valueTooltip: root.valueTooltip
        messageTooltip: root.messageTooltip
    }

    // code preview toolbar
    Column {
        id: miniToolbarEditor
        width: parent.width
        spacing: -2

        Rectangle {
            id: miniToolbar
            width: parent.width
            height: editorButton.height + 2
            radius: 4
            z: -1
            color: StudioTheme.Values.themeConnectionEditorMicroToolbar

            Row {
                spacing: 2
                HelperWidgets.AbstractButton {
                    id: editorButton
                    style: StudioTheme.Values.microToolbarButtonStyle
                    buttonIcon: StudioTheme.Constants.codeEditor_medium
                    tooltip: qsTr("Write the conditions manually.")
                    onClicked: expressionDialogLoader.show()
                }
                HelperWidgets.AbstractButton {
                    id: jumpToCodeButton
                    style: StudioTheme.Values.microToolbarButtonStyle
                    buttonIcon: StudioTheme.Constants.jumpToCode_medium
                    tooltip: qsTr("Jump to the code.")
                    onClicked: backend.jumpToCode()
                }
            }
        }

        // Editor
        Rectangle {
            id: editor
            width: parent.width
            height: 150
            color: StudioTheme.Values.themeConnectionCodeEditor

            Text {
                id: code
                anchors.fill: parent
                anchors.margins: 4
                text: backend.indentedSource
                color: StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.myFontSize
                wrapMode: Text.Wrap
                horizontalAlignment: code.lineCount === 1 ? Text.AlignHCenter : Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            Loader {
                id: expressionDialogLoader
                parent: editor
                anchors.fill: parent
                visible: false
                active: visible

                function show() {
                    expressionDialogLoader.visible = true
                }

                sourceComponent: Item {
                    id: bindingEditorParent

                    Component.onCompleted: {
                        bindingEditor.showWidget()
                        bindingEditor.text = backend.source
                        bindingEditor.showControls(false)
                        bindingEditor.setMultilne(true)
                        bindingEditor.updateWindowName()
                    }

                    HelperWidgets.ActionEditor {
                        id: bindingEditor

                        onRejected: {
                            bindingEditor.hideWidget()
                            expressionDialogLoader.visible = false
                        }

                        onAccepted: {
                            backend.setNewSource(bindingEditor.text)
                            bindingEditor.hideWidget()
                            expressionDialogLoader.visible = false
                        }
                    }
                }
            }
        }
    }
}
