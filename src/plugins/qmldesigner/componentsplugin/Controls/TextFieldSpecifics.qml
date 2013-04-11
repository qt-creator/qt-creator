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

import Bauhaus 1.0
import HelperWidgets 1.0

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0
        GroupBox {
            finished: finishedNotify;
            caption: qsTr("Text Field")
            layout: VerticalLayout {

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text:  qsTr("Text")
                            toolTip:  qsTr("The text shown on the text field")
                        }
                        LineEdit {
                            backendValue: backendValues.text
                            baseStateFlag: isBaseState;
                            translation: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text:  qsTr("Placeholder text")
                            toolTip: qsTr("The placeholder text")
                        }
                        LineEdit {
                            backendValue: backendValues.placeholderText
                            baseStateFlag: isBaseState;
                            translation: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Read only")
                            toolTip: qsTr("Determines whether the text field is read only.")
                        }
                        CheckBox {
                            text: backendValues.readOnly.value
                            backendValue: backendValues.readOnly
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Password mode")
                            toolTip: qsTr("Determines whether the text field is in password mode.")
                        }
                        CheckBox {
                            text: backendValues.passwordMode.value
                            backendValue: backendValues.passwordMode
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Input mask")
                            toolTip: qsTr("Restricts the valid text in the text field.")
                        }

                        LineEdit {
                            backendValue: backendValues.inputMask
                            baseStateFlag: isBaseState
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Echo mode")
                            toolTip: qsTr("Specifies how the text is displayed in the text field.")
                        }

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

            }
        }

        FontGroupBox {

        }

        ScrollArea {

        }
    }
}
