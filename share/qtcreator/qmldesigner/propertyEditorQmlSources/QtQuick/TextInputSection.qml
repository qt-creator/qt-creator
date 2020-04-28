/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Text Input")

    property bool isTextInput: false
    id: textInputSection

    SectionLayout {
        rows: 4
        columns: 2

        Label {
            text: qsTr("Mouse selection mode")
        }
        ComboBox {
            Layout.fillWidth: true
            backendValue: backendValues.mouseSelectionMode
            scope: "TextInput"
            model: ["SelectCharacters", "SelectWords"]
        }

        Label {
            visible: textInputSection.isTextInput
            text: qsTr("Input mask")
        }

        LineEdit {
            visible: textInputSection.isTextInput
            backendValue: backendValues.inputMask
            Layout.fillWidth: true
            showTranslateCheckBox: false
        }

        Label {
            visible: textInputSection.isTextInput
            text: qsTr("Echo mode")
        }

        ComboBox {
            visible: textInputSection.isTextInput
            Layout.fillWidth: true
            backendValue: backendValues.echoMode
            scope: "TextInput"
            model: ["Normal", "Password", "PasswordEchoOnEdit", "NoEcho"]
        }

        Label {
            visible: textInputSection.isTextInput
            text: qsTr("Pass. char")
            tooltip: qsTr("Character displayed when users enter passwords.")
        }

        LineEdit {
            visible: textInputSection.isTextInput
            backendValue: backendValues.passwordCharacter
            Layout.fillWidth: true
            showTranslateCheckBox: false
        }

        Label {
            visible: !textInputSection.isTextInput
            text: qsTr("Tab stop distance")
            tooltip: qsTr("Sets the default distance, in device units, between tab stops.")
        }
        SpinBox {
            visible: !textInputSection.isTextInput
            Layout.fillWidth: true
            backendValue: backendValues.tabStopDistance
            maximumValue: 200
            minimumValue: 0
        }

        Label {
            visible: !textInputSection.isTextInput
            text: qsTr("Text margin")
            tooltip: qsTr("Sets the margin, in pixels, around the text in the Text Edit.")
        }
        SpinBox {
            visible: !textInputSection.isTextInput
            Layout.fillWidth: true
            backendValue: backendValues.textMargin
            maximumValue: 200
            minimumValue: -200
        }

        Label {
            visible: textInputSection.isTextInput
            text: qsTr("Maximum length")
            tooltip: qsTr("Sets the maximum permitted length of the text in the TextInput.")
        }
        SpinBox {
            visible: textInputSection.isTextInput
            Layout.fillWidth: true
            backendValue: backendValues.maximumLength
            minimumValue: 0
            maximumValue: 32767
        }

        Label {
            text: qsTr("Flags")
            Layout.alignment: Qt.AlignTop
        }

        SecondColumnLayout {
            ColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    text: qsTr("Read only")
                    backendValue: backendValues.readOnly
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: qsTr("Cursor visible")
                    backendValue: backendValues.cursorVisible
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: qsTr("Active focus on press")
                    backendValue: backendValues.activeFocusOnPress
                }

                CheckBox {
                    visible: textInputSection.isTextInput
                    Layout.fillWidth: true
                    text: qsTr("Auto scroll")
                    backendValue: backendValues.autoScroll
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: qsTr("Overwrite mode")
                    backendValue: backendValues.overwriteMode
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: qsTr("Persistent selection")
                    backendValue: backendValues.persistentSelection
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: qsTr("Select by mouse")
                    backendValue: backendValues.selectByMouse
                }

                CheckBox {
                    visible: !textInputSection.isTextInput
                    Layout.fillWidth: true
                    text: qsTr("Select by keyboard")
                    backendValue: backendValues.selectByKeyboard
                }
            }
        }
    }
}
