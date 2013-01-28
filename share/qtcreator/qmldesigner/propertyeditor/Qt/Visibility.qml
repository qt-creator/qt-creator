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
    finished: finishedNotify;
    caption: qsTr("Visibility")
    maximumHeight: 200;
    id: visibility;

    layout: VerticalLayout {

        QWidget {
            layout: HorizontalLayout {
                Label {                    
                    text: qsTr("Visibility")
                }
                CheckBox {
                    id: visibleCheckBox;
                    text: qsTr("Visible")
                    toolTip: qsTr("isVisible")
                    backendValue: backendValues.visible;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
                CheckBox {
                    id: smoothCheckBox;
                    text: qsTr("Smooth")
                    backendValue: backendValues.smooth;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {
                Label {                    
                    text: ""
                }
                CheckBox {
                    id: clipCheckBox;
                    text: qsTr("Clip")
                    backendValue: backendValues.clip;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Opacity")
                }

                DoubleSpinBox {
                    text: ""
                    decimals: 2
                    id: opacitySpinBox;
                    backendValue: backendValues.opacity
                    property variant backendValueValue: backendValues.opacity.value;
                    minimumWidth: 60;
                    minimum: 0;
                    maximum: 1;
                    singleStep: 0.1
                    baseStateFlag: isBaseState;                    
                }
                SliderWidget {
                    id: opacitySlider
                    minimum: 0
                    maximum: 100
                    property variant pureValue: backendValues.opacity.value;
                    onPureValueChanged: {
                        if (value != pureValue * 100)
                            value = pureValue * 100;
                    }
                    singleStep: 5;
                    backendValue: backendValues.opacity
                    onValueChanged: {
                    if ((value >= 0) && (value <= 100))
                        backendValues.opacity.value = value / 100;
                    }
                }
            }
        }       
    }
}
