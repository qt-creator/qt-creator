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

import HelperWidgets 1.0
import Bauhaus 1.0

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0

        GroupBox {
            caption: "Text Area"

            layout: VerticalLayout {


                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Text")
                            toolTip: qsTr("The text of the text area")
                        }
                        LineEdit {
                            backendValue: backendValues.text
                            baseStateFlag: isBaseState
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Read only")
                            toolTip: qsTr("Determines whether the text area is read only.")
                        }
                        CheckBox {
                            text: backendValues.readOnly.value
                            backendValue: backendValues.readOnly
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                ColorGroupBox {
                    caption: qsTr("Color")
                    toolTip: qsTr("The color of the text")
                    finished: finishedNotify
                    backendColor: backendValues.color
                }


                IntEditor {
                    backendValue: backendValues.documentMargins
                    caption: qsTr("Document margins")
                    toolTip: qsTr("The margins of the text area")
                    baseStateFlag: isBaseState
                    slider: false
                }
                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Frame")
                            toolTip: qsTr("Determines whether the text area has a frame.")
                        }
                        CheckBox {
                            text: backendValues.frame.value
                            backendValue: backendValues.frame
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                IntEditor {
                    backendValue: backendValues.frameWidth
                    caption: qsTr("Frame width")
                    toolTip: qsTr("The width of the frame")
                    baseStateFlag: isBaseState
                    slider: false
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Contents frame")
                            toolTip: qsTr("Determines whether the frame around contents is shown.")
                        }
                        CheckBox {
                            text: backendValues.frameAroundContents.value
                            backendValue: backendValues.frameAroundContents
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

            }
        }

        FontGroupBox {

        }

        GroupBox {
            caption: qsTr("Focus Handling")

            layout: VerticalLayout {

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Highlight on focus")
                            toolTip: qsTr("Determines whether the text area is highlighted on focus.")
                        }
                        CheckBox {
                            text: backendValues.highlightOnFocus.value
                            backendValue: backendValues.highlightOnFocus
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Tab changes focus")
                            toolTip: qsTr("Determines whether tab changes the focus of the text area.")
                        }
                        CheckBox {
                            text: backendValues.tabChangesFocus.value
                            backendValue: backendValues.tabChangesFocus
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }
                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Focus on press")
                            toolTip: qsTr("Determines whether the text area gets focus if pressed.")
                        }
                        CheckBox {
                            text: backendValues.activeFocusOnPress.value
                            backendValue: backendValues.activeFocusOnPress
                            baseStateFlag: isBaseState
                            checkable: true
                        }
                    }
                }

            }
        }

        ScrollArea {

        }
    }
}
