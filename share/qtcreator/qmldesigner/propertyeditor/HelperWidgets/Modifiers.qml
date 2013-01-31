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
    caption: qsTr("Manipulation")
    maximumHeight: 200;
    minimumHeight: 180;
    id: mofifiers;

    layout: VerticalLayout {

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: "Visibility"
                }

                CheckBox {
                    id: visibleCheckBox;
                    text: "Is visible";
                    backendValue: backendValues.visible === undefined ? false : backendValues.visible;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
                CheckBox {
                    id: clipCheckBox;
                    text: "Clip Content";
                    backendValue: backendValues.clip === undefined ? false : backendValues.clip;
                    baseStateFlag: isBaseState;
                    checkable: true;
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Opacity"
                }

                DoubleSpinBox {
                    text: ""
                    id: opacitySpinBox;
                    backendValue: backendValues.opacity === undefined ? null : backendValues.opacity
                    property variant backendValueValue: backendValues.opacity.value;
                    minimumWidth: 60;
                    minimum: 0;
                    maximum: 1;
                    singleStep: 0.1
                    baseStateFlag: isBaseState;
                    onBackendValueValueChanged: {
                        opacitySlider.value = backendValue.value * 100;
                    }
                }
                SliderWidget {
                    id: opacitySlider
                    minimum: 0
                    maximum: 100
                    singleStep: 5;
                    backendValue: backendValues.opacity === undefined ? null : backendValues.opacity
                    onValueChanged: {
                        if (backendValues.opacity !== undefined)
                        backendValues.opacity.value = value / 100;
                    }
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Origin";
                }
                ComboBox {
                    minimumWidth: 20
                    baseStateFlag: isBaseState

                    items : { [
                            "TopLeft", "Top", "TopRight", "Left", "Center", "Right", "BottomLeft", "Bottom",
                            "BottomRight"
                            ] }
										
					backendValue: backendValues.transformOrigin
                }
            }
        }
        QWidget {
            layout: HorizontalLayout {

                Label {
                    text: "Scale"
                }

                DoubleSpinBox {
                    text: ""
                    id: scaleSpinBox;

                    backendValue: backendValues.scale;
                    property variant backendValueValue: backendValues.scale.value;
                    minimumWidth: 60;
                    minimum: 0.01
                    maximum: 10
                    singleStep: 0.1
                    baseStateFlag: isBaseState;
                    onBackendValueValueChanged: {
                        scaleSlider.value = backendValue.value * 10;
                    }
                }
                SliderWidget {
                    id: scaleSlider;
                    backendValue: backendValues.scale;
                    minimum: 1;
                    maximum: 100;
                    singleStep: 1;
                    onValueChanged: {
                        backendValues.scale.value = value / 10;
                    }
                }
            }
        }
        IntEditor {
            backendValue: backendValues.rotation
            caption: qsTr("Rotation")
            baseStateFlag: isBaseState;
            step: 10;
            minimumValue: 0;
            maximumValue: 360;
        }

        IntEditor {
            backendValue: backendValues.z == undefined ? 0 : backendValues.z
            caption: qsTr("z")
            baseStateFlag: isBaseState;
            step: 1;
            minimumValue: -100;
            maximumValue: 100;
        }
    }
}
