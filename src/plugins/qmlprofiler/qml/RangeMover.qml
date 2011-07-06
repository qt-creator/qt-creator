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

import "MainView.js" as Plotter
import QtQuick 1.0
import Monitor 1.0

Item {
    id: rangeMover


    property color lighterColor:"#cc80b2f6"
    property color darkerColor:"#cc6da1e8"
    property real value: (canvas.canvasWindow.x + x) * Plotter.xScale(canvas)
    property real zoomWidth: 20
    onZoomWidthChanged: timeDisplayLabel.hideAll();

    function updateZoomControls() {
        rightRange.x = rangeMover.width;
    }
    onXChanged: updateZoomControls();

    width: Math.max(rangeMover.zoomWidth, 20); height: 50

    MouseArea {
        anchors.fill: parent
        drag.target: rangeMover
        drag.axis: "XAxis"
        drag.minimumX: 0
        drag.maximumX: canvas.width - rangeMover.zoomWidth //###
    }

    Rectangle {
        id: frame
        color:"transparent"
        border.width: 1
        border.color: darkerColor
        anchors.fill: parent
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
    }

    Rectangle {
        id: rect

        color: lighterColor
        width: parent.zoomWidth
        height: parent.height
    }



    Rectangle {
        id: leftRange

        property int currentX: rangeMover.x
        property int currentWidth : rangeMover.zoomWidth

        x: -width
        height: parent.height
        width: 15
        color: darkerColor

        Text {
            anchors.centerIn: parent
            text:"<"
        }

        MouseArea {
            anchors.fill: parent
            drag.target: leftRange
            drag.axis: "XAxis"
            drag.minimumX: -parent.currentX
            drag.maximumX: parent.currentWidth - width - 1
            onPressed: {
                parent.currentX = rangeMover.x;
                parent.currentWidth = rangeMover.zoomWidth;
            }
        }
        onXChanged: {
            if (x + width != 0) {
                rangeMover.zoomWidth = currentWidth - x - width;
                rangeMover.x = currentX + x + width;
                x = -width;
            }
        }
    }

    Rectangle {
        id: rightRange
        property int currentX: rangeMover.x
        property int widthSpace: rangeMover.width - rangeMover.zoomWidth

        height: parent.height
        width: 15
        x: rangeMover.width
        color: darkerColor;

        Text {
            anchors.centerIn: parent
            text:">"
        }
        MouseArea {
            anchors.fill: parent
            drag.target: rightRange
            drag.axis: "XAxis"
            drag.minimumX: 1 + parent.widthSpace
            drag.maximumX: canvas.width - parent.currentX;
            onPressed: {
                parent.currentX = rangeMover.x;
                parent.widthSpace = rangeMover.width - rangeMover.zoomWidth;
            }
            onReleased: rightRange.x = rangeMover.width;
        }
        onXChanged: {
            if (x != rangeMover.width) {
                rangeMover.zoomWidth = x - widthSpace;
            }
        }
    }

}
