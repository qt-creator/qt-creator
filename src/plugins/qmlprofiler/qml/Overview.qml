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
import "Overview.js" as Plotter

Canvas2D {
    id: canvas

    // ***** properties
    height: 50
    property bool dataAvailable: false
    property variant startTime : 0
    property variant endTime : 0

    // ***** functions
    function clearDisplay()
    {
        dataAvailable = false;
        requestRedraw();
    }

    function updateRange() {
        var newStartTime = Math.round(rangeMover.x * qmlEventList.traceDuration() / width) + qmlEventList.traceStartTime();
        var newEndTime = Math.round((rangeMover.x + rangeMover.width) * qmlEventList.traceDuration() / width) + qmlEventList.traceStartTime();
        if (startTime !== newStartTime || endTime !== newEndTime) {
            zoomControl.setRange(newStartTime, newEndTime);
        }
    }

    // ***** connections to external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            if (qmlEventList) {
                startTime = zoomControl.startTime();
                endTime = zoomControl.endTime();
                var newRangeX = (startTime - qmlEventList.traceStartTime()) * width / qmlEventList.traceDuration();
                if (rangeMover.x !== newRangeX)
                    rangeMover.x = newRangeX;
                var newWidth = (endTime-startTime) * width / qmlEventList.traceDuration();
                if (rangeMover.width !== newWidth)
                    rangeMover.width = newWidth;
            }
        }
    }

    Connections {
        target: qmlEventList
        onDataReady: {
            if (qmlEventList.count() > 0) {
                dataAvailable = true;
                requestRedraw();
            }
        }
    }

    // ***** slots
    onDrawRegion: {
        Plotter.qmlEventList = qmlEventList;
        if (dataAvailable) {
            Plotter.plot(canvas, ctxt, region);
        } else {
            Plotter.drawGraph(canvas, ctxt, region)    //just draw the background
        }
    }

    // ***** child items
    MouseArea {
        anchors.fill: canvas
        function jumpTo(posX) {
            var newX = posX - rangeMover.width/2;
            if (newX < 0)
                newX = 0;
            if (newX + rangeMover.width > canvas.width)
                newX = canvas.width - rangeMover.width;
            rangeMover.x = newX;
            updateRange();
        }

        onPressed: {
            jumpTo(mouse.x);
        }
        onMousePositionChanged: {
            jumpTo(mouse.x);
        }
    }

    RangeMover {
        id: rangeMover
        visible: dataAvailable
    }

    Rectangle {
        height: 1
        width: parent.width
        color: "#858585"
    }
}
