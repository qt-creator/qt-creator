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
            maximumHeight: 240;

            finished: finishedNotify;
            caption: qsTr("Image")

            layout: VerticalLayout {

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Source")
                        }

                        FileWidget {
                            enabled: isBaseState || backendValues.id.value != "";
                            fileName: backendValues.source.value;
                            onFileNameChanged: {
                                backendValues.source.value = fileName;
                            }
                            itemNode: anchorBackend.itemNode
                            filter: "*.png *.gif *.jpg *.bmp *.jpeg"
                            showComboBox: true
                        }
                    }
                }

                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Fill mode")
                        }

                        ComboBox {
                            baseStateFlag: isBaseState
                            items : { ["Stretch", "PreserveAspectFit", "PreserveAspectCrop", "Tile", "TileVertically", "TileHorizontally"] }
                            currentText: backendValues.fillMode.value;
                            onItemsChanged: {
                                currentText =  backendValues.fillMode.value;
                            }
                            backendValue: backendValues.fillMode
                        }
                    }
                }

                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Source size")
                        }

                        DoubleSpinBox {
                            text: "W"
                            alignRight: false
                            spacing: 4
                            singleStep: 1;
                            enabled: anchorBackend.hasParent;
                            backendValue: backendValues.sourceSize_width
                            minimum: -2000;
                            maximum: 2000;
                            baseStateFlag: isBaseState;
                        }

                        DoubleSpinBox {
                            singleStep: 1;
                            text: "H"
                            alignRight: false
                            spacing: 4
                            backendValue: backendValues.sourceSize_height
                            enabled: anchorBackend.hasParent;
                            minimum: -2000;
                            maximum: 2000;
                            baseStateFlag: isBaseState;
                        }


                    }
                } //QWidget  //1

            }
        }

        QScrollArea {}
    }
}
