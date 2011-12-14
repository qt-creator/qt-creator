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

.pragma library

var qmlEventList = 0;

//draw background of the graph
function drawGraph(canvas, ctxt, region)
{
    ctxt.fillStyle = "#eaeaea";
    ctxt.fillRect(0, 0, canvas.width, canvas.height);
}

//draw the actual data to be graphed
function drawData(canvas, ctxt, region)
{
    if ((!qmlEventList) || qmlEventList.count() == 0)
        return;

    var typeCount = 5;
    var width = canvas.width;
    var bump = 10;
    var height = canvas.height - bump;
    var blockHeight = height / typeCount;

    var spacing = width / qmlEventList.traceDuration();

    var highest = [0,0,0,0,0]; // note: change if typeCount changes

    for (var ii = 0; ii < qmlEventList.count(); ++ii) {

        var xx = (qmlEventList.getStartTime(ii) - qmlEventList.traceStartTime()) * spacing;
        if (xx > region.x + region.width)
            continue;

        var eventWidth = qmlEventList.getDuration(ii) * spacing;
        if (xx + eventWidth < region.x)
            continue;

        if (eventWidth < 1)
            eventWidth = 1;

        xx = Math.round(xx);
        var ty = qmlEventList.getType(ii);
        if (xx + eventWidth > highest[ty]) {
            // special: animations
            if (ty === 0 && qmlEventList.getAnimationCount(ii) >= 0) {
                var vertScale = qmlEventList.getMaximumAnimationCount() - qmlEventList.getMinimumAnimationCount();
                if (vertScale < 1)
                    vertScale = 1;
                var fraction = (qmlEventList.getAnimationCount(ii) - qmlEventList.getMinimumAnimationCount()) / vertScale;
                var eventHeight = blockHeight * (fraction * 0.85 + 0.15);
                var yy = bump + ty*blockHeight + blockHeight - eventHeight;

                var fpsFraction = qmlEventList.getFramerate(ii) / 60.0;
                if (fpsFraction > 1.0)
                    fpsFraction = 1.0;
                ctxt.fillStyle = "hsl("+(fpsFraction*0.27+0.028)+",0.3,0.65)";
                ctxt.fillRect(xx, yy, eventWidth, eventHeight);
            } else {
                var hue = ( qmlEventList.getEventId(ii) * 25 ) % 360;
                ctxt.fillStyle = "hsl("+(hue/360.0+0.001)+",0.3,0.65)";
                ctxt.fillRect(xx, bump + ty*blockHeight, eventWidth, blockHeight);
            }
            highest[ty] = xx+eventWidth;
        }
    }
}

function drawTimeBar(canvas, ctxt, region)
{
    if (!qmlEventList)
        return;

    var width = canvas.width;
    var height = 10;
    var startTime = qmlEventList.traceStartTime();
    var endTime = qmlEventList.traceEndTime();

    var totalTime = qmlEventList.traceDuration();
    var spacing = width / totalTime;

    var initialBlockLength = 120;
    var timePerBlock = Math.pow(2, Math.floor( Math.log( totalTime / width * initialBlockLength ) / Math.LN2 ) );
    var pixelsPerBlock = timePerBlock * spacing;
    var pixelsPerSection = pixelsPerBlock / 5;
    var blockCount = width / pixelsPerBlock;

    var realStartTime = Math.floor(startTime/timePerBlock) * timePerBlock;
    var realStartPos = (startTime-realStartTime) * spacing;

    var timePerPixel = timePerBlock/pixelsPerBlock;

    ctxt.fillStyle = "#000000";
    ctxt.font = "6px sans-serif";

    ctxt.fillStyle = "#cccccc";
    ctxt.fillRect(0, 0, width, height);
    for (var ii = 0; ii < blockCount+1; ii++) {
        var x = Math.floor(ii*pixelsPerBlock - realStartPos);

        // block boundary
        ctxt.strokeStyle = "#525252";
        ctxt.beginPath();
        ctxt.moveTo(x, height/2);
        ctxt.lineTo(x, height);
        ctxt.stroke();

        // block time label
        ctxt.fillStyle = "#000000";
        var timeString = prettyPrintTime((ii+0.5)*timePerBlock + realStartTime);
        ctxt.textAlign = "center";
        ctxt.fillText(timeString, x + pixelsPerBlock/2, height/2 + 3);
    }

    ctxt.fillStyle = "#808080";
    ctxt.fillRect(0, height-1, width, 1);
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

function plot(canvas, ctxt, region)
{
    drawGraph(canvas, ctxt, region);
    drawData(canvas, ctxt, region);
    drawTimeBar(canvas, ctxt, region);

}
