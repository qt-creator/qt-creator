/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    width: rect.width; height: 50

    property real prevXStep: -1
    property real possibleValue: (canvas.canvasWindow.x + x) * Plotter.xScale(canvas)
    onPossibleValueChanged:  {
        var set = (!zooming || (zooming && prevXStep != canvas.canvasWindow.x))
        prevXStep = canvas.canvasWindow.x;
        if (set)
            value = possibleValue
    }

    property real value
    //onValueChanged: console.log("*************** " + value)

    /*Image {
        id: leftRange
        source: "range.png"
        anchors.horizontalCenter: parent.left
        anchors.bottom: parent.bottom
    }*/

    Rectangle {
        id: rect

        color: "#cc80b2f6"
        width: 20
        //anchors.left: parent.left
        //anchors.right: rightRange.horizontalCenter

        height: parent.height
    }

    /*Image {
        id: rightRange
        source: "range.png"
        x: 13
        anchors.bottom: parent.bottom

        MouseArea {
            width: parent.width
            height: 15
            drag.target: rightRange
            drag.axis: "XAxis"
            drag.minimumX: -7    //###
            drag.maximumX: canvas.width - rangeMover.width  //###
        }
    }*/

    MouseArea {
        anchors.fill: parent
        drag.target: rangeMover
        drag.axis: "XAxis"
        drag.minimumX: 0
        drag.maximumX: canvas.width - rangeMover.width //###
    }
}
