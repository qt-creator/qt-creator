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

QExtGroupBox {
    id: groupBox;

    property variant finished;

    property variant caption;

    property variant oldMaximumHeight;

    onFinishedChanged: {
        checkBox.raise();
        oldMaximumHeight = maximumHeight;
        visible = false;
        visible = true;
        //x = 6;
    }

    QToolButton {
        //QCheckBox {
        id: checkBox;

        text: groupBox.caption;
        focusPolicy: "Qt::NoFocus";
        styleSheetFile: "specialCheckBox.css";
        y: 0;
        x: 0;
        fixedHeight: 17
        fixedWidth: groupBox.width;
        arrowType: "Qt::DownArrow";
        toolButtonStyle: "Qt::ToolButtonTextBesideIcon";
        checkable: true;
        checked: true;
        font.bold: true;
        onClicked: {
            if (checked) {
                //groupBox.maximumHeight = oldMaximumHeight;
                collapsed = false;
                //text = "";
                //width = 12;
                //width = 120;
                arrowType =  "Qt::DownArrow";
                visible = true;
                } else {
                //groupBox.maximumHeight = 20;

                collapsed = true;
                //text = groupBox.caption;
                visible = true;
                //width = 120;
                arrowType =  "Qt::RightArrow";
                }
        }
    }
}
