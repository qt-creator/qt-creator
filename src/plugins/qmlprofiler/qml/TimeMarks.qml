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

Canvas2D {
    id: timeDisplay

    property variant startTime
    property variant endTime
    property variant timePerPixel

    Component.onCompleted: {
        requestRedraw();
    }

    onWidthChanged: {
        requestRedraw();
    }
    onHeightChanged: {
        requestRedraw();
    }

    onDrawRegion: {
        drawBackgroundBars( ctxt, region );

        var totalTime = endTime - startTime;
        var spacing = width / totalTime;

        var initialBlockLength = 120;
        var timePerBlock = Math.pow(2, Math.floor( Math.log( totalTime / width * initialBlockLength ) / Math.LN2 ) );
        var pixelsPerBlock = timePerBlock * spacing;
        var pixelsPerSection = pixelsPerBlock / 5;
        var blockCount = width / pixelsPerBlock;

        var realStartTime = Math.floor(startTime/timePerBlock) * timePerBlock;
        var realStartPos = (startTime-realStartTime) * spacing;

        timePerPixel = timePerBlock/pixelsPerBlock;


        ctxt.fillStyle = "#000000";
        ctxt.font = "8px sans-serif";
        for (var ii = 0; ii < blockCount+1; ii++) {
            var x = Math.floor(ii*pixelsPerBlock - realStartPos);
            ctxt.strokeStyle = "#B0B0B0";
            ctxt.beginPath();
            ctxt.moveTo(x, 0);
            ctxt.lineTo(x, height);
            ctxt.stroke();

            ctxt.strokeStyle = "#CCCCCC";
            for (var jj=1; jj < 5; jj++) {
                var xx = Math.floor(ii*pixelsPerBlock + jj*pixelsPerSection - realStartPos);
                ctxt.beginPath();
                ctxt.moveTo(xx, 0);
                ctxt.lineTo(xx, height);
                ctxt.stroke();
            }
        }

        // gray off out-of-bounds areas
        var rectWidth;
        if (startTime < qmlEventList.traceStartTime()) {
            ctxt.fillStyle = "rgba(127,127,127,0.2)";
            rectWidth = (qmlEventList.traceStartTime() - startTime) * spacing;
            ctxt.fillRect(0, 0, rectWidth, height);
        }
        if (endTime > qmlEventList.traceEndTime()) {
            ctxt.fillStyle = "rgba(127,127,127,0.2)";
            var rectX = (qmlEventList.traceEndTime() - startTime) * spacing;
            rectWidth = (endTime - qmlEventList.traceEndTime()) * spacing;
            ctxt.fillRect(rectX, 0, rectWidth, height);
        }
    }

    function updateMarks(start, end) {
        if (startTime !== start || endTime !== end) {
            startTime = start;
            endTime = end;
            requestRedraw();
        }
    }

    function drawBackgroundBars( ctxt, region ) {
        var colorIndex = true;
        // row background
        for (var y=0; y < labels.height; y+= root.singleRowHeight) {
            ctxt.fillStyle = colorIndex ? "#f0f0f0" : "white";
            ctxt.strokeStyle = colorIndex ? "#f0f0f0" : "white";
            ctxt.fillRect(0, y, width, root.singleRowHeight);
            colorIndex = !colorIndex;
        }

        // separators
        var cumulatedHeight = 0;
        for (var i=0; i<labels.rowCount; i++) {
            cumulatedHeight += root.singleRowHeight + (labels.rowExpanded[i] ?
                    qmlEventList.uniqueEventsOfType(i) * root.singleRowHeight :
                    qmlEventList.maxNestingForType(i) * root.singleRowHeight);

            ctxt.strokeStyle = "#B0B0B0";
            ctxt.beginPath();
            ctxt.moveTo(0, cumulatedHeight);
            ctxt.lineTo(width, cumulatedHeight);
            ctxt.stroke();
        }

        // bottom
        ctxt.fillStyle = "#f5f5f5";
        ctxt.fillRect(0, labels.height, width, height - labels.height);
    }
}
