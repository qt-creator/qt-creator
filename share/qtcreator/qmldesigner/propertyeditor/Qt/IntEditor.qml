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

    layout: HorizontalLayout {
        Label {
            id: label
            text: caption
            toolTip: caption
            visible: caption != ""
        }

        SpinBox {
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
