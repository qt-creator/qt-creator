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
            caption: qsTr("Button")
            layout: VerticalLayout {

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text:  qsTr("Text")
                            toolTip:  qsTr("The text shown on the button")
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
                            text: qsTr("Checked")
                            toolTip: qsTr("The state of the button")
                        }
                        CheckBox {
                            text: backendValues.checked.value
                            backendValue: backendValues.checked
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Checkable")
                            toolTip: qsTr("Determines whether the button is checkable or not.")
                        }
                        CheckBox {
                            text: backendValues.checkable.value
                            backendValue: backendValues.checkable
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Enabled")
                            toolTip: qsTr("Determines whether the button is enabled or not.")
                        }
                        CheckBox {
                            text: backendValues.enabled.value
                            backendValue: backendValues.enabled
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Default button")
                            toolTip: qsTr("Sets the button as the default button in a dialog.")
                        }
                        CheckBox {
                            text: backendValues.defaultbutton.value
                            backendValue: backendValues.defaultbutton
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Tool tip")
                            toolTip: qsTr("The tool tip shown for the button.")
                        }
                        LineEdit {
                            backendValue: backendValues.toolTip
                            baseStateFlag: isBaseState;
                            translation: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Focus on press")
                            toolTip: "Determines whether the check box gets focus if pressed."
                        }
                        CheckBox {
                            text: backendValues.activeFocusOnPress.value
                            backendValue: backendValues.activeFocusOnPress
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Icon source")
                            toolTip: qsTr("The URL of an icon resource.")
                        }

                        FileWidget {
                            enabled: isBaseState || backendValues.id.value != "";
                            fileName: backendValues.iconSource.value;
                            onFileNameChanged: {
                                backendValues.iconSource.value = fileName;
                            }
                            itemNode: anchorBackend.itemNode
                            filter: "*.png *.gif *.jpg *.bmp *.jpeg"
                            showComboBox: true
                        }
                    }
                }
            }
        }
    }
}
