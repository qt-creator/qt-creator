/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 1.0
import Bauhaus 1.0

GroupBox {

    id: textInputGroupBox
    caption: qsTr("Text Input")
    property bool isTextInput: false

    layout: VerticalLayout {

        QWidget {
            visible: isTextInput
            layout: HorizontalLayout {

                Label {text: qsTr("Input mask") }

                LineEdit {
                    backendValue: backendValues.inputMask
                    baseStateFlag: isBaseState
                }
            }
        }

        QWidget {
            visible: isTextInput
            layout: HorizontalLayout {

                Label {text: qsTr("Echo mode") }

                ComboBox {
                    baseStateFlag: isBaseState
                    items : { ["Normal", "Password", "PasswordEchoOnEdit", "NoEcho"] }
                    currentText: backendValues.echoMode.value;
                    onItemsChanged: {
                        currentText =  backendValues.echoMode.value;
                    }
                    backendValue: backendValues.echoMode
                }
            }
        }

        QWidget {
            visible: isTextInput
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Pass. char")
                    toolTip: qsTr("Character displayed when users enter passwords.")
                }

                LineEdit {
                    backendValue: backendValues.passwordCharacter
                    baseStateFlag: isBaseState
                }
            }
        }

        QWidget {
            layout: HorizontalLayout {
                Label {text: qsTr("Flags") }

                CheckBox {
                    text: qsTr("Read only")
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue: backendValues.readOnly;
                }
            }
        }


        QWidget {
            layout: HorizontalLayout {
                Label {text: ""}

                CheckBox {

                    text: qsTr("Cursor visible")
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue: backendValues.cursorVisible;

                }

            }
        }

        QWidget {
            layout: HorizontalLayout {
                Label {text: ""}
                CheckBox {
                    text: qsTr("Active focus on press")
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue:  backendValues.activeFocusOnPress;
                }
            }
        }

        QWidget {
            visible: isTextInput
            layout: HorizontalLayout {
                Label {text: ""}
                CheckBox {
                    text: qsTr("Auto scroll")
                    baseStateFlag: isBaseState;
                    checkable: true;
                    backendValue:  backendValues.autoScroll;
                }
            }
        }
    }
}
