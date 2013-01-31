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

QWidget {
    id: intEditor;

    property variant backendValue;
    property variant baseStateFlag;

    property variant caption;

    property variant maximumValue: 1000
    property variant minimumValue: -1000
    property variant step: 1
    property bool slider: true
    property alias alignment: label.alignment

    property bool editorEnabled: true

    layout: HorizontalLayout {
        Label {
            id: label
            text: caption
            toolTip: caption
            visible: caption != ""
        }

        SpinBox {
            enabled: editorEnabled
            backendValue: (intEditor.backendValue === undefined ||
            intEditor.backendValue === null)
            ? null : intEditor.backendValue;
            singleStep: step

            property variant backendValueValue: (intEditor.backendValue === undefined ||
            intEditor.backendValue === null)
            ? null : intEditor.backendValue.value;

            minimum: minimumValue
            maximum: maximumValue
            baseStateFlag: intEditor.baseStateFlag
        }


        QWidget {
            visible: intEditor.slider
            id: sliderWidget
            QSlider {
                id: intSlider
                enabled: editorEnabled
                y: sliderWidget.height / 2 - 12
                width: sliderWidget.width
                height: 24
                property alias backendValue: intEditor.backendValue
                orientation: "Qt::Horizontal"
                minimum: minimumValue
                maximum: maximumValue
                singleStep: step
                property int valueFromBackend: intEditor.backendValue.value;

                onValueFromBackendChanged: {
                    if (intSlider.maximum < valueFromBackend)
                        intSlider.maximum = valueFromBackend;
                    if (intSlider.minimum > valueFromBackend)
                        intSlider.minimum = valueFromBackend;
                    intSlider.value = valueFromBackend;
                }

                onValueChanged: {
                    if (backendValue != undefined && backendValue != null)
                    backendValue.value = value;
                }

                onSliderPressed: {
                    transaction.start();
                }
                onSliderReleased: {
                    transaction.end();
                }
            }
        }
    }
}
