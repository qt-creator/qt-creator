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
            caption: qsTr("Grid")
            layout: VerticalLayout {
                IntEditor {
                    backendValue: backendValues.columns
                    caption: qsTr("Columns")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 100;
                }
                IntEditor {
                    backendValue: backendValues.rows
                    caption: qsTr("Rows")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 100;
                }
                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Flow")
                        }

                        ComboBox {
                            baseStateFlag: isBaseState
                            items : { ["LeftToRight", "TopToBottom"] }
                            currentText: backendValues.flow.value;
                            onItemsChanged: {
                                currentText =  backendValues.flow.value;
                            }
                            backendValue: backendValues.flow
                        }
                    }
                } //QWidget
//                Qt namespace enums not supported by the rewriter
//                QWidget {
//                    layout: HorizontalLayout {
//                        Label {
//                            text: qsTr("Layout direction")
//                        }

//                        ComboBox {
//                            baseStateFlag: isBaseState
//                            items : { ["LeftToRight", "RightToLeft"] }
//                            currentText: backendValues.layoutDirection.value;
//                            onItemsChanged: {
//                                currentText =  backendValues.layoutDirection.value;
//                            }
//                            backendValue: backendValues.layoutDirection
//                        }
//                    }
//                } //QWidget
                IntEditor {
                    backendValue: backendValues.spacing
                    caption: qsTr("Spacing")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 100;
                }
            }
        }
    }
}
