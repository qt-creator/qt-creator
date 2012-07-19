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

import Qt 4.7
import Bauhaus 1.0


QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0

        RectangleColorGroupBox {
            id: colorsBox
            finished: finishedNotify;
        }

        GroupBox {

            finished: finishedNotify;
            caption: qsTr("Rectangle")

            layout: VerticalLayout {
                rightMargin: 24

                IntEditor {
                    enabled: colorsBox.hasBorder
                    opacity: enabled ? 1 : 0.6
                    toolTip: enabled ? qsTr("Border width") : qsTr("Border has to be solid to change width")
                    id: borderWidth;
                    backendValue: backendValues.border_width === undefined ? 0 : backendValues.border_width

                    caption: qsTr("Border")
                    baseStateFlag: isBaseState;

                    step: 1;
                    minimumValue: 0;
                    maximumValue: 100;
                }

                IntEditor {
                    property int border: backendValues.border_width.value === undefined ? 0 : backendValues.border_width.value
                    backendValue: backendValues.radius
                    caption: qsTr("Radius")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: Math.max(1, (Math.min(backendValues.width.value,backendValues.height.value) - border) / 2)
                }

            }
        }

        QScrollArea {
        }
    }
}
