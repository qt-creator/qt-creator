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

import QtQuick 1.0
import Monitor 1.0

TiledCanvas {
    id: timeDisplay

    property variant startTime
    property variant endTime
    property variant timePerPixel

    canvasSize.width: timeDisplay.width
    canvasSize.height: timeDisplay.height

    tileSize.width: width
    tileSize.height: height
    canvasWindow.width:  width
    canvasWindow.height: height

    Component.onCompleted: {
        requestPaint();
    }

    onWidthChanged: {
        requestPaint();
    }
    onHeightChanged: {
        requestPaint();
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
            ctxt.strokeStyle = "#909090";
            ctxt.beginPath();
            ctxt.moveTo(x, 0);
            ctxt.lineTo(x, height);
            ctxt.stroke();

            ctxt.strokeStyle = "#C0C0C0";
            for (var jj=1; jj < 5; jj++) {
                var xx = Math.floor(ii*pixelsPerBlock + jj*pixelsPerSection - realStartPos);
                ctxt.beginPath();
                ctxt.moveTo(xx, 0);
                ctxt.lineTo(xx, height);
                ctxt.stroke();
            }
        }
    }

    function updateMarks(start, end) {
        if (startTime !== start || endTime !== end) {
            startTime = start;
            endTime = end;
            requestPaint();
        }
    }

    function drawBackgroundBars( ctxt, region ) {
        var barHeight = Math.round(labels.height / labels.rowCount);
        for (var i=0; i<labels.rowCount; i++) {
            ctxt.fillStyle = i%2 ? "#f3f3f3" : "white"
            ctxt.strokeStyle = i%2 ? "#f3f3f3" : "white"
            ctxt.fillRect(0, i * barHeight, width, barHeight);
        }
        ctxt.fillStyle = "white";
        ctxt.fillRect(0, labels.height, width, height - labels.height);
    }
}
