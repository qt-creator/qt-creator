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

import QtQuick 1.1
import qtcomponents.custom 1.0 as Custom

Custom.CheckBox{
    id:checkbox
    property string text
    property string hint
    height:20
    width: Math.max(110, backgroundItem.rowWidth)
    property url backgroundSource: "qrc:welcome/images/lineedit.png";
    property url checkSource: "qrc:welcome/images/checked.png";
    background: Item {
        property int rowWidth: row.width
        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            BorderImage {
                source: checkBox.backgroundSource
                width: 16;
                height: 16;
                border.left: 4;
                border.right: 4;
                border.top: 4;
                border.bottom: 4
                Image {
                    source: checkBox.checkSource
                    width: 10; height: 10
                    anchors.centerIn: parent
                    visible: checkbox.checked
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: checkbox.text
            }
        }
        Keys.onSpacePressed:checked = !checked

    }
}
