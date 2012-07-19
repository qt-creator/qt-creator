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
import Monitor 1.0

Item {
    id: rangeDetails

    property string duration
    property string label
    property string type
    property string file
    property int line
    property int column
    property bool isBindingLoop

    property bool locked: view.selectionLocked

    width: col.width + 45
    height: col.height + 30
    z: 1
    visible: false
    x: 200
    y: 25

    property int yoffset: root.scrollY
    onYoffsetChanged: y = relativey + yoffset
    property int relativey : y - yoffset
    onYChanged: relativey = y - yoffset

    // shadow
    BorderImage {
        property int px: 4
        source: "dialog_shadow.png"

        border {
            left: px; top: px
            right: px; bottom: px
        }
        width: parent.width + 2*px - 1
        height: parent.height
        x: -px + 1
        y: px + 1
    }

    // title bar
    Rectangle {
        width: parent.width
        height: 20
        color: "#55a3b8"
        radius: 5
        border.width: 1
        border.color: "#a0a0a0"
    }
    Item {
        width: parent.width+1
        height: 11
        y: 10
        clip: true
        Rectangle {
            width: parent.width-1
            height: 15
            y: -5
            color: "#55a3b8"
            border.width: 1
            border.color: "#a0a0a0"
        }
    }

    //title
    Text {
        id: typeTitle
        text: "  "+rangeDetails.type
        font.bold: true
        height: 18
        y: 2
        verticalAlignment: Text.AlignVCenter
        width: parent.width
        color: "white"
    }

    // Details area
    Rectangle {
        color: "white"
        width: parent.width
        height: col.height + 10
        y: 20
        border.width: 1
        border.color: "#a0a0a0"

        //details
        Column {
            id: col
            x: 10
            y: 5
            Detail {
                label: qsTr("Duration:")
                content: rangeDetails.duration < 1000 ? rangeDetails.duration + "μs" : Math.floor(rangeDetails.duration/10)/100 + "ms"
            }
            Detail {
                opacity: content.length !== 0 ? 1 : 0
                label: qsTr("Details:")
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
                label: qsTr("Location:")
                content: {
                    var file = rangeDetails.file;
                    var pos = file.lastIndexOf("/");
                    if (pos != -1)
                        file = file.substr(pos+1);
                    return (file.length !== 0) ? (file + ":" + rangeDetails.line) : "";
                }
            }
            Detail {
                visible: isBindingLoop
                opacity: content.length != 0 ? 1 : 0
                label: qsTr("Binding loop detected")
            }
        }
    }

    MouseArea {
        width: col.width + 45
        height: col.height + 30
        drag.target: parent
        onClicked: {
            root.gotoSourceLocation(file, line, column);
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
        x: col.width + 30
        y: 4
        text:"X"
        color: "white"
        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.hideRangeDetails();
                view.selectedItem = -1;
            }
        }
    }

}
