// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Text Input")

    property bool isTextInput: false

    function isBackendValueAvailable(name) {
        if (backendValues[name] !== undefined)
            return backendValues[name].isAvailable

        return false
    }

    SectionLayout {
        PropertyLabel {
            text: qsTr("Selection color")
            tooltip: qsTr("Sets the background color of selected text.")
            blockedByTemplate: !root.isBackendValueAvailable("selectionColor")
        }

        ColorEditor {
            backendValue: backendValues.selectionColor
            supportGradient: false
            enabled: root.isBackendValueAvailable("selectionColor")
        }

        PropertyLabel {
            text: qsTr("Selected text color")
            tooltip: qsTr("Sets the color of selected text.")
            blockedByTemplate: !root.isBackendValueAvailable("selectedTextColor")
        }

        ColorEditor {
            backendValue: backendValues.selectedTextColor
            supportGradient: false
            enabled: root.isBackendValueAvailable("selectedTextColor")
        }

        PropertyLabel {
            text: qsTr("Selection mode")
            tooltip: qsTr("Sets the way text is selected with the mouse.")
            blockedByTemplate: !root.isBackendValueAvailable("mouseSelectionMode")
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.mouseSelectionMode
                scope: root.isTextInput ? "TextInput" : "TextEdit"
                model: ["SelectCharacters", "SelectWords"]
                enabled: root.isBackendValueAvailable("mouseSelectionMode")
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Input mask")
            tooltip: qsTr("Sets the allowed characters.")
            blockedByTemplate: !root.isBackendValueAvailable("inputMask")
        }

        SecondColumnLayout {
            visible: root.isTextInput

            LineEdit {
                backendValue: backendValues.inputMask
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                showTranslateCheckBox: false
                enabled: root.isBackendValueAvailable("inputMask")
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Echo mode")
            tooltip: qsTr("Sets the visibility mode.")
            blockedByTemplate: !root.isBackendValueAvailable("echoMode")
        }

        SecondColumnLayout {
            visible: root.isTextInput

            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.echoMode
                scope: "TextInput"
                model: ["Normal", "Password", "PasswordEchoOnEdit", "NoEcho"]
                enabled: root.isBackendValueAvailable("echoMode")
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Password character")
            tooltip: qsTr("Sets which character to display when passwords are entered.")
            blockedByTemplate: !root.isBackendValueAvailable("passwordCharacter")
        }

        SecondColumnLayout {
            visible: root.isTextInput

            LineEdit {
                backendValue: backendValues.passwordCharacter
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                showTranslateCheckBox: false
                enabled: root.isBackendValueAvailable("passwordCharacter")
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: !root.isTextInput
            text: qsTr("Tab stop distance")
            tooltip: qsTr("Default distance between tab stops in device units.")
            blockedByTemplate: !root.isBackendValueAvailable("tabStopDistance")
        }

        SecondColumnLayout {
            visible: !root.isTextInput

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.tabStopDistance
                maximumValue: 200
                minimumValue: 0
                enabled: root.isBackendValueAvailable("tabStopDistance")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "px" }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: !root.isTextInput
            text: qsTr("Text margin")
            tooltip: qsTr("Margin around the text in the Text Edit in pixels.")
            blockedByTemplate: !root.isBackendValueAvailable("textMargin")
        }

        SecondColumnLayout {
            visible: !root.isTextInput

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.textMargin
                maximumValue: 200
                minimumValue: -200
                enabled: root.isBackendValueAvailable("textMargin")
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "px" }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Maximum length")
            tooltip: qsTr("Sets the maximum length of the text.")
            blockedByTemplate: !root.isBackendValueAvailable("maximumLength")
        }

        SecondColumnLayout {
            visible: root.isTextInput

            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.maximumLength
                minimumValue: 0
                maximumValue: 32767
                enabled: root.isBackendValueAvailable("maximumLength")
            }

            ExpandingSpacer {}
        }

        component FlagItem : SecondColumnLayout {
            property alias backendValue: checkBox.backendValue

            CheckBox {
                id: checkBox
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                text: (checkBox.backendValue === undefined) ? "" : checkBox.backendValue.valueToString
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Read only")
            tooltip: qsTr("Toggles if the text allows edits.")
            blockedByTemplate: !root.isBackendValueAvailable("readOnly")
        }

        FlagItem {
            backendValue: backendValues.readOnly
            enabled: root.isBackendValueAvailable("readOnly")
        }

        PropertyLabel {
            text: qsTr("Cursor visible")
            tooltip: qsTr("Toggles if the cursor is visible.")
            blockedByTemplate: !root.isBackendValueAvailable("cursorVisible")
        }

        FlagItem {
            backendValue: backendValues.cursorVisible
            enabled: root.isBackendValueAvailable("cursorVisible")
        }

        PropertyLabel {
            text: qsTr("Focus on press")
            tooltip: qsTr("Toggles if the text is focused on mouse click.")
            blockedByTemplate: !root.isBackendValueAvailable("activeFocusOnPress")
        }

        FlagItem {
            backendValue: backendValues.activeFocusOnPress
            enabled: root.isBackendValueAvailable("activeFocusOnPress")
        }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Auto scroll")
            tooltip: qsTr("Toggles if the text scrolls when it exceeds its boundary.")
            blockedByTemplate: !root.isBackendValueAvailable("autoScroll")
        }

        FlagItem {
            visible: root.isTextInput
            backendValue: backendValues.autoScroll
            enabled: root.isBackendValueAvailable("autoScroll")
        }

        PropertyLabel {
            text: qsTr("Overwrite mode")
            tooltip: qsTr("Toggles if overwriting text is allowed.")
            blockedByTemplate: !root.isBackendValueAvailable("overwriteMode")
        }

        FlagItem {
            backendValue: backendValues.overwriteMode
            enabled: root.isBackendValueAvailable("overwriteMode")
        }

        PropertyLabel {
            text: qsTr("Persistent selection")
            tooltip: qsTr("Toggles if the text should remain selected after moving the focus elsewhere.")
            blockedByTemplate: !root.isBackendValueAvailable("persistentSelection")
        }

        FlagItem {
            backendValue: backendValues.persistentSelection
            enabled: root.isBackendValueAvailable("persistentSelection")
        }

        PropertyLabel {
            text: qsTr("Select by mouse")
            tooltip: qsTr("Toggles if the text can be selected with the mouse.")
            blockedByTemplate: !root.isBackendValueAvailable("selectByMouse")
        }

        FlagItem {
            backendValue: backendValues.selectByMouse
            enabled: root.isBackendValueAvailable("selectByMouse")
        }

        PropertyLabel {
            visible: !root.isTextInput
            text: qsTr("Select by keyboard")
            blockedByTemplate: !root.isBackendValueAvailable("selectByKeyboard")
        }

        FlagItem {
            visible: !root.isTextInput
            backendValue: backendValues.selectByKeyboard
            enabled: root.isBackendValueAvailable("selectByKeyboard")
        }
    }
}
