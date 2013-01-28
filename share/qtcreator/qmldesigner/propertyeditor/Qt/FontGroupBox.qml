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
    id: fontGroupBox
    caption: qsTr("Font")
    property variant showStyle: false
    layout: VerticalLayout {

        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Font")
                }

                FontComboBox {
                    backendValue: backendValues.font_family
                    baseStateFlag: isBaseState
                }
            }
        }        
        QWidget {
            id: sizeWidget
            property bool selectionFlag: selectionChanged
            
            property bool pixelSize: sizeType.currentText == "pixels"
            property bool isSetup;
            
            onSelectionFlagChanged: {
                isSetup = true;
                sizeType.currentText = "points";
                if (backendValues.font_pixelSize.isInModel)
                    sizeType.currentText = "pixels";
                isSetup = false;
            }            
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Size")
                }
                SpinBox {
                    minimum: 0
                    maximum: 400
                    visible: !sizeWidget.pixelSize
                    backendValue: backendValues.font_pointSize
                    baseStateFlag: isBaseState;
                }                
                SpinBox {
                    minimum: 0
                    maximum: 400
                    visible: sizeWidget.pixelSize
                    backendValue: backendValues.font_pixelSize
                    baseStateFlag: isBaseState;
                }                
                QComboBox {
                    id: sizeType
                    maximumWidth: 60
                    items : { ["pixels", "points"] }
                    onCurrentTextChanged: {
                        if (sizeWidget.isSetup)
                            return;
                        if (currentText == "pixels") {
                            backendValues.font_pointSize.resetValue();
                            backendValues.font_pixelSize.value = 8;
                        } else {
                            backendValues.font_pixelSize.resetValue();
                            }
                    }
                }
            }
        }               
        QWidget {
            layout: HorizontalLayout {
                Label {
                    text: qsTr("Font style")
                }
                FontStyleButtons {}
            }
        }
        QWidget {
            visible: showStyle
            layout: HorizontalLayout {                
                Label {
                    text: qsTr("Style")
                }
                ComboBox {
                    baseStateFlag:isBaseState
                    backendValue: (backendValues.style === undefined) ? dummyBackendValue : backendValues.style
                    items : { ["Normal", "Outline", "Raised", "Sunken"] }
                    currentText: backendValue.value;
                    onItemsChanged: {
                        currentText =  backendValue.value;
                    }
                }
            }
        }
    }
}
