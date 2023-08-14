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

    component PopupLabel : Text {
        width: root.columnWidth
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.myFontSize
    }

    enum ActionType {
        CallFunction,
        Assign,
        ChangeState,
        SetProperty,
        PrintMessage,
        Custom
    }

    y: StudioTheme.Values.popupMargin
    width: parent.width
    spacing: root.verticalSpacing

    Row {
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("Signal") }
        PopupLabel { text: qsTr("Action") }
    }

    Row {
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            id: signal
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: ["Clicked", "Pressed", "Released"]
        }

        StudioControls.TopLevelComboBox {
            id: action
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            textRole: "text"
            valueRole: "value"

            model: [
                { value: ConnectionsDialogForm.CallFunction, text: qsTr("Call Function") },
                { value: ConnectionsDialogForm.Assign, text: qsTr("Assign") },
                { value: ConnectionsDialogForm.ChangeState, text: qsTr("Change State") },
                { value: ConnectionsDialogForm.SetProperty, text: qsTr("Set Property") },
                { value: ConnectionsDialogForm.PrintMessage, text: qsTr("Print Message") },
                { value: ConnectionsDialogForm.Custom, text: qsTr("Custom") }
            ]
        }
    }

    // Call Function
    Row {
        visible: action.currentValue === ConnectionsDialogForm.CallFunction
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("Item") }
        PopupLabel { text: qsTr("Method") }
    }

    Row {
        visible: action.currentValue === ConnectionsDialogForm.CallFunction
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: ["mySpinBox", "myAnimation", "myCustomComponent"]
        }

        StudioControls.TopLevelComboBox {
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: ["start", "stop", "reset"]
        }
    }

    // Assign
    Row {
        visible: action.currentValue === ConnectionsDialogForm.Assign
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("From") }
        PopupLabel { text: qsTr("To") }
    }

    Row {
        visible: action.currentValue === ConnectionsDialogForm.Assign
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: ["value", "foo", "bar"]
        }

        StudioControls.TopLevelComboBox {
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: ["myValue", "yourValue", "ourValue"]
        }
    }

    // Change State
    Row {
        visible: action.currentValue === ConnectionsDialogForm.ChangeState
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("State Group") }
        PopupLabel { text: qsTr("State") }
    }

    Row {
        visible: action.currentValue === ConnectionsDialogForm.ChangeState
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: ["main", "group1", "group2"]
        }

        StudioControls.TopLevelComboBox {
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: ["state1", "state2", "state3", "state4"]
        }
    }

    // Set Property
    Row {
        visible: action.currentValue === ConnectionsDialogForm.SetProperty
        spacing: root.horizontalSpacing

        PopupLabel { text: qsTr("Item") }
        PopupLabel { text: qsTr("Property") }
    }

    Row {
        visible: action.currentValue === ConnectionsDialogForm.SetProperty
        spacing: root.horizontalSpacing

        StudioControls.TopLevelComboBox {
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: ["value", "foo", "bar"]
        }

        StudioControls.TopLevelComboBox {
            style: StudioTheme.Values.connectionPopupControlStyle
            width: root.columnWidth
            model: ["myValue", "yourValue", "ourValue"]
        }
    }

    PopupLabel {
        visible: action.currentValue === ConnectionsDialogForm.SetProperty
        text: qsTr("Value")
    }

    StudioControls.TextField {
        visible: action.currentValue === ConnectionsDialogForm.SetProperty
        width: root.width
        text: "This is a test"
        actionIndicatorVisible: false
        translationIndicatorVisible: false
    }

    // Print Message
    PopupLabel {
        visible: action.currentValue === ConnectionsDialogForm.PrintMessage
        text: qsTr("Message")
    }

    StudioControls.TextField {
        visible: action.currentValue === ConnectionsDialogForm.PrintMessage
        width: root.width
        text: "my value is"
        actionIndicatorVisible: false
        translationIndicatorVisible: false
    }

    // Custom
    PopupLabel {
        visible: action.currentValue === ConnectionsDialogForm.Custom
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
        visible: action.currentValue !== ConnectionsDialogForm.Custom

        onClicked: console.log("ADD CONDITION")
    }

    // Editor
    Rectangle {
        id: editor
        width: parent.width
        height: 150
        color: StudioTheme.Values.themeToolbarBackground

        Text {
            anchors.centerIn: parent
            text: "backend.myValue = mySpinbox.value"
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
