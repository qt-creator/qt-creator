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
            maximumHeight: 200;

            finished: finishedNotify;
            caption: qsTr("WebView")
            id: webViewSpecifics;

            layout: VerticalLayout {
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 0;
                        rightMargin: 0;
                        Label {
                            text: qsTr("URL")
                        }
                        LineEdit {
                            backendValue: backendValues.url
                            baseStateFlag: isBaseState;
                        }
                    }
                }                       

                IntEditor {
                    id: preferredWidth;
                    backendValue: backendValues.preferredWidth;
                    caption: qsTr("Pref Width")
                    toolTip: qsTr("Preferred Width")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 2000;
                }

                IntEditor {
                    id: webPageWidth;
                    backendValue: backendValues.preferredHeight;
                    caption: qsTr("Pref Height")
                    toolTip: qsTr("Preferred Height")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 2000;
                }

                QWidget {
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Scale")
                            toolTip: qsTr("Contents Scale")
                        }
                        DoubleSpinBox {
                            id: zoomSpinBox;
                            minimumWidth: 60;
                            text: ""
                            backendValue: backendValues.contentsScale;
                            minimum: 0.01
                            maximum: 10
                            singleStep: 0.1
                            baseStateFlag: isBaseState;
                            onBackendValueChanged: {
                                zoomSlider.value = backendValue.value * 10;
                            }
                        }
                        QSlider {
                            id: zoomSlider;
                            orientation: "Qt::Horizontal";
                            minimum: 1;
                            maximum: 100;
                            singleStep: 1;
                            onValueChanged: {
                                backendValues.contentsScale.value = value / 10;
                            }
                        }
                    }
                }
            }
        }
    }
}
