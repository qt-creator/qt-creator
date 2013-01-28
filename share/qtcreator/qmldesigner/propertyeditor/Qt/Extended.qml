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
    caption: qsTr("Effect")
    id: extended;
    maximumHeight: 260;

    layout: VerticalLayout{

        property variant effect: backendValues.effect
        property variant complexNode: effect.complexNode

        QWidget  {
            maximumHeight: 40;
            layout: HorizontalLayout {
                Label {
                    text: "Effect ";
                }
                QComboBox {
                    enabled: isBaseState;
                    property variant type: backendValues.effect.complexNode.type
                    property variant dirty;
                    id: effectComboBox;
                    items : { [
                            "None",
                            "Blur",
                            "Opacity",
                            "Colorize",
                            "DropShadow"
                            ] }

                            onCurrentTextChanged: {

                                if (dirty) //avoid recursion;
                                return;
                                if (currentText == "")

                                if (backendValues.effect.complexNode.exists)
                                backendValues.effect.complexNode.remove();
                                if (currentText == "None") {
                                    ;
                                    } else if (backendValues.effect.complexNode != null) {
                                    backendValues.effect.complexNode.add("Qt/" + currentText);
                                    }
                            }

                            onTypeChanged: {
                                dirty = true;
                                if (backendValues.effect.complexNode.exists)
                                currentText = backendValues.effect.complexNode.type;
                                else
                                currentText = "None";
                                dirty = false;
                            }
                }
                QWidget {
                    fixedWidth: 100
                }
            }
            }// QWidget

            property variant properties: complexNode == null ? null : complexNode.properties

            QWidget {
                minimumHeight: 20;
                layout: QVBoxLayout {

                    QWidget {
                        visible: effectComboBox.currentText == "Blur";
                        layout: QVBoxLayout {
                            topMargin: 12;
                            IntEditor {
                                id: blurRadius;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.blurRadius : null;
                                caption: qsTr("Blur Radius:")
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }
                        }
                    }

                    QWidget {
                        visible: effectComboBox.currentText == "Opacity";
                        layout: QVBoxLayout {
                            DoubleSpinBox {
                                id: opcacityEffectSpinBox;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.opacity : null;
                                minimum: 0;
                                maximum: 1;
                                singleStep: 0.1;
                                baseStateFlag: isBaseState;
                            }
                        }
                    }

                    QWidget {
                        visible: effectComboBox.currentText == "Colorize";
                        layout: QVBoxLayout {

                            property variant colorProp: properties == null ? null : properties.color


                            ColorLabel {
                                text: "    Color"
                            }

                            ColorGroupBox {

                                finished: finishedNotify

                                backendColor: properties.color
                            }

                        }
                    }

                    QWidget {
                        visible: effectComboBox.currentText == "Pixelize";
                        layout: QVBoxLayout {
                            topMargin: 12;
                            IntEditor {
                                id: pixelSize;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.pixelSize : null;
                                caption: qsTr("Pixel Size:")
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }
                        }
                    }

                    QWidget {
                        visible: effectComboBox.currentText == "DropShadow";
                        layout: QVBoxLayout {

                            topMargin: 12;
                            IntEditor {
                                id: blurRadiusShadow;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.blurRadius : null;
                                caption: qsTr("Blur Radius:")
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }


                            ColorLabel {
                                text: "    Color"
                            }

                            ColorGroupBox {

                                finished: finishedNotify

                                backendColor: properties.color
                            }

                            IntEditor {
                                id: xOffset;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.xOffset : null;
                                caption: qsTr("x Offset:     ")
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }

                            IntEditor {
                                id: yOffset;
                                backendValue: backendValues.effect.complexNode.exists ? backendValues.effect.complexNode.properties.yOffset : null;
                                caption: qsTr("y Offset:     ")
                                baseStateFlag: isBaseState;

                                step: 1;
                                minimumValue: 0;
                                maximumValue: 20;
                            }
                        }
                    }
                }
            }
            } //QVBoxLayout
            } //GroupBox
