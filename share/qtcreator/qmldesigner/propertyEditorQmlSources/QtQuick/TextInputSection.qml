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
            visible: textInputSection.isTextInput
            text: qsTr("Input mask")
        }

        LineEdit {
            visible: textInputSection.isTextInput
            backendValue: backendValues.inputMask
            Layout.fillWidth: true
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
            model:  ["Normal", "Password", "PasswordEchoOnEdit", "NoEcho"]
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
        }

        Label {
            text: qsTr("Flags")
            Layout.alignment: Qt.AlignTop
        }

        SecondColumnLayout {
            ColumnLayout {
                CheckBox {
                    text: qsTr("Read only")
                    backendValue: backendValues.readOnly;
                }

                CheckBox {
                    text: qsTr("Cursor visible")
                    backendValue: backendValues.cursorVisible;
                }

                CheckBox {
                    text: qsTr("Active focus on press")
                    backendValue:  backendValues.activeFocusOnPress;
                }

                CheckBox {
                    text: qsTr("Auto scroll")
                    backendValue:  backendValues.autoScroll;
                }

            }
        }
    }
}
