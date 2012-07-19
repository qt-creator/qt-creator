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

QtObject {
    property alias linkFont: linkText.font
    property alias standardCaption: standardCaptionText.font
    property alias standstandardDescription: standardDescriptionText.font
    property alias italicDescription: italicDescriptionText.font
    property alias boldDescription: boldText.font
    property alias smallPath: smallPathText.font

    property list<Item> texts: [

        Text {
            id: smallPathText

            visible: false
            font.pixelSize: 12
            font.family: "Helvetica"
        },

        Text {
            id: linkText

            visible: false

            font.pixelSize: 13
            //font.bold: true
            font.family: "Helvetica"
        },

        Text {
            id: boldText

            visible: false

            font.pixelSize: 13
            font.bold: true
            font.family: "Helvetica"
        },

        Text {
            id: standardCaptionText

            visible: false

            font.family: "Helvetica"
            font.pixelSize: 14
        },

        Text {
            id: standardDescriptionText

            visible: false

            font.pixelSize: 13
            font.bold: false
            font.family: "Helvetica"
        },

        Text {
            id: italicDescriptionText

            visible: false

            font.pixelSize: 13
            font.bold: false
            font.family: "Helvetica"
        }
    ]

}
