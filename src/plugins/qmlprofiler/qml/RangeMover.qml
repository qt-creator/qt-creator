/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

Rectangle {
    id: rangeMover
    anchors.fill: parent
    color: "transparent"
    signal rangeDoubleClicked()

    property color handleColor: "#869cd1"
    property color rangeColor:"#444a64b8"
    property color dragColor:"#664a64b8"
    property color borderColor:"#cc4a64b8"
    property color dragMarkerColor: "#4a64b8"
    property color singleLineColor: "#4a64b8"

    property alias rangeLeft: leftRange.x
    property alias rangeRight: rightRange.x
    readonly property alias rangeWidth: selectedRange.width

    Rectangle {
        id: selectedRange

        x: leftRange.x
        width: rightRange.x - leftRange.x
        height: parent.height

        color: width > 1 ? (dragArea.pressed ? dragColor : rangeColor) : singleLineColor
    }

    Rectangle {
        id: leftRange

        onXChanged: {
            if (dragArea.drag.active)
                rightRange.x = x + dragArea.origWidth;
        }

        x: 0
        height: parent.height
        width: 1
        color: borderColor

        Rectangle {
            id: leftBorderHandle
            height: parent.height
            anchors.right: parent.left
            width: 7
            color: handleColor
            visible: false
            Image {
                source: "range_handle.png"
                x: 2
                width: 4
                height: 9
                fillMode: Image.Tile
                y: parent.height / 2 - 4
            }
        }

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: leftBorderHandle
                visible: true
            }
        }

        MouseArea {
            anchors.fill: leftBorderHandle

            drag.target: leftRange
            drag.axis: "XAxis"
            drag.minimumX: 0
            drag.maximumX: rangeMover.width
            drag.onActiveChanged: drag.maximumX = rightRange.x

            hoverEnabled: true

            onEntered: {
                parent.state = "highlighted";
            }
            onExited: {
                if (!pressed) parent.state = "";
            }
            onReleased: {
                if (!containsMouse) parent.state = "";
            }
        }
    }

    Rectangle {
        id: rightRange

        x: 1
        height: parent.height
        width: 1
        color: borderColor

        Rectangle {
            id: rightBorderHandle
            height: parent.height
            anchors.left: parent.right
            width: 7
            color: handleColor
            visible: false
            Image {
                source: "range_handle.png"
                x: 2
                width: 4
                height: 9
                fillMode: Image.Tile
                y: parent.height / 2 - 4
            }
        }

        states: State {
            name: "highlighted"
            PropertyChanges {
                target: rightBorderHandle
                visible: true
            }
        }

        MouseArea {
            anchors.fill: rightBorderHandle

            drag.target: rightRange
            drag.axis: "XAxis"
            drag.minimumX: 0
            drag.maximumX: rangeMover.width
            drag.onActiveChanged: drag.minimumX = leftRange.x

            hoverEnabled: true

            onEntered: {
                parent.state = "highlighted";
            }
            onExited: {
                if (!pressed) parent.state = "";
            }
            onReleased: {
                if (!containsMouse) parent.state = "";
            }
        }
    }

    MouseArea {
        id: dragArea
        property double origWidth: 0

        anchors.fill: selectedRange
        drag.target: leftRange
        drag.axis: "XAxis"
        drag.minimumX: 0
        drag.maximumX: rangeMover.width - origWidth
        drag.onActiveChanged: origWidth = selectedRange.width
        onDoubleClicked: parent.rangeDoubleClicked()
    }
}
