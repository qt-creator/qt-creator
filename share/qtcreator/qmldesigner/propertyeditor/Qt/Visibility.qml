/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
