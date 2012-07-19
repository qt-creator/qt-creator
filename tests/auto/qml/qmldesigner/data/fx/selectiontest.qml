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

Item {
    id: rootItem
    width: 640
    height: 480
Rectangle {
        height: 100;
        x: 104;
        y: 178;
        color: "#ffffff";
        width: 100;
        id: rectangle_1;
    }
Rectangle {
        x: 110;
        y: 44;
        color: "#ffffff";
        height: 100;
        width: 100;
        id: rectangle_2;
    }
Rectangle {
        width: 100;
        color: "#ffffff";
        height: 100;
        x: 323;
        y: 160;
        id: rectangle_3;
    }
Rectangle {
        color: "#ffffff";
        width: 100;
        height: 100;
        x: 233;
        y: 293;
        id: rectangle_4;
    }
}
