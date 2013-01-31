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

GroupBox {
    finished: finishedNotify;
    caption: qsTr("Flickable")
    layout: VerticalLayout {
        QWidget {  // 1
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Content size")
                }

                DoubleSpinBox {
                    text: "W"
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    backendValue: backendValues.contentWidth
                    minimum: 0;
                    maximum: 8000;
                    baseStateFlag: isBaseState;
                }

                DoubleSpinBox {
                    singleStep: 1;
                    text: "H"
                    alignRight: false
                    spacing: 4
                    backendValue: backendValues.contentHeight
                    minimum: 0;
                    maximum: 8000;
                    baseStateFlag: isBaseState;
                }


            }
        } //QWidget  //1
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Flick direction")
                    toolTip: qsTr("Flickable direction")
                }

                ComboBox {
                    baseStateFlag: isBaseState
                    items : { ["AutoFlickDirection", "HorizontalFlick", "VerticalFlick", "HorizontalAndVerticalFlick"] }
                    currentText: backendValues.flickableDirection.value;
                    onItemsChanged: {
                        currentText =  backendValues.flickableDirection.value;
                    }
                    backendValue: backendValues.flickableDirection
                }
            }
        } //QWidget
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Behavior")
                    toolTip: qsTr("Bounds behavior")
                }

                ComboBox {
                    baseStateFlag: isBaseState
                    items : { ["StopAtBounds", "DragOverBounds", "DragAndOvershootBounds"] }
                    currentText: backendValues.boundsBehavior.value;
                    onItemsChanged: {
                        currentText =  backendValues.boundsBehavior.value;
                    }
                    backendValue: backendValues.boundsBehavior
                }
            }
        } //QWidget
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text:qsTr("Interactive")
                }
                CheckBox {
                    text: ""
                    backendValue: backendValues.interactive;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
            }
        }// QWidget
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Max. velocity")
                    toolTip: qsTr("Maximum flick velocity")
                }

                DoubleSpinBox {
                    text: ""
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    backendValue: backendValues.maximumFlickVelocity
                    minimum: 0;
                    maximum: 8000;
                    baseStateFlag: isBaseState;
                }
            }
        } //QWidget
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Deceleration")
                    toolTip: qsTr("Flick deceleration")
                }

                DoubleSpinBox {
                    text: ""
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    backendValue: backendValues.flickDeceleration
                    minimum: 0;
                    maximum: 8000;
                    baseStateFlag: isBaseState;
                }
            }
        } //QWidget
    }
}
