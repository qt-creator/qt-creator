/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0


Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Text Input")

    property bool showEchoMode: false
    id: textInputSection

    SectionLayout {
        rows: 4
        columns: 2


        Label {
            text: qsTr("Input mask")
        }

        LineEdit {
            backendValue: backendValues.inputMask
            Layout.fillWidth: true
        }

        Label {
            visible: textInputSection.showEchoMode
            text: qsTr("Echo mode")
        }

        ComboBox {
            visible: textInputSection.showEchoMode
            Layout.fillWidth: true
            backendValue: backendValues.echoMode
            scope: "TextInput"
            model:  ["Normal", "Password", "PasswordEchoOnEdit", "NoEcho"]
        }

        Label {
            text: qsTr("Pass. char")
            toolTip: qsTr("Character displayed when users enter passwords.")
        }

        LineEdit {
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
