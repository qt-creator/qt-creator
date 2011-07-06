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
import "MainView.js" as Plotter

TiledCanvas {
    id: timeDisplay

    canvasSize {
        width: timeDisplay.width
        height: timeDisplay.height
    }
    tileSize.width: width
    tileSize.height: height
    canvasWindow.width:  width
    canvasWindow.height: height


    Component.onCompleted: {
        requestPaint();
    }

    property variant startTime;
    property variant endTime;

    onStartTimeChanged: requestPaint();
    onEndTimeChanged: requestPaint();
    onWidthChanged: requestPaint();
    onHeightChanged: requestPaint();

    property variant timePerPixel;

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
                ctxt.moveTo(xx, labels.y);
                ctxt.lineTo(xx, height);
                ctxt.stroke();
            }

            ctxt.fillText(prettyPrintTime(ii*timePerBlock + realStartTime), x + 5, 5 + labels.y/2);
        }
    }

    function drawBackgroundBars( ctxt, region ) {
        var barHeight = labels.height / labels.rowCount;
        var originY = labels.y
        for (var i=0; i<labels.rowCount; i++) {
            ctxt.fillStyle = i%2 ? "#f3f3f3" : "white"
            ctxt.strokeStyle = i%2 ? "#f3f3f3" : "white"
            ctxt.fillRect(0, i * barHeight + originY, width, barHeight);
        }

        ctxt.fillStyle = "white";
        ctxt.fillRect(0, 0, width, originY);
    }

    function prettyPrintTime( t )
    {
        if (t <= 0) return "0";
        if (t<1000) return t+" ns";
        t = t/1000;
        if (t<1000) return t+" μs";
        t = Math.floor(t/100)/10;
        if (t<1000) return t+" ms";
        t = Math.floor(t)/1000;
        if (t<60) return t+" s";
        var m = Math.floor(t/60);
        t = Math.floor(t - m*60);
        return m+"m"+t+"s";
    }

    function detailedPrintTime( t )
    {
        if (t <= 0) return "0";
        if (t<1000) return t+" ns";
        t = Math.floor(t/1000);
        if (t<1000) return t+" μs";
        return (t/1000) + " ms";
    }

    // show exact time
    MouseArea {
        width: parent.width
        height: labels.y
        hoverEnabled: true

        function setStartTime(xpos) {
            var realTime = startTime + xpos * timePerPixel;
            timeDisplayText.text = detailedPrintTime(realTime);
            timeDisplayBegin.visible = true;
            timeDisplayBegin.x = xpos + flick.x;
        }

        function setEndTime(xpos) {
            var bt = startTime + (timeDisplayBegin.x - flick.x) * timePerPixel;
            var et = startTime + xpos * timePerPixel;
            var timeDisplayBeginTime = Math.min(bt, et);
            var timeDisplayEndTime = Math.max(bt, et);

            timeDisplayText.text = qsTr("length: %1").arg(detailedPrintTime(timeDisplayEndTime-timeDisplayBeginTime));
            timeDisplayEnd.visible = true;
            timeDisplayEnd.x = xpos + flick.x
        }

        onMousePositionChanged: {
            if (!Plotter.ranges.length)
                return;

            if (!pressed && timeDisplayEnd.visible)
                return;

            timeDisplayLabel.x = mouseX + flick.x
            timeDisplayLabel.visible = true

            if (pressed) {
                setEndTime(mouseX);
            } else {
                setStartTime(mouseX);
            }
        }

        onPressed:  {
            setStartTime(mouseX);
        }

        onEntered: {
            root.hideRangeDetails();
        }
        onExited: {
            if ((!pressed) && (!timeDisplayEnd.visible)) {
                timeDisplayLabel.hideAll();
            }
        }
    }
}
