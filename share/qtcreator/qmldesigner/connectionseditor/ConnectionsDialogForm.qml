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
    readonly property real verticalSpacing: 12
    readonly property real columnWidth: (root.width - root.horizontalSpacing) / 2

    property var backend

    width: parent.width
    spacing: root.verticalSpacing

    TapHandler {
        onTapped: root.forceActiveFocus()
    }

    Row {
        spacing: root.horizontalSpacing

        PopupLabel {
            width: root.columnWidth
            text: qsTr("Signal")
            tooltip: qsTr("Sets an interaction method that connects to the <b>Target</b> component.")
        }

        PopupLabel {
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

            model: ListModel {
                ListElement {
                    value: ConnectionModelStatementDelegate.CallFunction
                    text: qsTr("Call Function")
                    enabled: true
                }
                ListElement {
                    value: ConnectionModelStatementDelegate.Assign
                    text: qsTr("Assign")
                    enabled: true
                }
                ListElement {
                    value: ConnectionModelStatementDelegate.ChangeState
                    text: qsTr("Change State")
                    enabled: true
                }
                ListElement {
                    value: ConnectionModelStatementDelegate.SetProperty
                    text: qsTr("Set Property")
                    enabled: true
                }
                ListElement {
                    value: ConnectionModelStatementDelegate.PrintMessage
                    text: qsTr("Print Message")
                    enabled: true
                }
                ListElement {
                    value: ConnectionModelStatementDelegate.Custom
                    text: qsTr("Custom")
                    enabled: false
                }
            }
        }
    }

    StatementEditor {
        width: root.width
        actionType: action.currentValue ?? ConnectionModelStatementDelegate.Custom
        horizontalSpacing: root.horizontalSpacing
        columnWidth: root.columnWidth
        statement: backend.okStatement
        backend: root.backend
        spacing: root.verticalSpacing
    }

    HelperWidgets.AbstractButton {
        style: StudioTheme.Values.connectionPopupButtonStyle
        width: 160
        buttonIcon: qsTr("Add Condition")
        tooltip: qsTr("Sets a logical condition for the selected <b>Signal</b>. It works with the properties of the <b>Target</b> component.")
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
        tooltip: qsTr("Removes the logical condition for the <b>Target</b> component.")
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
        tooltip: qsTr("Sets an alternate condition for the previously defined logical condition.")
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
        tooltip: qsTr("Removes the alternate logical condition for the previously defined logical condition.")
        iconSize: StudioTheme.Values.baseFontSize
        iconFont: StudioTheme.Constants.font
        anchors.horizontalCenter: parent.horizontalCenter
        visible: action.currentValue !== ConnectionModelStatementDelegate.Custom
                 && backend.hasCondition && backend.hasElse

        onClicked: backend.removeElse()
    }

    //Else Statement
    StatementEditor {
        width: root.width
        actionType: action.currentValue ?? ConnectionModelStatementDelegate.Custom
        horizontalSpacing: root.horizontalSpacing
        columnWidth: root.columnWidth
        statement: backend.koStatement
        backend: root.backend
        spacing: root.verticalSpacing
        visible: action.currentValue !== ConnectionModelStatementDelegate.Custom
                 && backend.hasCondition && backend.hasElse
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
                    tooltip: qsTr("Write the conditions for the components and the signals manually.")
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

                    ActionEditor {
                        id: bindingEditor

                        onRejected: {
                            hideWidget()
                            expressionDialogLoader.visible = false
                        }

                        onAccepted: {
                            backend.setNewSource(bindingEditor.text)
                            hideWidget()
                            expressionDialogLoader.visible = false
                        }
                    }
                }
            } // loader
        } // rect
    } //col 2
}//col1

