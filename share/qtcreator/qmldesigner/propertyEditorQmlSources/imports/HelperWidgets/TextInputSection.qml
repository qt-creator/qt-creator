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

    SectionLayout {
        PropertyLabel {
            text: qsTr("Selection color")
            tooltip: qsTr("Sets the background color of selected text.")
        }

        ColorEditor {
            backendValue: backendValues.selectionColor
            supportGradient: false
        }

        PropertyLabel {
            text: qsTr("Selected text color")
            tooltip: qsTr("Sets the color of selected text.")
        }

        ColorEditor {
            backendValue: backendValues.selectedTextColor
            supportGradient: false
        }

        PropertyLabel {
            text: qsTr("Selection mode")
            tooltip: qsTr("Sets the way text is selected with the mouse.")
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.mouseSelectionMode
                scope: root.isTextInput ? "TextInput" : "TextEdit"
                model: ["SelectCharacters", "SelectWords"]
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Input mask")
            tooltip: qsTr("Sets the allowed characters.")
        }

        SecondColumnLayout {
            visible: root.isTextInput

            LineEdit {
                backendValue: backendValues.inputMask
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                showTranslateCheckBox: false
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Echo mode")
            tooltip: qsTr("Sets the visibility mode.")
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
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Password character")
            tooltip: qsTr("Sets which character to display when passwords are entered.")
        }

        SecondColumnLayout {
            visible: root.isTextInput

            LineEdit {
                backendValue: backendValues.passwordCharacter
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                showTranslateCheckBox: false
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: !root.isTextInput
            text: qsTr("Tab stop distance")
            tooltip: qsTr("Default distance between tab stops in device units.")
        }

        SecondColumnLayout {
            visible: !root.isTextInput

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.tabStopDistance
                maximumValue: 200
                minimumValue: 0
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "px" }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: !root.isTextInput
            text: qsTr("Text margin")
            tooltip: qsTr("Margin around the text in the Text Edit in pixels.")
        }

        SecondColumnLayout {
            visible: !root.isTextInput

            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.textMargin
                maximumValue: 200
                minimumValue: -200
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "px" }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Maximum length")
            tooltip: qsTr("Sets the maximum length of the text.")
        }

        SecondColumnLayout {
            visible: root.isTextInput

            SpinBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.maximumLength
                minimumValue: 0
                maximumValue: 32767
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
        }

        FlagItem { backendValue: backendValues.readOnly }

        PropertyLabel {
            text: qsTr("Cursor visible")
            tooltip: qsTr("Toggles if the cursor is visible.")
        }

        FlagItem { backendValue: backendValues.cursorVisible }

        PropertyLabel {
            text: qsTr("Focus on press")
            tooltip: qsTr("Toggles if the text is focused on mouse click.")
        }

        FlagItem { backendValue: backendValues.activeFocusOnPress }

        PropertyLabel {
            visible: root.isTextInput
            text: qsTr("Auto scroll")
            tooltip: qsTr("Toggles if the text scrolls when it exceeds its boundary.")
        }

        FlagItem {
            visible: root.isTextInput
            backendValue: backendValues.autoScroll
        }

        PropertyLabel {
            text: qsTr("Overwrite mode")
            tooltip: qsTr("Toggles if overwriting text is allowed.")
        }

        FlagItem { backendValue: backendValues.overwriteMode }

        PropertyLabel {
            text: qsTr("Persistent selection")
            tooltip: qsTr("Toggles if the text should remain selected after moving the focus elsewhere.")
        }

        FlagItem { backendValue: backendValues.persistentSelection }

        PropertyLabel {
            text: qsTr("Select by mouse")
            tooltip: qsTr("Toggles if the text can be selected with the mouse.")
        }

        FlagItem { backendValue: backendValues.selectByMouse }

        PropertyLabel {
            visible: !root.isTextInput
            text: qsTr("Select by keyboard")
        }

        FlagItem {
            visible: !root.isTextInput
            backendValue: backendValues.selectByKeyboard
        }
    }
}
