/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

import QtQuick 1.0
import Monitor 1.0

BorderImage {
    id: selectionRangeDetails

    property string startTime
    property string endTime
    property string duration
    property bool showDuration

    source: "popup_orange.png"
    border {
        left: 10; top: 10
        right: 20; bottom: 20
    }

    width: 170
    height: childrenRect.height
    z: 1
    visible: false
    x: 200
    y: 125

    //title
    Text {
        id: typeTitle
        text: qsTr("Selection")
        font.bold: true
        y: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -5
    }

    //details
    Column {
        id: col
        anchors.top: typeTitle.bottom
        x: 2
        Detail {
            label: qsTr("Start")
            content:  selectionRangeDetails.startTime
        }
        Detail {
            label: qsTr("End")
            visible: selectionRangeDetails.showDuration
            content:  selectionRangeDetails.endTime
        }
        Detail {
            label: qsTr("Duration")
            visible: selectionRangeDetails.showDuration
            content: selectionRangeDetails.duration
        }
    }

    Text {
        id: closeIcon
        x: selectionRangeDetails.width - 24
        y: 10
        text:"X"
        MouseArea {
            anchors.fill: parent
            anchors.leftMargin: -8
            onClicked: {
                root.selectionRangeMode = false;
                root.updateRangeButton();
            }
        }
    }

    MouseArea {
        width: col.width
        height: col.height + typeTitle.height + 30
        drag.target: parent
        onClicked: {
            if ((selectionRange.x < flick.contentX) ^ (selectionRange.x+selectionRange.width > flick.contentX + flick.width)) {
                root.recenter(selectionRange.startTime + selectionRange.duration/2);
            }
        }
    }
}
