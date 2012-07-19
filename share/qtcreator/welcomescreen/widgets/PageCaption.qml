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

import QtQuick 1.1
import QtQuick 1.0

Item {
    id: pageCaption
    width: 960
    height: 40
    property int textOffset: captionText.x + captionText.width

    property alias caption: captionText.text
    Text {
        id: captionText
        y: 9
        color: "#515153"
        anchors.left: parent.left
        anchors.leftMargin: 8
        font.pixelSize: 18
        font.bold: false
        font.family: "Helvetica"
    }
    Rectangle {
        height: 1
        color: "#a0a0a0"
        anchors.bottomMargin: 8
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }

    CustomColors {
        id: colors
    }
}
