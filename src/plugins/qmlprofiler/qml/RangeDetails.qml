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
    id: rangeDetails

    property string duration
    property string label
    property string type
    property string file
    property int line

    property bool locked: view.selectionLocked

    source: "popup_green.png"
    border {
        left: 10; top: 10
        right: 20; bottom: 20
    }

    width: col.width + 45
    height: childrenRect.height
    z: 1
    visible: false
    x: 200
    y: 25

    //title
    Text {
        id: typeTitle
        text: rangeDetails.type
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
            label: qsTr("Duration")
            content: rangeDetails.duration < 1000 ?
                        rangeDetails.duration + "μs" :
                        Math.floor(rangeDetails.duration/10)/100 + "ms"
        }
        Detail {
            opacity: content.length !== 0 ? 1 : 0
            label: qsTr("Details")
            content: {
                var inputString = rangeDetails.label;
                if (inputString.length > 7 && inputString.substring(0,7) == "file://") {
                    var pos = inputString.lastIndexOf("/");
                    return inputString.substr(pos+1);
                }
                var maxLen = 40;
                if (inputString.length > maxLen)
                    inputString = inputString.substring(0,maxLen)+"...";

                return inputString;
            }
        }
        Detail {
            opacity: content.length !== 0 ? 1 : 0
            label: qsTr("Location")
            content: {
                var file = rangeDetails.file;
                var pos = file.lastIndexOf("/");
                if (pos != -1)
                    file = file.substr(pos+1);
                return (file.length !== 0) ? (file + ":" + rangeDetails.line) : "";
            }
        }
    }

    MouseArea {
        width: col.width + 30
        height: col.height + typeTitle.height + 30
        drag.target: parent
        onClicked: {
            root.gotoSourceLocation(file, line);
            root.recenterOnItem(view.selectedItem);
        }
    }

    Image {
        id: lockIcon
        source: locked?"lock_closed.png" : "lock_open.png"
        anchors.top: closeIcon.top
        anchors.right: closeIcon.left
        anchors.rightMargin: 4
        width: 8
        height: 12
        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.selectionLocked = !root.selectionLocked;
            }
        }
    }

    Text {
        id: closeIcon
        x: col.width + 20
        y: 10
        text:"X"
        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.hideRangeDetails();
                view.selectedItem = -1;
            }
        }
    }


}
