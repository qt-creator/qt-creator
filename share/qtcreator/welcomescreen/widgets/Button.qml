/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

import Qt 4.7
import "../components/custom" as Custom

Custom.Button {
    id: button

    width: Math.max(100, labelItem.contentsWidth+20)
    height: 32

    background: BorderImage {
        source: {
            if (pressed)
                return "qrc:/welcome/images/btn_26_pressed.png"
            else
                if (containsMouse)
                    return "qrc:/welcome/images/btn_26_hover.png"
                else
                    return "qrc:/welcome/images/btn_26.png"
        }

        border { left: 5; right: 5; top: 5; bottom: 5 }
    }

    label: Item {
        property int contentsWidth : row.width
        Row {
            id: row
            spacing: 4
            anchors.centerIn: parent
            property int contentsWidth : row.width
            Image {
                id: image
                source: iconSource
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.Stretch //mm Image should shrink if button is too small, depends on QTBUG-14957
            }
            Text {
                id:text
                color: textColor
                anchors.verticalCenter: parent.verticalCenter
                text: button.text
                horizontalAlignment: Text.Center
            }
        }
    }

    Keys.onSpacePressed:clicked()
}
